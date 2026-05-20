# CVM++

A tiny scripting language that compiles to bytecode and runs on a **stack-based virtual machine**, built from scratch in modern C++17. The full pipeline — lexer, parser, compiler, VM — is about 700 lines of well-commented code.

## The Pipeline

```
source code  →  Lexer  →  tokens
                              ↓
                           Parser
                              ↓
                             AST  →  Compiler  →  bytecode
                                                     ↓
                                                Virtual Machine
                                                     ↓
                                                   output
```

## The Language

```
// integers, booleans, variables
x = 10;
flag = true;

// arithmetic and comparisons
y = x * 2 + 5;
print y < 100;

// if / else
if (x == 10) {
    print x;
} else {
    print 0;
}

// while loops
i = 0;
while (i < 5) {
    print i;
    i = i + 1;
}

// line comments
```

**Types:** integers, booleans.
**Operators:** `+ - * /` (integer), `== < >`, `=` (assignment).
**Statements:** assignment, `print`, `if`/`else`, `while`, `{ }` blocks.

## Build

### With CMake
```
cmake -B build
cmake --build build
./build/bin/cvm tests/sample4_fibonacci.cvm
```

### With Make
```
make
./cvm tests/sample4_fibonacci.cvm
make run       # run all sample programs
```

## Usage

CVM++ has three modes — just like real compilers (Java, Python's `.pyc`, etc.):

```
# Compile and run from source (one-shot):
cvm <script.cvm>

# Compile source to a portable bytecode file:
cvm <script.cvm> -o <out.cvmb>

# Run a previously-compiled bytecode file (no source needed):
cvm --run <out.cvmb>
```

Inspection flags (source mode):

```
--tokens     print the token stream
--ast        print the abstract syntax tree
--bytecode   disassemble compiled bytecode
--trace      trace each VM instruction (to stderr; works in --run mode too)
```

Example: compile on one machine, ship the `.cvmb`, run on another:

```
# Machine A (has the source and compiler):
./cvm myprog.cvm -o myprog.cvmb

# Send myprog.cvmb anywhere — USB, email, scp...
# Machine B (just needs the cvm binary):
./cvm --run myprog.cvmb
```

Example inspection session:
```
./cvm tests/sample1_arithmetic.cvm --tokens --ast --bytecode
```

## Project Structure

```
cvm++/
├── include/
│   ├── token.h       token types + Token struct
│   ├── lexer.h       Lexer class
│   ├── ast.h         AST node hierarchy (visitor pattern)
│   ├── parser.h      recursive descent parser
│   ├── bytecode.h    OpCodes, Value (variant), Chunk
│   ├── compiler.h    AST visitor that emits bytecode
│   └── vm.h          stack-based execution engine
├── src/
│   ├── lexer.cpp
│   ├── parser.cpp
│   ├── compiler.cpp
│   ├── vm.cpp
│   └── main.cpp      CLI driver
├── tests/            sample .cvm programs
├── CMakeLists.txt
├── Makefile
└── README.md
```

## Instruction Set

| OpCode         | Operand         | Effect                                    |
|----------------|-----------------|-------------------------------------------|
| `PUSH_CONST`   | const idx (u16) | push constants[idx]                       |
| `PUSH_TRUE`    | —               | push true                                 |
| `PUSH_FALSE`   | —               | push false                                |
| `POP`          | —               | discard top                               |
| `LOAD`         | slot (u16)      | push variables[slot]                      |
| `STORE`        | slot (u16)      | variables[slot] = pop()                   |
| `ADD/SUB/MUL/DIV` | —            | int binary arithmetic (pop 2, push 1)     |
| `EQ/LT/GT`     | —               | int comparison → push bool                |
| `JMP`          | offset (u16)    | unconditional jump to absolute offset     |
| `JMP_IF_FALSE` | offset (u16)    | pop bool; jump if false                   |
| `PRINT`        | —               | pop and print                             |
| `HALT`         | —               | stop execution                            |

## Extending CVM++

Some fun next steps:

- **Strings** — add a `STRING` token, `StringExpr` node, and store strings in the constant pool. Extend `Value` to include `std::string`.
- **`for` loops** — pure grammar/parser work; desugar to an existing `while`.
- **Functions** — requires call frames. Add `CALL`/`RETURN` opcodes and a frame stack alongside the value stack.
- **More ops** — `!=`, `<=`, `>=`, `%` (modulo), `&&`, `||`, unary `-` and `!`.
- **Better errors** — carry source locations through to runtime errors.
- **Constant folding** — fold `2 + 3` into `5` during compilation.
