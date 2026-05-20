//
// lexer.cpp
// ----------
// Implementation of the Lexer. Walks the source string character-by-character
// and builds a vector of Tokens. See lexer.h for the interface.
//

#include "lexer.h"
#include <cctype>
#include <unordered_map>

namespace cvm {

// --- Keyword table -----------------------------------------------------------
// When we scan an identifier (e.g. "while"), we check this map.
// If it matches a keyword, we upgrade the token type from IDENTIFIER
// to the specific keyword type. This is a classic lexer trick: scan
// identifiers liberally, then classify.
static const std::unordered_map<std::string, TokenType> KEYWORDS = {
    {"if",    TokenType::IF},
    {"else",  TokenType::ELSE},
    {"while", TokenType::WHILE},
    {"print", TokenType::PRINT},
    {"true",  TokenType::TRUE},
    {"false", TokenType::FALSE},
};

// --- Constructor -------------------------------------------------------------
Lexer::Lexer(std::string source)
    : source_(std::move(source)), start_(0), current_(0), line_(1) {}

// --- Public entry point ------------------------------------------------------
std::vector<Token> Lexer::tokenize() {
    while (!isAtEnd()) {
        // Each iteration scans exactly one token (or skips whitespace).
        // start_ marks where the current token began.
        start_ = current_;
        scanToken();
    }
    tokens_.emplace_back(TokenType::END_OF_FILE, "", line_);
    return tokens_;
}

// --- scanToken: the main dispatch --------------------------------------------
// Look at the next character and decide what kind of token to build.
void Lexer::scanToken() {
    char c = advance();
    switch (c) {
        // Single-char tokens — easy cases.
        case '(': addToken(TokenType::LPAREN);    break;
        case ')': addToken(TokenType::RPAREN);    break;
        case '{': addToken(TokenType::LBRACE);    break;
        case '}': addToken(TokenType::RBRACE);    break;
        case ';': addToken(TokenType::SEMICOLON); break;
        case '+': addToken(TokenType::PLUS);      break;
        case '-': addToken(TokenType::MINUS);     break;
        case '*': addToken(TokenType::STAR);      break;
        case '<': addToken(TokenType::LT);        break;
        case '>': addToken(TokenType::GT);        break;

        // '=' is tricky: could be ASSIGN (=) or EQ (==).
        // We peek: if the next char is '=', consume it too.
        case '=':
            if (match('=')) addToken(TokenType::EQ);
            else            addToken(TokenType::ASSIGN);
            break;

        // '/' could start a comment ("//") or be division.
        case '/':
            if (match('/')) {
                // Line comment — consume until end of line.
                while (!isAtEnd() && peek() != '\n') advance();
            } else {
                addToken(TokenType::SLASH);
            }
            break;

        // Whitespace: just skip. '\n' also advances the line counter.
        case ' ':
        case '\r':
        case '\t':
            break;
        case '\n':
            line_++;
            break;

        default:
            // Multi-character tokens: numbers or identifiers/keywords.
            if (std::isdigit(static_cast<unsigned char>(c))) {
                scanNumber();
            } else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
                scanIdentifier();
            } else {
                throw LexError(
                    std::string("Unexpected character: '") + c + "'", line_);
            }
            break;
    }
}

// --- scanNumber: keep consuming digits ---------------------------------------
// We only support integers in CVM++ for simplicity. Adding floats would
// mean also looking for a '.' followed by more digits.
void Lexer::scanNumber() {
    while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
        advance();
    }
    addToken(TokenType::NUMBER);
}

// --- scanIdentifier: letters/digits/underscores, then classify ---------------
void Lexer::scanIdentifier() {
    while (!isAtEnd()) {
        char c = peek();
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
            advance();
        } else {
            break;
        }
    }

    // Extract the text we just scanned and check if it's a reserved keyword.
    std::string text = source_.substr(start_, current_ - start_);
    auto it = KEYWORDS.find(text);
    TokenType type = (it != KEYWORDS.end()) ? it->second : TokenType::IDENTIFIER;
    addToken(type);
}

// --- Helpers -----------------------------------------------------------------
bool Lexer::isAtEnd() const {
    return current_ >= source_.size();
}

char Lexer::advance() {
    // Post-increment: return the current char AND move forward.
    return source_[current_++];
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source_[current_];
}

char Lexer::peekNext() const {
    if (current_ + 1 >= source_.size()) return '\0';
    return source_[current_ + 1];
}

bool Lexer::match(char expected) {
    if (isAtEnd() || source_[current_] != expected) return false;
    current_++;  // consume it
    return true;
}

void Lexer::addToken(TokenType type) {
    // The lexeme is the substring from where this token started to now.
    std::string lexeme = source_.substr(start_, current_ - start_);
    tokens_.emplace_back(type, std::move(lexeme), line_);
}

} // namespace cvm
