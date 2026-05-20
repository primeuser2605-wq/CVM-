//
// vm.cpp
// -------
// The heart of CVM++: a classic fetch-decode-execute loop over our bytecode.
//

#include "vm.h"
#include <iostream>

namespace cvm {

// -------------------- Stack helpers --------------------
Value VM::pop() {
    if (stack_.empty()) throw RuntimeError("Stack underflow.");
    Value v = std::move(stack_.back());
    stack_.pop_back();
    return v;
}
Value& VM::top() {
    if (stack_.empty()) throw RuntimeError("Stack underflow on top().");
    return stack_.back();
}
int VM::popInt() {
    Value v = pop();
    if (!std::holds_alternative<int>(v))
        throw RuntimeError("Expected int on stack.");
    return std::get<int>(v);
}
bool VM::popBool() {
    Value v = pop();
    if (!std::holds_alternative<bool>(v))
        throw RuntimeError("Expected bool on stack.");
    return std::get<bool>(v);
}

// -------------------- Arithmetic --------------------
void VM::binaryIntOp(OpCode op) {
    // Note the order: right was pushed LAST, so it pops FIRST.
    int b = popInt();
    int a = popInt();
    int result = 0;
    switch (op) {
        case OpCode::ADD: result = a + b; break;
        case OpCode::SUB: result = a - b; break;
        case OpCode::MUL: result = a * b; break;
        case OpCode::DIV:
            if (b == 0) throw RuntimeError("Division by zero.");
            result = a / b;
            break;
        default: throw RuntimeError("Bad binary op.");
    }
    push(Value{result});
}

void VM::comparisonOp(OpCode op) {
    int b = popInt();
    int a = popInt();
    bool r = false;
    switch (op) {
        case OpCode::EQ: r = (a == b); break;
        case OpCode::LT: r = (a < b);  break;
        case OpCode::GT: r = (a > b);  break;
        default: throw RuntimeError("Bad comparison op.");
    }
    push(Value{r});
}

// -------------------- Main loop --------------------
void VM::run(const Chunk& chunk, bool trace, std::ostream* out) {
    if (!out) out = &std::cout;

    stack_.clear();
    variables_.assign(64, Value{0});  // room for 64 variables; grows if needed

    const auto& code = chunk.code;
    size_t ip = 0;

    while (ip < code.size()) {
        if (trace) {
            std::cerr << "[ip=" << ip << "  stack=";
            for (const auto& v : stack_) std::cerr << valueToString(v) << " ";
            std::cerr << "] " << opName(static_cast<OpCode>(code[ip])) << "\n";
        }

        OpCode op = static_cast<OpCode>(code[ip++]);

        switch (op) {
            // -------- Constants & literals --------
            case OpCode::PUSH_CONST: {
                uint16_t idx = Chunk::readShort(code, ip);
                ip += 2;
                if (idx >= chunk.constants.size())
                    throw RuntimeError("Bad constant index.");
                push(chunk.constants[idx]);
                break;
            }
            case OpCode::PUSH_TRUE:  push(Value{true});  break;
            case OpCode::PUSH_FALSE: push(Value{false}); break;
            case OpCode::POP:        pop(); break;

            // -------- Variables --------
            case OpCode::LOAD: {
                uint16_t slot = Chunk::readShort(code, ip);
                ip += 2;
                if (slot >= variables_.size())
                    variables_.resize(slot + 1, Value{0});
                push(variables_[slot]);
                break;
            }
            case OpCode::STORE: {
                uint16_t slot = Chunk::readShort(code, ip);
                ip += 2;
                if (slot >= variables_.size())
                    variables_.resize(slot + 1, Value{0});
                variables_[slot] = pop();
                break;
            }

            // -------- Arithmetic --------
            case OpCode::ADD:
            case OpCode::SUB:
            case OpCode::MUL:
            case OpCode::DIV:
                binaryIntOp(op);
                break;

            // -------- Comparisons --------
            case OpCode::EQ:
            case OpCode::LT:
            case OpCode::GT:
                comparisonOp(op);
                break;

            // -------- Control flow --------
            case OpCode::JMP: {
                uint16_t target = Chunk::readShort(code, ip);
                ip = target;
                break;
            }
            case OpCode::JMP_IF_FALSE: {
                uint16_t target = Chunk::readShort(code, ip);
                ip += 2;
                bool cond = popBool();
                if (!cond) ip = target;
                break;
            }

            // -------- I/O --------
            case OpCode::PRINT: {
                Value v = pop();
                (*out) << valueToString(v) << "\n";
                break;
            }

            // -------- End --------
            case OpCode::HALT:
                return;

            default:
                throw RuntimeError("Unknown opcode.");
        }
    }
}

} // namespace cvm
