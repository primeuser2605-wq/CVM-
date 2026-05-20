#pragma once
//
// bytecode.h
// -----------
// Defines the Instruction Set Architecture (ISA) for CVM++.
// The compiler emits a Chunk of these; the VM consumes them.
//

#include <cstdint>
#include <vector>
#include <string>
#include <variant>
#include <fstream>
#include <stdexcept>

namespace cvm {

// ---------- OpCodes ----------
// Each instruction is one byte. Some take an operand (stored as the next
// byte(s) in the stream) — we'll use a 2-byte operand where needed.
enum class OpCode : uint8_t {
    // --- Stack / constants ---
    PUSH_CONST,   // operand: constant index → push constants[idx]
    PUSH_TRUE,    // push true
    PUSH_FALSE,   // push false
    POP,          // discard top of stack

    // --- Variables ---
    LOAD,         // operand: var slot → push variables[slot]
    STORE,        // operand: var slot → variables[slot] = pop()

    // --- Arithmetic (pop two, push result) ---
    ADD,
    SUB,
    MUL,
    DIV,

    // --- Comparisons ---
    EQ,
    LT,
    GT,

    // --- Control flow (operand: absolute byte offset in code) ---
    JMP,          // unconditional jump
    JMP_IF_FALSE, // jump if top-of-stack is false; always pops

    // --- I/O ---
    PRINT,        // pop and print

    // --- End ---
    HALT
};

// Turn an OpCode into a human-readable name (for disassembly).
inline const char* opName(OpCode op) {
    switch (op) {
        case OpCode::PUSH_CONST:   return "PUSH_CONST";
        case OpCode::PUSH_TRUE:    return "PUSH_TRUE";
        case OpCode::PUSH_FALSE:   return "PUSH_FALSE";
        case OpCode::POP:          return "POP";
        case OpCode::LOAD:         return "LOAD";
        case OpCode::STORE:        return "STORE";
        case OpCode::ADD:          return "ADD";
        case OpCode::SUB:          return "SUB";
        case OpCode::MUL:          return "MUL";
        case OpCode::DIV:          return "DIV";
        case OpCode::EQ:           return "EQ";
        case OpCode::LT:           return "LT";
        case OpCode::GT:           return "GT";
        case OpCode::JMP:          return "JMP";
        case OpCode::JMP_IF_FALSE: return "JMP_IF_FALSE";
        case OpCode::PRINT:        return "PRINT";
        case OpCode::HALT:         return "HALT";
    }
    return "???";
}

// Does this opcode take a 2-byte operand?
inline bool hasOperand(OpCode op) {
    switch (op) {
        case OpCode::PUSH_CONST:
        case OpCode::LOAD:
        case OpCode::STORE:
        case OpCode::JMP:
        case OpCode::JMP_IF_FALSE:
            return true;
        default:
            return false;
    }
}

// ---------- Value ----------
// Our runtime values are either int or bool. std::variant keeps it simple
// and type-safe without us writing a tagged union by hand.
using Value = std::variant<int, bool>;

// Pretty-print a Value (for PRINT and debug).
inline std::string valueToString(const Value& v) {
    if (std::holds_alternative<int>(v))  return std::to_string(std::get<int>(v));
    if (std::holds_alternative<bool>(v)) return std::get<bool>(v) ? "true" : "false";
    return "?";
}

// ---------- Chunk ----------
// A "chunk" is a compiled program: raw bytecode + the constant pool it
// references. For CVM++ one Chunk == one whole program.
struct Chunk {
    std::vector<uint8_t> code;       // the instruction stream
    std::vector<Value>   constants;  // pool of literal values

    // Helpers used by the compiler to emit bytes.
    void writeByte(uint8_t b) { code.push_back(b); }

    void writeOp(OpCode op) { code.push_back(static_cast<uint8_t>(op)); }

    // Write a 16-bit operand in big-endian order.
    void writeShort(uint16_t value) {
        code.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        code.push_back(static_cast<uint8_t>(value & 0xFF));
    }

    // Patch a previously-written 16-bit placeholder at `offset`.
    // Used for forward jumps where we don't yet know the target.
    void patchShort(size_t offset, uint16_t value) {
        code[offset]     = static_cast<uint8_t>((value >> 8) & 0xFF);
        code[offset + 1] = static_cast<uint8_t>(value & 0xFF);
    }

    // Add a constant and return its index.
    int addConstant(Value v) {
        constants.push_back(std::move(v));
        return static_cast<int>(constants.size() - 1);
    }

    // Read a 16-bit operand at ip (for the VM and disassembler).
    static uint16_t readShort(const std::vector<uint8_t>& c, size_t ip) {
        return (static_cast<uint16_t>(c[ip]) << 8) | c[ip + 1];
    }

    // ------------------------------------------------------------------
    // Serialization — save/load bytecode to/from a binary file.
    //
    // File format (binary, big-endian for all multi-byte ints):
    //   Bytes 0..3 : magic 'C','V','M','B'
    //   Byte 4     : format version (currently 1)
    //   Bytes 5..8 : number of constants (uint32)
    //   For each constant:
    //       Byte : tag (0 = int, 1 = bool)
    //       int  : 4 bytes (int32) — only if tag is 0
    //       bool : 1 byte         — only if tag is 1
    //   uint32 : code length
    //   bytes  : code bytes
    // ------------------------------------------------------------------
    void save(const std::string& path) const {
        std::ofstream out(path, std::ios::binary);
        if (!out) throw std::runtime_error("Cannot open '" + path + "' for writing.");

        // Magic + version
        const char magic[4] = {'C', 'V', 'M', 'B'};
        out.write(magic, 4);
        uint8_t version = 1;
        out.write(reinterpret_cast<const char*>(&version), 1);

        // Constants
        writeU32(out, static_cast<uint32_t>(constants.size()));
        for (const auto& v : constants) {
            if (std::holds_alternative<int>(v)) {
                uint8_t tag = 0;
                out.write(reinterpret_cast<const char*>(&tag), 1);
                writeI32(out, std::get<int>(v));
            } else {
                uint8_t tag = 1;
                out.write(reinterpret_cast<const char*>(&tag), 1);
                uint8_t b = std::get<bool>(v) ? 1 : 0;
                out.write(reinterpret_cast<const char*>(&b), 1);
            }
        }

        // Code
        writeU32(out, static_cast<uint32_t>(code.size()));
        out.write(reinterpret_cast<const char*>(code.data()),
                  static_cast<std::streamsize>(code.size()));
    }

    static Chunk load(const std::string& path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) throw std::runtime_error("Cannot open '" + path + "' for reading.");

        // Magic
        char magic[4];
        in.read(magic, 4);
        if (!in || magic[0] != 'C' || magic[1] != 'V' ||
                   magic[2] != 'M' || magic[3] != 'B') {
            throw std::runtime_error("Not a CVM++ bytecode file (bad magic).");
        }

        // Version
        uint8_t version = 0;
        in.read(reinterpret_cast<char*>(&version), 1);
        if (version != 1)
            throw std::runtime_error("Unsupported bytecode version.");

        Chunk chunk;

        // Constants
        uint32_t numConsts = readU32(in);
        chunk.constants.reserve(numConsts);
        for (uint32_t i = 0; i < numConsts; ++i) {
            uint8_t tag = 0;
            in.read(reinterpret_cast<char*>(&tag), 1);
            if (tag == 0) {
                int v = readI32(in);
                chunk.constants.emplace_back(Value{v});
            } else if (tag == 1) {
                uint8_t b = 0;
                in.read(reinterpret_cast<char*>(&b), 1);
                chunk.constants.emplace_back(Value{b != 0});
            } else {
                throw std::runtime_error("Unknown constant tag in bytecode file.");
            }
        }

        // Code
        uint32_t codeLen = readU32(in);
        chunk.code.resize(codeLen);
        if (codeLen > 0) {
            in.read(reinterpret_cast<char*>(chunk.code.data()),
                    static_cast<std::streamsize>(codeLen));
        }

        if (!in) throw std::runtime_error("Unexpected end of bytecode file.");
        return chunk;
    }

private:
    static void writeU32(std::ofstream& out, uint32_t v) {
        uint8_t buf[4] = {
            static_cast<uint8_t>((v >> 24) & 0xFF),
            static_cast<uint8_t>((v >> 16) & 0xFF),
            static_cast<uint8_t>((v >>  8) & 0xFF),
            static_cast<uint8_t>( v        & 0xFF)
        };
        out.write(reinterpret_cast<const char*>(buf), 4);
    }
    static void writeI32(std::ofstream& out, int32_t v) {
        writeU32(out, static_cast<uint32_t>(v));
    }
    static uint32_t readU32(std::ifstream& in) {
        uint8_t buf[4];
        in.read(reinterpret_cast<char*>(buf), 4);
        return (static_cast<uint32_t>(buf[0]) << 24) |
               (static_cast<uint32_t>(buf[1]) << 16) |
               (static_cast<uint32_t>(buf[2]) <<  8) |
                static_cast<uint32_t>(buf[3]);
    }
    static int32_t readI32(std::ifstream& in) {
        return static_cast<int32_t>(readU32(in));
    }
};

} // namespace cvm
