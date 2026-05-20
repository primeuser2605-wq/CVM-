# CVM++ — Project Report

**A Stack-Based Virtual Machine and Custom Compiler in C++**

---

## 1. Problem Statement

Most developers use high-level languages like Python, JavaScript, or Java without ever asking *how a computer actually understands the text we type*. The compiler, the bytecode, the runtime — these feel like magic.

This project demystifies that magic by building a **complete language toolchain from scratch**: a custom scripting language is designed, lexed, parsed, compiled to a proprietary bytecode, and executed on a custom-built, stack-based Virtual Machine — all in modern C++17, with zero external dependencies.

## 2. Goals (from the problem statement)

| # | Goal | Status |
|---|---|---|
| 1 | Design a simple language grammar and an Instruction Set Architecture (ISA) | ✅ Done |
| 2 | Build a Lexer to convert raw source code strings into Tokens | ✅ Done |
| 3 | Build a Parser to arrange Tokens into an Abstract Syntax Tree (AST) | ✅ Done |
| 4 | Implement a Compiler to flatten the AST into bytecode | ✅ Done |
| 5 | Build a Virtual Machine (VM) with a stack-based execution loop | ✅ Done |

## 3. Architecture Overview

```
   ┌──────────────┐    ┌────────┐    ┌────────┐    ┌──────────┐    ┌────────┐
   │ Source (.cvm)│ ──▶│ Lexer  │──▶ │ Parser │──▶ │ Compiler │──▶ │   VM   │ ──▶ Output
   └──────────────┘    └────────┘    └────────┘    └──────────┘    └────────┘
                          │             │              │               │
                          ▼             ▼              ▼               ▼
                        tokens         AST          bytecode      stack-based
                                                                  execution
```

Every stage is **pure** (no side effects on previous stages) and inspectable via CLI flags (`--tokens`, `--ast`, `--bytecode`, `--trace`).

## 4. The Language

CVM++ scripts (`.cvm` files) support:

| Feature | Syntax |
|---|---|
| Integer literals | `42`, `0`, `100` |
| Boolean literals | `true`, `false` |
| Variables | `x = 10;` |
| Arithmetic | `+`, `-`, `*`, `/` |
| Comparisons | `==`, `<`, `>` |
| If / else | `if (cond) { ... } else { ... }` |
| While loop | `while (cond) { ... }` |
| Print | `print expr;` |
| Comments | `// ...` |
| Grouping | `( expr )` |

### Formal Grammar (BNF-ish)

```
program     → statement*
statement   → assignment | if_stmt | while_stmt | print_stmt | block
assignment  → IDENTIFIER "=" expression ";"
if_stmt     → "if" "(" expression ")" block ("else" block)?
while_stmt  → "while" "(" expression ")" block
print_stmt  → "print" expression ";"
block       → "{" statement* "}"

expression  → comparison
comparison  → term (("==" | "<" | ">") term)?
term        → factor (("+" | "-") factor)*
factor      → primary (("*" | "/") primary)*
primary     → NUMBER | IDENTIFIER | "true" | "false" | "(" expression ")"
```

**Operator precedence is baked directly into the grammar:** `factor` (which handles `*`/`/`) is called from inside `term` (which handles `+`/`-`), so multiplication automatically binds tighter than addition.

## 5. Implementation

### 5.1 The Lexer (`lexer.cpp`)

A one-pass, no-backtracking scanner. For each token:

1. Skip whitespace and comments
2. Look at the current character
3. Dispatch by category (single-char operator? digit? letter?)
4. Consume the relevant characters, emit a `Token`

Key techniques:

- **Peek-ahead** to distinguish `=` from `==`
- **"Scan-then-classify"** for identifiers: scan any alphanumeric run, then look up in the keyword table to see if it's actually `if`, `while`, etc.
- **Line tracking** for error messages (incremented on every `\n`)

The result is a `std::vector<Token>` terminated by `END_OF_FILE`.

### 5.2 The Parser (`parser.cpp`)

A textbook **recursive descent parser**. Every grammar rule becomes one C++ method:

| Grammar Rule | Method |
|---|---|
| `program` | `parseProgram()` |
| `statement` | `parseStatement()` |
| `expression` → `comparison` | `parseExpression()` |
| `comparison` → `term ((==|<|>) term)?` | `parseComparison()` |
| `term` → `factor ((+|-) factor)*` | `parseTerm()` |
| `factor` → `primary ((*|/) primary)*` | `parseFactor()` |
| `primary` | `parsePrimary()` |

Each call returns an `ExprPtr` or `StmtPtr` (a `std::unique_ptr` to an AST node). Ownership of children sits naturally inside parents — when the root is destroyed, the whole tree is too.

**Example:** parsing `2 + 3 * 4` produces this tree:

```
Binary(PLUS)
├── Number(2)
└── Binary(STAR)
    ├── Number(3)
    └── Number(4)
```

The `*` is deeper in the tree, so it evaluates first. Precedence comes for free.

### 5.3 The AST (`ast.h`)

Two parallel hierarchies:

- **`Expr`** — things that produce a value: `NumberExpr`, `BoolExpr`, `VarExpr`, `BinaryExpr`
- **`Stmt`** — things that do something: `AssignStmt`, `PrintStmt`, `IfStmt`, `WhileStmt`, `BlockStmt`

Both implement the **Visitor pattern**, letting new operations (pretty-printer, compiler, a future interpreter) be added without modifying the node classes themselves.

### 5.4 The Compiler (`compiler.cpp`)

A visitor over the AST that emits bytecode into a **`Chunk`**:

```cpp
struct Chunk {
    std::vector<uint8_t> code;       // the instruction stream
    std::vector<Value>   constants;  // pool of literals
};
```

Highlights:

- **Variables** get integer slot numbers assigned on first assignment (a simple symbol table).
- **Operands order matters:** for `a + b` we emit `a` first, then `b`, so when the VM pops it gets `b` first — correct for non-commutative operations like subtraction and division.
- **Forward jumps use back-patching.** For an `if`, we emit `JMP_IF_FALSE` with a placeholder operand, compile the *then* branch, then patch the placeholder with the actual byte offset of the *else* branch. Same trick for `while`'s exit jump.

```
   if (cond) { THEN } else { ELSE }
       ┌──────────────┐
       │ <cond bytes> │
       │ JMP_IF_FALSE │──┐
       │ <then bytes> │  │
       │ JMP          │──┼─┐
       │ <else bytes> │◀─┘ │
       │ ...          │◀───┘
```

### 5.5 The Virtual Machine (`vm.cpp`)

A classic **fetch–decode–execute loop**:

```
while (ip < code.size()):
    op = code[ip++]
    switch (op):
        case PUSH_CONST: ...
        case ADD:        ...
        case JMP:        ip = operand
        case HALT:       return
```

State the VM holds:

| Field | Purpose |
|---|---|
| `ip` | instruction pointer (offset into `chunk.code`) |
| `stack_` | operand stack (`std::vector<Value>`) |
| `variables_` | flat slot array, indexed by variable slot number |

Values are `std::variant<int, bool>` — type-safe, no manual tagged unions.

### 5.6 The CLI (`main.cpp`)

```
cvm <script.cvm> [--tokens] [--ast] [--bytecode] [--trace]
```

All inspection flags can be combined, making it easy to see how source code flows through each stage.

## 6. The Instruction Set Architecture (ISA)

Each instruction is a single byte; instructions that need a parameter store a **2-byte big-endian operand** immediately after the opcode.

| OpCode | Operand | Effect |
|---|---|---|
| `PUSH_CONST` | const index | push `constants[idx]` |
| `PUSH_TRUE` | — | push `true` |
| `PUSH_FALSE` | — | push `false` |
| `POP` | — | discard top |
| `LOAD` | var slot | push `variables[slot]` |
| `STORE` | var slot | `variables[slot] = pop()` |
| `ADD` / `SUB` / `MUL` / `DIV` | — | pop 2 ints, push result |
| `EQ` / `LT` / `GT` | — | pop 2 ints, push bool |
| `JMP` | code offset | unconditional jump |
| `JMP_IF_FALSE` | code offset | pop bool; jump if false |
| `PRINT` | — | pop and print |
| `HALT` | — | stop execution |

A total of **17 opcodes** — small enough to keep mental, expressive enough to compile our whole language.

## 7. Worked Example

Source:
```
x = 2 + 3 * 4;
print x;
```

Tokens:
```
IDENTIFIER(x) ASSIGN NUMBER(2) PLUS NUMBER(3) STAR NUMBER(4) SEMICOLON
PRINT IDENTIFIER(x) SEMICOLON EOF
```

AST:
```
Assign(x)
  Binary(PLUS)
    Number(2)
    Binary(STAR)
      Number(3)
      Number(4)
Print
  Var(x)
```

Bytecode (offsets shown in decimal):
```
0   PUSH_CONST  0   ; 2
3   PUSH_CONST  1   ; 3
6   PUSH_CONST  2   ; 4
9   MUL
10  ADD
11  STORE       0
14  LOAD        0
17  PRINT
18  HALT
```

VM stack trace (from `--trace`):
```
[stack=        ]  PUSH_CONST  →  [2]
[stack=2       ]  PUSH_CONST  →  [2, 3]
[stack=2 3     ]  PUSH_CONST  →  [2, 3, 4]
[stack=2 3 4   ]  MUL         →  [2, 12]
[stack=2 12    ]  ADD         →  [14]
[stack=14      ]  STORE       →  [], vars[0] = 14
[stack=        ]  LOAD        →  [14]
[stack=14      ]  PRINT       →  prints "14"
[stack=        ]  HALT
```

The output is `14` — matching the standard precedence rules.

## 8. Test Results

All seven sample programs were run end-to-end:

| Sample | What it tests | Expected | Actual |
|---|---|---|---|
| `sample1_arithmetic.cvm` | precedence, parens, all 4 ops | 14, 6, 40, 2, 14, 20 | ✅ matches |
| `sample2_control.cvm` | if/else, nested if, bool literals | 1, 42, 7, 1, 2 | ✅ matches |
| `sample3_loops.cvm` | while, accumulator, factorial | 1..5, 55, 720 | ✅ matches |
| `sample4_fibonacci.cvm` | classic Fibonacci, 10 terms | 0,1,1,2,3,5,8,13,21,34 | ✅ matches |
| `sample5_primes.cvm` | nested while, trial division | primes < 30 | ✅ matches |
| `demo_calculator.cvm` | calculator (24, 6) + 24^6 | last line 191102976 | ✅ matches |
| `demo_truth_machine.cvm` | classic truth machine | `1` ten times | ✅ matches |

All programs compile cleanly with `-Wall -Wextra -Wpedantic` on g++ 13.

## 9. Project Statistics

| Metric | Value |
|---|---|
| Total lines of source (incl. comments) | ~1,500 |
| Approximate lines of pure code | ~700 |
| Header files | 7 |
| Source files | 5 |
| Sample programs | 7 |
| Opcodes | 17 |
| External dependencies | 0 |
| C++ standard | C++17 |

## 10. Design Decisions & Trade-offs

- **Integer-only arithmetic.** `Value = variant<int, bool>` is intentionally narrow — adding floats means more Value variants and dispatch in every arithmetic opcode. Keeping it integer-only kept the VM dispatch a single `switch`.
- **A single global variable scope.** Variables are looked up in one flat slot array. Real languages have lexical scoping with stack-allocated locals — a worthwhile extension once functions exist.
- **No JIT, no register allocation.** Every value goes through the operand stack, which makes the VM slower than a register VM but enormously simpler. Optimisation was explicitly not a goal.
- **`std::variant` over a custom tagged union.** Slightly heavier than a hand-rolled union, but pays off in type safety and zero risk of undefined-behaviour bugs from reading the wrong union arm.
- **Visitor pattern for the AST.** Adds boilerplate, but cleanly separates the AST shape from operations on it. The pretty-printer and compiler share the same node classes.

## 11. Possible Extensions

- **Strings.** Add `STRING` to the Value variant, a string literal in the lexer, and string-aware `ADD` for concatenation.
- **`for` loops.** Pure parser work — desugar `for (init; cond; step) body` to existing `while`.
- **Functions.** Adds `CALL` and `RETURN` opcodes, a call frame stack, and lexical scoping for locals.
- **More operators.** `!=`, `<=`, `>=`, `%`, `&&`, `||`, unary `-` and `!`.
- **Constant folding.** Fold pure expressions (`2 + 3`) at compile time — a one-pass optimiser over the AST.
- **Better error messages.** Carry source line info into bytecode so runtime errors can pinpoint the original line.

## 12. Conclusion

CVM++ is a complete, working, end-to-end language toolchain in roughly 700 lines of modern C++. Every layer — lexer, parser, AST, bytecode, VM — is small enough to read in a sitting, but together they handle real programs: factorials, Fibonacci sequences, prime sieves, and the truth machine.

The most satisfying takeaway: there is no magic. Each step is a small, mechanical transformation. Once you've built one, every other compiler you encounter starts to look familiar.
