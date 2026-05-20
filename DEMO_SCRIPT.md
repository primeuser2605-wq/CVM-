# Demo Video Script — CVM++

**Length:** ~3-4 minutes
**Tools needed:** any screen recorder (OBS, ShareX, QuickTime, the Xbox Game Bar on Windows, `asciinema` for terminal-only)
**Setup:** open a terminal in the `cvm++/` directory; make the font large enough to read

---

## SHOT 1 — Show the project (15 s)

**Run:**
```bash
ls
tree -L 2 || find . -maxdepth 2 -type f
```

**Say:**
> "This is CVM++ — a small scripting language with its own compiler and stack-based virtual machine, written in modern C++. Here's the layout: headers in `include/`, implementations in `src/`, sample programs in `tests/`."

---

## SHOT 2 — Build it (15 s)

**Run:**
```bash
make clean
make
```

**Say:**
> "One command to build. It compiles five source files into a single executable called `cvm`. No external libraries needed."

---

## SHOT 3 — Show the demo program (30 s)

**Run:**
```bash
cat tests/demo_calculator.cvm
```

**Say:**
> "Here's our demo program — a calculator. It uses every feature of the language: integer arithmetic, operator precedence, variables, comparisons producing booleans, an if-else branch, and a while loop that computes 24 to the 6th power."

---

## SHOT 4 — Run it (15 s)

**Run:**
```bash
./cvm tests/demo_calculator.cvm
```

**Expected output:**
```
=== OUTPUT ===
24
6
30
18
144
4
36
60
true
false
false
1
191102976
```

**Say:**
> "And there's the output. Notice line 6 shows `36` while line 7 shows `60` — that's operator precedence working: `a + b * 2` versus `(a + b) * 2`. The last line, `191102976`, is 24 to the 6th, computed by the while loop."

---

## SHOT 5 — Show how compilation works — Tokens (30 s)

**Run:**
```bash
./cvm tests/demo_calculator.cvm --tokens 2>&1 | head -25
```

**Say:**
> "Now let's see what's happening internally. With `--tokens`, the lexer prints every token it produces. You can see identifiers, keywords like `if` and `print`, operators, and numbers — each tagged with its line number."

---

## SHOT 6 — Show the AST (30 s)

**Run:**
```bash
./cvm tests/demo_calculator.cvm --ast 2>&1 | head -30
```

**Say:**
> "With `--ast`, the parser shows us the Abstract Syntax Tree it built. Notice how `a + b * 2` becomes a `Binary(PLUS)` node with `Var(a)` on one side and a *nested* `Binary(STAR)` on the other — multiplication is deeper in the tree, so it gets evaluated first."

---

## SHOT 7 — Show the bytecode (30 s)

**Run:**
```bash
./cvm tests/demo_calculator.cvm --bytecode 2>&1 | head -30
```

**Say:**
> "With `--bytecode`, we see the compiled instructions our virtual machine actually runs. It's like assembly — `PUSH_CONST` puts a constant on the stack, `MUL` and `ADD` pop two values and push the result, `STORE` saves to a variable, `PRINT` outputs. Compact and easy to read."

---

## SHOT 8 — The Truth Machine (45 s)

**Run:**
```bash
cat tests/demo_truth_machine.cvm
```

**Say:**
> "Now the classic Truth Machine. If the input is 0, print 0 once and halt. If the input is 1, print 1 forever. We cap it at ten iterations so the demo finishes."

**Run:**
```bash
./cvm tests/demo_truth_machine.cvm
```

**Expected:** ten `1`s.

**Say:**
> "Input is `1`, so we get ten ones. Let me flip it."

**Run:** (edit the file or use sed inline)
```bash
sed -i 's/input = 1;/input = 0;/' tests/demo_truth_machine.cvm
./cvm tests/demo_truth_machine.cvm
sed -i 's/input = 0;/input = 1;/' tests/demo_truth_machine.cvm   # restore
```

**Say:**
> "Now input is `0`, and we get a single `0`. The if-else branch and the while loop both work."

---

## SHOT 9 — Trace mode (30 s)

**Run:**
```bash
./cvm tests/demo_truth_machine.cvm --trace 2>&1 | head -30
```

**Say:**
> "Finally, with `--trace`, the VM prints every instruction *before* it executes, along with the current stack contents. You can literally watch values being pushed onto the stack, operations consuming them, and results being pushed back. This is how a stack-based virtual machine actually runs."

---

## SHOT 10 — Wrap up (10 s)

**Run:**
```bash
wc -l include/*.h src/*.cpp
```

**Say:**
> "About 1,500 lines of source — call it 700 lines of code plus comments — for a complete lexer, parser, compiler, and virtual machine in pure C++17. That's CVM++."

---

# Easy alternative: terminal-only recording with asciinema

If you don't want to mess with screen recorders, asciinema records the terminal itself and produces a shareable web link:

```bash
# Install (Linux): sudo apt install asciinema
# Install (macOS): brew install asciinema

asciinema rec demo.cast
# ... run the commands above ...
# Ctrl-D to stop

asciinema play demo.cast      # play back locally
asciinema upload demo.cast    # get a shareable link
```

Or convert to GIF / MP4 with `agg` (asciinema gif generator) afterward.
