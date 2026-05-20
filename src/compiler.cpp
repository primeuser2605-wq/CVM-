//
// compiler.cpp
// -------------
// Walks the AST (via the visitor pattern) and emits bytecode into a Chunk.
// Forward jumps use back-patching: we emit a placeholder operand, remember
// its offset, compile the body, then fill in the correct target.
//

#include "compiler.h"
#include <iostream>
#include <iomanip>
#include <cstdio>

namespace cvm {

// ======================== Public entry points ===============================

Chunk Compiler::compile(const std::vector<StmtPtr>& program) {
    chunk_ = Chunk{};
    variables_.clear();

    for (const auto& stmt : program) {
        stmt->accept(*this);
    }
    chunk_.writeOp(OpCode::HALT);
    return std::move(chunk_);
}

// ======================== Symbol table ======================================

int Compiler::getOrCreateSlot(const std::string& name) {
    auto it = variables_.find(name);
    if (it != variables_.end()) return it->second;
    int slot = static_cast<int>(variables_.size());
    variables_[name] = slot;
    return slot;
}

// ======================== Expression visitors ===============================

void Compiler::visit(const NumberExpr& e) {
    int idx = chunk_.addConstant(Value{e.value});
    chunk_.writeOp(OpCode::PUSH_CONST);
    chunk_.writeShort(static_cast<uint16_t>(idx));
}

void Compiler::visit(const BoolExpr& e) {
    chunk_.writeOp(e.value ? OpCode::PUSH_TRUE : OpCode::PUSH_FALSE);
}

void Compiler::visit(const VarExpr& e) {
    auto it = variables_.find(e.name);
    if (it == variables_.end()) {
        throw CompileError("Undefined variable '" + e.name + "'.");
    }
    chunk_.writeOp(OpCode::LOAD);
    chunk_.writeShort(static_cast<uint16_t>(it->second));
}

void Compiler::visit(const BinaryExpr& e) {
    // Emit left, then right, so the VM finds them in the correct stack order:
    //   stack top  → right
    //   below      → left
    e.left->accept(*this);
    e.right->accept(*this);

    switch (e.op) {
        case TokenType::PLUS:  chunk_.writeOp(OpCode::ADD); break;
        case TokenType::MINUS: chunk_.writeOp(OpCode::SUB); break;
        case TokenType::STAR:  chunk_.writeOp(OpCode::MUL); break;
        case TokenType::SLASH: chunk_.writeOp(OpCode::DIV); break;
        case TokenType::EQ:    chunk_.writeOp(OpCode::EQ);  break;
        case TokenType::LT:    chunk_.writeOp(OpCode::LT);  break;
        case TokenType::GT:    chunk_.writeOp(OpCode::GT);  break;
        default:
            throw CompileError("Unknown binary operator.");
    }
}

// ======================== Statement visitors ================================

void Compiler::visit(const AssignStmt& s) {
    // Evaluate the RHS, then store into the variable's slot.
    s.value->accept(*this);
    int slot = getOrCreateSlot(s.name);
    chunk_.writeOp(OpCode::STORE);
    chunk_.writeShort(static_cast<uint16_t>(slot));
}

void Compiler::visit(const PrintStmt& s) {
    s.value->accept(*this);
    chunk_.writeOp(OpCode::PRINT);
}

void Compiler::visit(const BlockStmt& s) {
    for (const auto& stmt : s.statements) stmt->accept(*this);
}

// if (cond) then else? end
//
//   <cond bytecode>
//   JMP_IF_FALSE  → else_start     ← back-patched
//   <then bytecode>
//   JMP           → end            ← back-patched
// else_start:
//   <else bytecode>                ← omitted if no else
// end:
void Compiler::visit(const IfStmt& s) {
    s.condition->accept(*this);

    size_t jumpToElse = emitJump(OpCode::JMP_IF_FALSE);
    s.thenBranch->accept(*this);

    size_t jumpToEnd = emitJump(OpCode::JMP);

    // else_start:
    patchJump(jumpToElse);
    if (s.elseBranch) s.elseBranch->accept(*this);

    // end:
    patchJump(jumpToEnd);
}

// while (cond) body
//
// loop_start:
//   <cond bytecode>
//   JMP_IF_FALSE  → loop_end       ← back-patched
//   <body bytecode>
//   JMP           → loop_start     (backwards, target already known)
// loop_end:
void Compiler::visit(const WhileStmt& s) {
    size_t loopStart = chunk_.code.size();

    s.condition->accept(*this);
    size_t exitJump = emitJump(OpCode::JMP_IF_FALSE);

    s.body->accept(*this);

    // Jump back — target is known, no patching needed.
    chunk_.writeOp(OpCode::JMP);
    chunk_.writeShort(static_cast<uint16_t>(loopStart));

    patchJump(exitJump);
}

// ======================== Jump helpers ======================================

// Write the opcode plus a 2-byte placeholder (0xFFFF). Return the offset of
// the placeholder so we can fill it in later with patchJump().
size_t Compiler::emitJump(OpCode op) {
    chunk_.writeOp(op);
    size_t offset = chunk_.code.size();
    chunk_.writeShort(0xFFFF);  // placeholder
    return offset;
}

// Set the 2-byte operand at `offset` to "current position" (jump target).
void Compiler::patchJump(size_t offset) {
    uint16_t target = static_cast<uint16_t>(chunk_.code.size());
    chunk_.patchShort(offset, target);
}

// ======================== Disassembler ======================================

void Compiler::disassemble(const Chunk& chunk) {
    std::cout << "=== BYTECODE ===\n";
    std::cout << "-- constants --\n";
    for (size_t i = 0; i < chunk.constants.size(); ++i) {
        std::cout << "  [" << i << "] " << valueToString(chunk.constants[i]) << "\n";
    }
    std::cout << "-- code --\n";

    size_t ip = 0;
    while (ip < chunk.code.size()) {
        // Print offset as 4-digit zero-padded number, then reset fill.
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%04zu  ", ip);
        std::cout << buf;

        OpCode op = static_cast<OpCode>(chunk.code[ip]);
        std::cout << std::left << std::setw(14) << opName(op) << std::right;
        ip++;

        if (hasOperand(op)) {
            uint16_t operand = Chunk::readShort(chunk.code, ip);
            std::cout << operand;
            // Annotate PUSH_CONST with the actual constant value.
            if (op == OpCode::PUSH_CONST && operand < chunk.constants.size()) {
                std::cout << "  ; " << valueToString(chunk.constants[operand]);
            }
            ip += 2;
        }
        std::cout << "\n";
    }
    std::cout << std::right;
}

} // namespace cvm
