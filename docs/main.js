// main.js — wires up the live demo
(function () {
  'use strict';

  const SAMPLES = {
    fib:
`// First 10 Fibonacci numbers
a = 0;
b = 1;
i = 0;
while (i < 10) {
    print a;
    t = a + b;
    a = b;
    b = t;
    i = i + 1;
}`,

    fact:
`// Factorial of 6
n = 6;
f = 1;
while (n > 1) {
    f = f * n;
    n = n - 1;
}
print f;`,

    prec:
`// Operator precedence
print 2 + 3 * 4;        // 14
print (2 + 3) * 4;      // 20
print 10 - 2 - 3;       // 5  (left-associative)
print 100 / 5 / 2;      // 10`,

    primes:
`// Primes below 30 by trial division
n = 2;
while (n < 30) {
    isPrime = 1;
    d = 2;
    while (d * d < n + 1) {
        q = n / d;
        if (q * d == n) {
            isPrime = 0;
        }
        d = d + 1;
    }
    if (isPrime == 1) {
        print n;
    }
    n = n + 1;
}`,

    truth:
`// The classic truth machine
// 0 → print 0 once; 1 → loop forever (capped at 10)
input = 1;

if (input == 0) {
    print 0;
} else {
    count = 0;
    while (count < 10) {
        print 1;
        count = count + 1;
    }
}`
  };

  // ----- DOM refs -----
  const $source       = document.getElementById('source');
  const $tokensOut    = document.getElementById('tokensOut');
  const $astOut       = document.getElementById('astOut');
  const $bytecodeOut  = document.getElementById('bytecodeOut');
  const $output       = document.getElementById('output');
  const $lineCount    = document.getElementById('lineCount');
  const $tokenCount   = document.getElementById('tokenCount');
  const $astCount     = document.getElementById('astCount');
  const $codeSize     = document.getElementById('codeSize');
  const $execStatus   = document.getElementById('execStatus');

  // ----- Sample buttons -----
  document.querySelectorAll('.sample-btn').forEach(btn => {
    btn.addEventListener('click', () => {
      const id = btn.dataset.sample;
      if (SAMPLES[id]) {
        $source.value = SAMPLES[id];
        runPipeline();
      }
    });
  });

  // ----- The live pipeline -----
  let debounceTimer = null;

  function runPipeline() {
    const src = $source.value;

    // Line count
    const lines = src.split('\n').length;
    $lineCount.textContent = `${lines} line${lines === 1 ? '' : 's'}`;

    // Empty input → clear all panes
    if (!src.trim()) {
      $tokensOut.textContent = '';
      $astOut.textContent = '';
      $bytecodeOut.textContent = '';
      $output.textContent = '';
      $tokenCount.textContent = '0';
      $astCount.textContent = '—';
      $codeSize.textContent = '0 bytes';
      setStatus('idle', '');
      return;
    }

    // --- Stage 1: lex ---
    let tokens;
    try {
      tokens = CVM.tokenize(src);
    } catch (e) {
      const msg = `lex error on line ${e.line}: ${e.message}`;
      $tokensOut.textContent = msg;
      $tokensOut.classList.add('error');
      $astOut.textContent = '';
      $bytecodeOut.textContent = '';
      $output.textContent = '';
      setStatus('error', 'lex failed');
      return;
    }
    $tokensOut.classList.remove('error');
    $tokensOut.textContent = formatTokens(tokens);
    $tokenCount.textContent = tokens.length;

    // --- Stage 2: parse ---
    let ast;
    try {
      ast = CVM.parse(tokens);
    } catch (e) {
      const msg = `parse error on line ${e.line}: ${e.message}`;
      $astOut.textContent = msg;
      $astOut.classList.add('error');
      $bytecodeOut.textContent = '';
      $output.textContent = '';
      setStatus('error', 'parse failed');
      return;
    }
    $astOut.classList.remove('error');
    $astOut.textContent = CVM.astString(ast);
    $astCount.textContent = `${ast.length} stmt${ast.length === 1 ? '' : 's'}`;

    // --- Stage 3: compile ---
    let chunk;
    try {
      chunk = CVM.compile(ast);
    } catch (e) {
      $bytecodeOut.textContent = `compile error: ${e.message}`;
      $bytecodeOut.classList.add('error');
      $output.textContent = '';
      setStatus('error', 'compile failed');
      return;
    }
    $bytecodeOut.classList.remove('error');
    $bytecodeOut.textContent = CVM.disassemble(chunk);
    $codeSize.textContent = `${chunk.code.length} bytes · ${chunk.constants.length} consts`;

    // --- Stage 4: run ---
    setStatus('running', 'running…');
    try {
      const result = CVM.run(chunk, { stepLimit: 200000 });
      $output.textContent = result.output.length
        ? result.output.join('\n')
        : '(no output produced)';
      $output.classList.remove('error');
      setStatus('success', `done · ${result.output.length} line${result.output.length === 1 ? '' : 's'}`);
    } catch (e) {
      $output.textContent = `runtime error: ${e.message}`;
      $output.classList.add('error');
      setStatus('error', 'runtime error');
    }
  }

  function formatTokens(tokens) {
    return tokens.map(t => {
      const lineStr = `L${String(t.line).padStart(2, ' ')}`;
      const typeStr = t.type.padEnd(11, ' ');
      const lexemeStr = t.lexeme ? ` ${t.lexeme}` : '';
      return `${lineStr}  ${typeStr}${lexemeStr}`;
    }).join('\n');
  }

  function setStatus(state, text) {
    $execStatus.textContent = text || state;
    $execStatus.classList.remove('error', 'running', 'success');
    if (state === 'error') $execStatus.classList.add('error');
    else if (state === 'running') $execStatus.classList.add('running');
    else if (state === 'success') $execStatus.classList.add('success');
  }

  // Debounced input handler
  $source.addEventListener('input', () => {
    clearTimeout(debounceTimer);
    debounceTimer = setTimeout(runPipeline, 200);
  });

  // Initial load: fibonacci sample
  $source.value = SAMPLES.fib;
  runPipeline();
})();
