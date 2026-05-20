#pragma once
//
// lexer.h
// --------
// The Lexer takes a raw source string and produces a vector<Token>.
// It's a "one-pass scanner": reads left-to-right, emits tokens, never
// backtracks more than one character.
//

#include "token.h"
#include <string>
#include <vector>
#include <stdexcept>

namespace cvm {

// Thrown when the lexer sees a character it doesn't know what to do with.
// We carry the line number so the user can locate the problem.
class LexError : public std::runtime_error {
public:
    int line;
    LexError(const std::string& msg, int ln)
        : std::runtime_error(msg), line(ln) {}
};

class Lexer {
public:
    // Construct with the source code to tokenize.
    explicit Lexer(std::string source);

    // Runs the scan and returns all tokens (ending with END_OF_FILE).
    std::vector<Token> tokenize();

private:
    // --- State ---
    std::string source_;   // the full source text
    size_t      start_;    // start of the current token being scanned
    size_t      current_;  // the character we're about to look at
    int         line_;     // current line number (for error reporting)
    std::vector<Token> tokens_;

    // --- Core scan steps ---
    void scanToken();              // scan a single token
    void scanNumber();             // scan a numeric literal
    void scanIdentifier();         // scan an identifier or keyword

    // --- Low-level helpers ---
    bool isAtEnd() const;          // have we consumed all characters?
    char advance();                // consume and return current char
    char peek() const;              // look at current char without consuming
    char peekNext() const;         // look one char ahead
    bool match(char expected);     // consume if current == expected

    // Add a token using the text from start_ .. current_.
    void addToken(TokenType type);
};

} // namespace cvm
