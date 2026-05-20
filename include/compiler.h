#pragma once
//
// compiler.h
// -----------
// The Compiler is a visitor over the AST that emits bytecode into a Chunk.
// It also manages a symbol table mapping variable names → stack slot numbers.
//

#include "ast.h"
#include "bytecode.h"
#include <unordered_map>
#include <string>
#include <stdexcept>

namespace cvm {

class CompileError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class Compiler : public ExprVisitor, public StmtVisitor {
public:
    // Compile a whole program into a Chunk.
    Chunk compile(const std::vector<StmtPtr>& program);

    // Disassemble (pretty-print) a chunk — useful for --bytecode output.
    static void disassemble(const Chunk& chunk);

private:
    Chunk chunk_;

    // Symbol table: variable name → slot index.
    // Slots are allocated in order of first assignment.
    std::unordered_map<std::string, int> variables_;

    int getOrCreateSlot(const std::string& name);

    // --- Visitor implementations (Expr) ---
    void visit(const NumberExpr&) override;
    void visit(const BoolExpr&)   override;
    void visit(const VarExpr&)    override;
    void visit(const BinaryExpr&) override;

    // --- Visitor implementations (Stmt) ---
    void visit(const AssignStmt&) override;
    void visit(const PrintStmt&)  override;
    void visit(const IfStmt&)     override;
    void visit(const WhileStmt&)  override;
    void visit(const BlockStmt&)  override;

    // Emit a jump with a placeholder operand; return the offset of that
    // placeholder so we can patch it once we know the target.
    size_t emitJump(OpCode op);
    void   patchJump(size_t offset);
};

} // namespace cvm
