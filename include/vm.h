#pragma once
//
// vm.h
// -----
// A stack-based Virtual Machine that executes a compiled Chunk.
// Holds three pieces of state:
//   - an instruction pointer (ip) into chunk.code
//   - an operand stack
//   - a flat array of variable slots
//

#include "bytecode.h"
#include <vector>
#include <stdexcept>
#include <iosfwd>

namespace cvm {

class RuntimeError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class VM {
public:
    // trace=true prints every instruction before executing it.
    // out is where PRINT writes (defaults to std::cout).
    void run(const Chunk& chunk, bool trace = false, std::ostream* out = nullptr);

private:
    std::vector<Value> stack_;
    std::vector<Value> variables_;  // indexed by slot number

    // --- Stack helpers ---
    void  push(Value v)   { stack_.push_back(std::move(v)); }
    Value pop();
    Value& top();

    // --- Typed pop helpers ---
    int  popInt();
    bool popBool();

    // --- Arithmetic / comparison primitives ---
    void binaryIntOp(OpCode op);
    void comparisonOp(OpCode op);
};

} // namespace cvm
