// cvm.js — Browser port of the CVM++ pipeline
// Mirrors the C++ implementation closely so interviewers can see the same
// stages running in JavaScript.

(function (global) {
  'use strict';

  // ==========================================================================
  // TOKEN TYPES (mirror token.h)
  // ==========================================================================
  const TT = {
    NUMBER: 'NUMBER', IDENTIFIER: 'IDENTIFIER',
    IF: 'IF', ELSE: 'ELSE', WHILE: 'WHILE', PRINT: 'PRINT',
    TRUE: 'TRUE', FALSE: 'FALSE',
    PLUS: 'PLUS', MINUS: 'MINUS', STAR: 'STAR', SLASH: 'SLASH',
    ASSIGN: 'ASSIGN', EQ: 'EQ', LT: 'LT', GT: 'GT',
    LPAREN: 'LPAREN', RPAREN: 'RPAREN',
    LBRACE: 'LBRACE', RBRACE: 'RBRACE', SEMICOLON: 'SEMICOLON',
    EOF: 'EOF'
  };

  const KEYWORDS = {
    'if': TT.IF, 'else': TT.ELSE, 'while': TT.WHILE,
    'print': TT.PRINT, 'true': TT.TRUE, 'false': TT.FALSE
  };

  // ==========================================================================
  // LEXER (mirrors lexer.cpp)
  // ==========================================================================
  class LexError extends Error {
    constructor(msg, line) { super(msg); this.line = line; }
  }

  function tokenize(source) {
    const tokens = [];
    let start = 0, current = 0, line = 1;

    const isDigit = c => c >= '0' && c <= '9';
    const isAlpha = c => (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c === '_';
    const isAlnum = c => isDigit(c) || isAlpha(c);
    const atEnd = () => current >= source.length;
    const peek = () => atEnd() ? '\0' : source[current];
    const advance = () => source[current++];
    const match = expected => {
      if (atEnd() || source[current] !== expected) return false;
      current++;
      return true;
    };
    const add = type => tokens.push({
      type,
      lexeme: source.substring(start, current),
      line
    });

    while (!atEnd()) {
      start = current;
      const c = advance();
      switch (c) {
        case '(': add(TT.LPAREN); break;
        case ')': add(TT.RPAREN); break;
        case '{': add(TT.LBRACE); break;
        case '}': add(TT.RBRACE); break;
        case ';': add(TT.SEMICOLON); break;
        case '+': add(TT.PLUS); break;
        case '-': add(TT.MINUS); break;
        case '*': add(TT.STAR); break;
        case '<': add(TT.LT); break;
        case '>': add(TT.GT); break;
        case '=': add(match('=') ? TT.EQ : TT.ASSIGN); break;
        case '/':
          if (match('/')) while (!atEnd() && peek() !== '\n') advance();
          else add(TT.SLASH);
          break;
        case ' ': case '\r': case '\t': break;
        case '\n': line++; break;
        default:
          if (isDigit(c)) {
            while (!atEnd() && isDigit(peek())) advance();
            add(TT.NUMBER);
          } else if (isAlpha(c)) {
            while (!atEnd() && isAlnum(peek())) advance();
            const text = source.substring(start, current);
            add(KEYWORDS[text] || TT.IDENTIFIER);
          } else {
            throw new LexError(`Unexpected character: '${c}'`, line);
          }
      }
    }
    tokens.push({ type: TT.EOF, lexeme: '', line });
    return tokens;
  }

  // ==========================================================================
  // PARSER (mirrors parser.cpp). AST nodes are plain objects.
  // ==========================================================================
  class ParseError extends Error {
    constructor(msg, line) { super(msg); this.line = line; }
  }

  function parse(tokens) {
    let cur = 0;
    const peek = () => tokens[cur];
    const previous = () => tokens[cur - 1];
    const atEnd = () => peek().type === TT.EOF;
    const check = t => !atEnd() && peek().type === t;
    const advance = () => { if (!atEnd()) cur++; return previous(); };
    const matchAny = types => {
      for (const t of types) if (check(t)) { advance(); return true; }
      return false;
    };
    const consume = (t, msg) => {
      if (check(t)) return advance();
      throw new ParseError(`${msg} (got '${peek().lexeme || peek().type}')`, peek().line);
    };

    function parseProgram() {
      const stmts = [];
      while (!atEnd()) stmts.push(parseStatement());
      return stmts;
    }

    function parseStatement() {
      if (check(TT.IF)) return parseIf();
      if (check(TT.WHILE)) return parseWhile();
      if (check(TT.PRINT)) return parsePrint();
      if (check(TT.LBRACE)) return parseBlock();
      return parseAssignment();
    }

    function parseAssignment() {
      const name = consume(TT.IDENTIFIER, 'Expected variable name');
      consume(TT.ASSIGN, "Expected '='");
      const value = parseExpression();
      consume(TT.SEMICOLON, "Expected ';'");
      return { kind: 'Assign', name: name.lexeme, value };
    }

    function parsePrint() {
      consume(TT.PRINT, "Expected 'print'");
      const value = parseExpression();
      consume(TT.SEMICOLON, "Expected ';'");
      return { kind: 'Print', value };
    }

    function parseIf() {
      consume(TT.IF, "Expected 'if'");
      consume(TT.LPAREN, "Expected '('");
      const condition = parseExpression();
      consume(TT.RPAREN, "Expected ')'");
      const thenBranch = parseBlock();
      let elseBranch = null;
      if (matchAny([TT.ELSE])) elseBranch = parseBlock();
      return { kind: 'If', condition, thenBranch, elseBranch };
    }

    function parseWhile() {
      consume(TT.WHILE, "Expected 'while'");
      consume(TT.LPAREN, "Expected '('");
      const condition = parseExpression();
      consume(TT.RPAREN, "Expected ')'");
      const body = parseBlock();
      return { kind: 'While', condition, body };
    }

    function parseBlock() {
      consume(TT.LBRACE, "Expected '{'");
      const stmts = [];
      while (!check(TT.RBRACE) && !atEnd()) stmts.push(parseStatement());
      consume(TT.RBRACE, "Expected '}'");
      return { kind: 'Block', statements: stmts };
    }

    function parseExpression() { return parseComparison(); }

    function parseComparison() {
      let left = parseTerm();
      if (matchAny([TT.EQ, TT.LT, TT.GT])) {
        const op = previous().type;
        const right = parseTerm();
        left = { kind: 'Binary', op, left, right };
      }
      return left;
    }

    function parseTerm() {
      let left = parseFactor();
      while (matchAny([TT.PLUS, TT.MINUS])) {
        const op = previous().type;
        const right = parseFactor();
        left = { kind: 'Binary', op, left, right };
      }
      return left;
    }

    function parseFactor() {
      let left = parsePrimary();
      while (matchAny([TT.STAR, TT.SLASH])) {
        const op = previous().type;
        const right = parsePrimary();
        left = { kind: 'Binary', op, left, right };
      }
      return left;
    }

    function parsePrimary() {
      if (matchAny([TT.NUMBER])) {
        return { kind: 'Number', value: parseInt(previous().lexeme, 10) };
      }
      if (matchAny([TT.TRUE])) return { kind: 'Bool', value: true };
      if (matchAny([TT.FALSE])) return { kind: 'Bool', value: false };
      if (matchAny([TT.IDENTIFIER])) return { kind: 'Var', name: previous().lexeme };
      if (matchAny([TT.LPAREN])) {
        const e = parseExpression();
        consume(TT.RPAREN, "Expected ')'");
        return e;
      }
      throw new ParseError(`Unexpected token '${peek().lexeme || peek().type}'`, peek().line);
    }

    return parseProgram();
  }

  // ==========================================================================
  // AST PRETTY PRINTER
  // ==========================================================================
  function astString(program) {
    const lines = [];
    const emit = (depth, s) => lines.push('  '.repeat(depth) + s);

    function visit(node, depth) {
      switch (node.kind) {
        case 'Assign':
          emit(depth, `Assign(${node.name})`);
          visit(node.value, depth + 1);
          break;
        case 'Print':
          emit(depth, 'Print');
          visit(node.value, depth + 1);
          break;
        case 'If':
          emit(depth, 'If');
          emit(depth + 1, 'cond:');
          visit(node.condition, depth + 2);
          emit(depth + 1, 'then:');
          visit(node.thenBranch, depth + 2);
          if (node.elseBranch) {
            emit(depth + 1, 'else:');
            visit(node.elseBranch, depth + 2);
          }
          break;
        case 'While':
          emit(depth, 'While');
          emit(depth + 1, 'cond:');
          visit(node.condition, depth + 2);
          emit(depth + 1, 'body:');
          visit(node.body, depth + 2);
          break;
        case 'Block':
          emit(depth, 'Block');
          for (const s of node.statements) visit(s, depth + 1);
          break;
        case 'Number': emit(depth, `Number(${node.value})`); break;
        case 'Bool': emit(depth, `Bool(${node.value})`); break;
        case 'Var': emit(depth, `Var(${node.name})`); break;
        case 'Binary':
          emit(depth, `Binary(${node.op})`);
          visit(node.left, depth + 1);
          visit(node.right, depth + 1);
          break;
      }
    }
    for (const s of program) visit(s, 0);
    return lines.join('\n');
  }

  // ==========================================================================
  // BYTECODE & COMPILER (mirrors bytecode.h + compiler.cpp)
  // ==========================================================================
  const OP = {
    PUSH_CONST: 0, PUSH_TRUE: 1, PUSH_FALSE: 2, POP: 3,
    LOAD: 4, STORE: 5,
    ADD: 6, SUB: 7, MUL: 8, DIV: 9,
    EQ: 10, LT: 11, GT: 12,
    JMP: 13, JMP_IF_FALSE: 14,
    PRINT: 15, HALT: 16
  };
  const OP_NAME = Object.fromEntries(Object.entries(OP).map(([k, v]) => [v, k]));
  const HAS_OPERAND = new Set([OP.PUSH_CONST, OP.LOAD, OP.STORE, OP.JMP, OP.JMP_IF_FALSE]);

  class CompileError extends Error {}

  function compile(program) {
    const code = [];
    const constants = [];
    const variables = new Map();

    const writeByte = b => code.push(b & 0xff);
    const writeOp = op => writeByte(op);
    const writeShort = v => { writeByte((v >> 8) & 0xff); writeByte(v & 0xff); };
    const patchShort = (offset, v) => {
      code[offset] = (v >> 8) & 0xff;
      code[offset + 1] = v & 0xff;
    };
    const addConstant = v => { constants.push(v); return constants.length - 1; };

    function getOrCreateSlot(name) {
      if (variables.has(name)) return variables.get(name);
      const slot = variables.size;
      variables.set(name, slot);
      return slot;
    }

    function emitJump(op) {
      writeOp(op);
      const offset = code.length;
      writeShort(0xffff);
      return offset;
    }
    function patchJump(offset) { patchShort(offset, code.length); }

    function compileExpr(e) {
      switch (e.kind) {
        case 'Number': {
          const idx = addConstant({ type: 'int', value: e.value });
          writeOp(OP.PUSH_CONST);
          writeShort(idx);
          break;
        }
        case 'Bool':
          writeOp(e.value ? OP.PUSH_TRUE : OP.PUSH_FALSE);
          break;
        case 'Var': {
          if (!variables.has(e.name))
            throw new CompileError(`Undefined variable '${e.name}'`);
          writeOp(OP.LOAD);
          writeShort(variables.get(e.name));
          break;
        }
        case 'Binary': {
          compileExpr(e.left);
          compileExpr(e.right);
          const map = {
            [TT.PLUS]: OP.ADD, [TT.MINUS]: OP.SUB,
            [TT.STAR]: OP.MUL, [TT.SLASH]: OP.DIV,
            [TT.EQ]: OP.EQ, [TT.LT]: OP.LT, [TT.GT]: OP.GT
          };
          writeOp(map[e.op]);
          break;
        }
      }
    }

    function compileStmt(s) {
      switch (s.kind) {
        case 'Assign':
          compileExpr(s.value);
          writeOp(OP.STORE);
          writeShort(getOrCreateSlot(s.name));
          break;
        case 'Print':
          compileExpr(s.value);
          writeOp(OP.PRINT);
          break;
        case 'Block':
          for (const stmt of s.statements) compileStmt(stmt);
          break;
        case 'If': {
          compileExpr(s.condition);
          const jumpToElse = emitJump(OP.JMP_IF_FALSE);
          compileStmt(s.thenBranch);
          const jumpToEnd = emitJump(OP.JMP);
          patchJump(jumpToElse);
          if (s.elseBranch) compileStmt(s.elseBranch);
          patchJump(jumpToEnd);
          break;
        }
        case 'While': {
          const loopStart = code.length;
          compileExpr(s.condition);
          const exitJump = emitJump(OP.JMP_IF_FALSE);
          compileStmt(s.body);
          writeOp(OP.JMP);
          writeShort(loopStart);
          patchJump(exitJump);
          break;
        }
      }
    }

    for (const stmt of program) compileStmt(stmt);
    writeOp(OP.HALT);
    return { code, constants };
  }

  // ==========================================================================
  // DISASSEMBLER
  // ==========================================================================
  function disassemble(chunk) {
    const lines = [];
    lines.push('-- constants --');
    chunk.constants.forEach((c, i) => {
      const v = c.type === 'int' ? c.value : (c.value ? 'true' : 'false');
      lines.push(`  [${i}] ${v}`);
    });
    lines.push('-- code --');
    let ip = 0;
    while (ip < chunk.code.length) {
      const ipStr = String(ip).padStart(4, '0');
      const op = chunk.code[ip];
      const name = OP_NAME[op] || '???';
      let row = `${ipStr}  ${name.padEnd(14, ' ')}`;
      ip++;
      if (HAS_OPERAND.has(op)) {
        const operand = (chunk.code[ip] << 8) | chunk.code[ip + 1];
        row += operand;
        if (op === OP.PUSH_CONST && operand < chunk.constants.length) {
          const c = chunk.constants[operand];
          const v = c.type === 'int' ? c.value : (c.value ? 'true' : 'false');
          row += `   ; ${v}`;
        }
        ip += 2;
      }
      lines.push(row);
    }
    return lines.join('\n');
  }

  // ==========================================================================
  // VM (mirrors vm.cpp)
  // ==========================================================================
  class RuntimeError extends Error {}

  function run(chunk, opts) {
    opts = opts || {};
    const out = [];
    const stack = [];
    const variables = new Array(64).fill({ type: 'int', value: 0 });
    const code = chunk.code;
    let ip = 0;
    let steps = 0;
    const STEP_LIMIT = opts.stepLimit || 100000;
    const trace = opts.trace || false;
    const traceLines = [];

    const push = v => stack.push(v);
    const pop = () => {
      if (stack.length === 0) throw new RuntimeError('Stack underflow');
      return stack.pop();
    };
    const popInt = () => {
      const v = pop();
      if (v.type !== 'int') throw new RuntimeError('Expected int');
      return v.value;
    };
    const popBool = () => {
      const v = pop();
      if (v.type !== 'bool') throw new RuntimeError('Expected bool');
      return v.value;
    };
    const valStr = v => v.type === 'int' ? String(v.value) : (v.value ? 'true' : 'false');
    const readShort = i => (code[i] << 8) | code[i + 1];

    while (ip < code.length) {
      if (steps++ > STEP_LIMIT) throw new RuntimeError('Step limit exceeded (possible infinite loop)');
      if (trace) {
        traceLines.push(`[ip=${String(ip).padStart(4,'0')} stack=${stack.map(valStr).join(' ')}] ${OP_NAME[code[ip]]}`);
      }
      const op = code[ip++];
      switch (op) {
        case OP.PUSH_CONST: {
          const idx = readShort(ip); ip += 2;
          push(chunk.constants[idx]);
          break;
        }
        case OP.PUSH_TRUE: push({ type: 'bool', value: true }); break;
        case OP.PUSH_FALSE: push({ type: 'bool', value: false }); break;
        case OP.POP: pop(); break;
        case OP.LOAD: {
          const slot = readShort(ip); ip += 2;
          push(variables[slot]);
          break;
        }
        case OP.STORE: {
          const slot = readShort(ip); ip += 2;
          variables[slot] = pop();
          break;
        }
        case OP.ADD: { const b = popInt(); const a = popInt(); push({ type: 'int', value: a + b }); break; }
        case OP.SUB: { const b = popInt(); const a = popInt(); push({ type: 'int', value: a - b }); break; }
        case OP.MUL: { const b = popInt(); const a = popInt(); push({ type: 'int', value: a * b }); break; }
        case OP.DIV: {
          const b = popInt(); const a = popInt();
          if (b === 0) throw new RuntimeError('Division by zero');
          push({ type: 'int', value: Math.trunc(a / b) });
          break;
        }
        case OP.EQ: { const b = popInt(); const a = popInt(); push({ type: 'bool', value: a === b }); break; }
        case OP.LT: { const b = popInt(); const a = popInt(); push({ type: 'bool', value: a < b }); break; }
        case OP.GT: { const b = popInt(); const a = popInt(); push({ type: 'bool', value: a > b }); break; }
        case OP.JMP: ip = readShort(ip); break;
        case OP.JMP_IF_FALSE: {
          const target = readShort(ip); ip += 2;
          if (!popBool()) ip = target;
          break;
        }
        case OP.PRINT: out.push(valStr(pop())); break;
        case OP.HALT: return { output: out, trace: traceLines };
        default: throw new RuntimeError(`Unknown opcode ${op}`);
      }
    }
    return { output: out, trace: traceLines };
  }

  // ==========================================================================
  // Public API
  // ==========================================================================
  global.CVM = {
    tokenize, parse, compile, disassemble, run, astString,
    LexError, ParseError, CompileError, RuntimeError,
    OP_NAME
  };
})(window);
