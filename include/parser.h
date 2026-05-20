#pragma once
//
// parser.h
// ---------
// Recursive descent parser. Consumes tokens, produces a list of top-level
// statements (the program's AST).
//

#include "token.h"
#include "ast.h"
#include <vector>
#include <stdexcept>

namespace cvm {

class ParseError : public std::runtime_error {
public:
    int line;
    ParseError(const std::string& msg, int ln)
        : std::runtime_error(msg), line(ln) {}
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);

    // Parse the entire program; returns a list of top-level statements.
    std::vector<StmtPtr> parseProgram();

private:
    std::vector<Token> tokens_;
    size_t current_ = 0;

    // --- Statement-level rules ---
    StmtPtr parseStatement();
    StmtPtr parseAssignment();
    StmtPtr parsePrint();
    StmtPtr parseIf();
    StmtPtr parseWhile();
    StmtPtr parseBlock();

    // --- Expression-level rules (low → high precedence) ---
    ExprPtr parseExpression();
    ExprPtr parseComparison();
    ExprPtr parseTerm();       // + -
    ExprPtr parseFactor();     // * /
    ExprPtr parsePrimary();

    // --- Token navigation helpers ---
    const Token& peek() const;
    const Token& previous() const;
    bool  isAtEnd() const;
    bool  check(TokenType t) const;
    bool  match(std::initializer_list<TokenType> types);
    const Token& advance();
    const Token& consume(TokenType t, const std::string& msg);
};

} // namespace cvm
