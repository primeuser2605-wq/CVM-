#pragma once
//
// token.h
// ---------
// Defines every kind of token our lexer can produce, plus the Token struct
// itself. A Token is the smallest meaningful unit of source code — think of
// it like a "word" in a sentence.
//

#include <string>
#include <ostream>

namespace cvm {

// Every kind of token our language understands.
// Keep this enum small and flat — it's easier to debug.
enum class TokenType {
    // Literals
    NUMBER,       // 42, 7, 100
    IDENTIFIER,   // variable names: x, count, myVar

    // Keywords
    IF,
    ELSE,
    WHILE,
    PRINT,
    TRUE,
    FALSE,

    // Operators
    PLUS,         // +
    MINUS,        // -
    STAR,         // *
    SLASH,        // /
    ASSIGN,       // =
    EQ,           // ==
    LT,           // <
    GT,           // >

    // Punctuation
    LPAREN,       // (
    RPAREN,       // )
    LBRACE,       // {
    RBRACE,       // }
    SEMICOLON,    // ;

    // Special
    END_OF_FILE
};

// A single token produced by the lexer.
//   - type:   what kind of token this is
//   - lexeme: the actual text from the source (useful for identifiers/numbers)
//   - line:   which line it came from (helps with error messages)
struct Token {
    TokenType   type;
    std::string lexeme;
    int         line;

    Token(TokenType t, std::string lex, int ln)
        : type(t), lexeme(std::move(lex)), line(ln) {}
};

// Helper: turn a TokenType into a readable string (for debugging / --tokens flag).
inline const char* tokenTypeName(TokenType t) {
    switch (t) {
        case TokenType::NUMBER:      return "NUMBER";
        case TokenType::IDENTIFIER:  return "IDENTIFIER";
        case TokenType::IF:          return "IF";
        case TokenType::ELSE:        return "ELSE";
        case TokenType::WHILE:       return "WHILE";
        case TokenType::PRINT:       return "PRINT";
        case TokenType::TRUE:        return "TRUE";
        case TokenType::FALSE:       return "FALSE";
        case TokenType::PLUS:        return "PLUS";
        case TokenType::MINUS:       return "MINUS";
        case TokenType::STAR:        return "STAR";
        case TokenType::SLASH:       return "SLASH";
        case TokenType::ASSIGN:      return "ASSIGN";
        case TokenType::EQ:          return "EQ";
        case TokenType::LT:          return "LT";
        case TokenType::GT:          return "GT";
        case TokenType::LPAREN:      return "LPAREN";
        case TokenType::RPAREN:      return "RPAREN";
        case TokenType::LBRACE:      return "LBRACE";
        case TokenType::RBRACE:      return "RBRACE";
        case TokenType::SEMICOLON:   return "SEMICOLON";
        case TokenType::END_OF_FILE: return "EOF";
    }
    return "UNKNOWN";
}

} // namespace cvm
