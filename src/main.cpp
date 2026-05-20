//
// main.cpp
// ---------
// The CVM++ command-line driver.
//
// Usage:
//   cvm <script.cvm> [--tokens] [--ast] [--bytecode] [--trace]
//
// Flags (any combination):
//   --tokens    print the token stream
//   --ast       print the abstract syntax tree
//   --bytecode  disassemble the compiled bytecode
//   --trace     trace each VM instruction at runtime (to stderr)
//
// With no flags, just runs the program and prints its output.
//

#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "vm.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>

// ===========================================================================
//  AST pretty-printer (visitor). Reused from our Step 3 demo.
// ===========================================================================
namespace cvm {
class AstPrinter : public ExprVisitor, public StmtVisitor {
public:
    void print(const std::vector<StmtPtr>& program) {
        for (const auto& s : program) s->accept(*this);
    }
    void visit(const AssignStmt& s) override {
        line("Assign(" + s.name + ")");
        indent_++; s.value->accept(*this); indent_--;
    }
    void visit(const PrintStmt& s) override {
        line("Print");
        indent_++; s.value->accept(*this); indent_--;
    }
    void visit(const IfStmt& s) override {
        line("If");
        indent_++;
        line("cond:");  indent_++; s.condition->accept(*this);  indent_--;
        line("then:");  indent_++; s.thenBranch->accept(*this); indent_--;
        if (s.elseBranch) { line("else:"); indent_++; s.elseBranch->accept(*this); indent_--; }
        indent_--;
    }
    void visit(const WhileStmt& s) override {
        line("While");
        indent_++;
        line("cond:"); indent_++; s.condition->accept(*this); indent_--;
        line("body:"); indent_++; s.body->accept(*this);      indent_--;
        indent_--;
    }
    void visit(const BlockStmt& s) override {
        line("Block");
        indent_++;
        for (const auto& stmt : s.statements) stmt->accept(*this);
        indent_--;
    }
    void visit(const NumberExpr& e) override { line("Number(" + std::to_string(e.value) + ")"); }
    void visit(const BoolExpr& e)   override { line(std::string("Bool(") + (e.value ? "true" : "false") + ")"); }
    void visit(const VarExpr& e)    override { line("Var(" + e.name + ")"); }
    void visit(const BinaryExpr& e) override {
        line(std::string("Binary(") + tokenTypeName(e.op) + ")");
        indent_++; e.left->accept(*this); e.right->accept(*this); indent_--;
    }
private:
    int indent_ = 0;
    void line(const std::string& s) {
        for (int i = 0; i < indent_; i++) std::cout << "  ";
        std::cout << s << "\n";
    }
};
} // namespace cvm

// ===========================================================================
//  Helpers
// ===========================================================================
static std::string readFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Could not open file: " + path);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static void printUsage() {
    std::cerr <<
        "CVM++ - a tiny compiled scripting language\n"
        "\n"
        "Usage:\n"
        "  cvm <script.cvm> [inspection options]      compile and run from source\n"
        "  cvm <script.cvm> -o <out.cvmb>             compile source -> bytecode file\n"
        "  cvm --run <file.cvmb> [--trace]            run a saved bytecode file\n"
        "\n"
        "Inspection options (source mode only):\n"
        "  --tokens     print the token stream\n"
        "  --ast        print the abstract syntax tree\n"
        "  --bytecode   disassemble compiled bytecode\n"
        "  --trace      trace each VM instruction (stderr)\n"
        "  -h, --help   show this help\n"
        "\n"
        "Examples:\n"
        "  cvm tests/demo_calculator.cvm\n"
        "  cvm tests/demo_calculator.cvm -o calc.cvmb\n"
        "  cvm --run calc.cvmb\n";
}

// ===========================================================================
//  main
// ===========================================================================
int main(int argc, char** argv) {
    if (argc < 2) { printUsage(); return 1; }

    std::string scriptPath;     // input source file (mode 1 or 2)
    std::string outBytecode;    // -o target           (mode 2 only)
    std::string runBytecode;    // --run target        (mode 3 only)
    bool showTokens = false, showAst = false, showBytecode = false, trace = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if      (arg == "--tokens")   showTokens   = true;
        else if (arg == "--ast")      showAst      = true;
        else if (arg == "--bytecode") showBytecode = true;
        else if (arg == "--trace")    trace        = true;
        else if (arg == "-h" || arg == "--help") { printUsage(); return 0; }
        else if (arg == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "-o requires a filename.\n";
                return 1;
            }
            outBytecode = argv[++i];
        }
        else if (arg == "--run") {
            if (i + 1 >= argc) {
                std::cerr << "--run requires a .cvmb filename.\n";
                return 1;
            }
            runBytecode = argv[++i];
        }
        else if (!arg.empty() && arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage();
            return 1;
        }
        else {
            if (!scriptPath.empty()) {
                std::cerr << "Only one script file at a time.\n";
                return 1;
            }
            scriptPath = arg;
        }
    }

    // --- Sanity checks: which mode are we in? ---
    if (!runBytecode.empty() && (!scriptPath.empty() || !outBytecode.empty())) {
        std::cerr << "--run cannot be combined with a source file or -o.\n";
        return 1;
    }
    if (runBytecode.empty() && scriptPath.empty()) {
        printUsage();
        return 1;
    }

    try {
        // =====================================================================
        // MODE 3: --run <file.cvmb>   (just run pre-compiled bytecode)
        // =====================================================================
        if (!runBytecode.empty()) {
            cvm::Chunk chunk = cvm::Chunk::load(runBytecode);
            std::cout << "=== OUTPUT ===\n";
            cvm::VM vm;
            vm.run(chunk, trace);
            return 0;
        }

        // =====================================================================
        // MODE 1 or 2: read source, lex, parse, compile.
        // =====================================================================
        std::string source = readFile(scriptPath);

        // ---- Lex ----
        cvm::Lexer lexer(source);
        auto tokens = lexer.tokenize();
        if (showTokens) {
            std::cout << "=== TOKENS ===\n";
            for (const auto& t : tokens) {
                std::cout << "Line " << t.line << "  " << cvm::tokenTypeName(t.type);
                if (!t.lexeme.empty()) std::cout << "(" << t.lexeme << ")";
                std::cout << "\n";
            }
            std::cout << "\n";
        }

        // ---- Parse ----
        cvm::Parser parser(std::move(tokens));
        auto program = parser.parseProgram();
        if (showAst) {
            std::cout << "=== AST ===\n";
            cvm::AstPrinter printer;
            printer.print(program);
            std::cout << "\n";
        }

        // ---- Compile ----
        cvm::Compiler compiler;
        auto chunk = compiler.compile(program);
        if (showBytecode) {
            cvm::Compiler::disassemble(chunk);
            std::cout << "\n";
        }

        // =====================================================================
        // MODE 2: -o <file.cvmb>   (save bytecode and stop)
        // =====================================================================
        if (!outBytecode.empty()) {
            chunk.save(outBytecode);
            std::cout << "Wrote bytecode to " << outBytecode
                      << " (" << chunk.code.size() << " code bytes, "
                      << chunk.constants.size() << " constants)\n";
            return 0;
        }

        // =====================================================================
        // MODE 1: compile and run
        // =====================================================================
        std::cout << "=== OUTPUT ===\n";
        cvm::VM vm;
        vm.run(chunk, trace);
    }
    catch (const cvm::LexError& e) {
        std::cerr << "Lex error on line " << e.line << ": " << e.what() << "\n";
        return 1;
    }
    catch (const cvm::ParseError& e) {
        std::cerr << "Parse error on line " << e.line << ": " << e.what() << "\n";
        return 1;
    }
    catch (const cvm::CompileError& e) {
        std::cerr << "Compile error: " << e.what() << "\n";
        return 1;
    }
    catch (const cvm::RuntimeError& e) {
        std::cerr << "Runtime error: " << e.what() << "\n";
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
