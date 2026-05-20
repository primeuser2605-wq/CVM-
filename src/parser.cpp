//
// parser.cpp
// -----------
// Recursive descent implementation. Each grammar rule from Step 1 maps to
// one private method. Precedence falls out of the call order automatically.
//

#include "parser.h"
#include <sstream>

namespace cvm {

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

// ======================== Top-level ========================================

std::vector<StmtPtr> Parser::parseProgram() {
    std::vector<StmtPtr> statements;
    while (!isAtEnd()) {
        statements.push_back(parseStatement());
    }
    return statements;
}

// ======================== Statements =======================================

StmtPtr Parser::parseStatement() {
    if (check(TokenType::IF))     return parseIf();
    if (check(TokenType::WHILE))  return parseWhile();
    if (check(TokenType::PRINT))  return parsePrint();
    if (check(TokenType::LBRACE)) return parseBlock();

    // Otherwise: must be an assignment (IDENTIFIER = expr ;)
    return parseAssignment();
}

// assignment → IDENTIFIER "=" expression ";"
StmtPtr Parser::parseAssignment() {
    const Token& name = consume(TokenType::IDENTIFIER,
                                "Expected variable name at start of statement.");
    consume(TokenType::ASSIGN, "Expected '=' after variable name.");
    ExprPtr value = parseExpression();
    consume(TokenType::SEMICOLON, "Expected ';' after assignment.");
    return std::make_unique<AssignStmt>(name.lexeme, std::move(value));
}

// print_stmt → "print" expression ";"
StmtPtr Parser::parsePrint() {
    consume(TokenType::PRINT, "Expected 'print'.");
    ExprPtr value = parseExpression();
    consume(TokenType::SEMICOLON, "Expected ';' after print value.");
    return std::make_unique<PrintStmt>(std::move(value));
}

// if_stmt → "if" "(" expression ")" block ("else" block)?
StmtPtr Parser::parseIf() {
    consume(TokenType::IF, "Expected 'if'.");
    consume(TokenType::LPAREN, "Expected '(' after 'if'.");
    ExprPtr condition = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after if condition.");

    StmtPtr thenBranch = parseBlock();
    StmtPtr elseBranch = nullptr;
    if (match({TokenType::ELSE})) {
        elseBranch = parseBlock();
    }
    return std::make_unique<IfStmt>(
        std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

// while_stmt → "while" "(" expression ")" block
StmtPtr Parser::parseWhile() {
    consume(TokenType::WHILE, "Expected 'while'.");
    consume(TokenType::LPAREN, "Expected '(' after 'while'.");
    ExprPtr condition = parseExpression();
    consume(TokenType::RPAREN, "Expected ')' after while condition.");
    StmtPtr body = parseBlock();
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
}

// block → "{" statement* "}"
StmtPtr Parser::parseBlock() {
    consume(TokenType::LBRACE, "Expected '{' to start block.");
    std::vector<StmtPtr> stmts;
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        stmts.push_back(parseStatement());
    }
    consume(TokenType::RBRACE, "Expected '}' to close block.");
    return std::make_unique<BlockStmt>(std::move(stmts));
}

// ======================== Expressions ======================================
// Ordered from LOWEST precedence (top) to HIGHEST (bottom).

ExprPtr Parser::parseExpression() {
    return parseComparison();
}

// comparison → term (("==" | "<" | ">") term)?
ExprPtr Parser::parseComparison() {
    ExprPtr left = parseTerm();
    if (match({TokenType::EQ, TokenType::LT, TokenType::GT})) {
        TokenType op = previous().type;
        ExprPtr right = parseTerm();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

// term → factor (("+" | "-") factor)*
ExprPtr Parser::parseTerm() {
    ExprPtr left = parseFactor();
    while (match({TokenType::PLUS, TokenType::MINUS})) {
        TokenType op = previous().type;
        ExprPtr right = parseFactor();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

// factor → primary (("*" | "/") primary)*
ExprPtr Parser::parseFactor() {
    ExprPtr left = parsePrimary();
    while (match({TokenType::STAR, TokenType::SLASH})) {
        TokenType op = previous().type;
        ExprPtr right = parsePrimary();
        left = std::make_unique<BinaryExpr>(op, std::move(left), std::move(right));
    }
    return left;
}

// primary → NUMBER | IDENTIFIER | "true" | "false" | "(" expression ")"
ExprPtr Parser::parsePrimary() {
    if (match({TokenType::NUMBER})) {
        int value = std::stoi(previous().lexeme);
        return std::make_unique<NumberExpr>(value);
    }
    if (match({TokenType::TRUE}))  return std::make_unique<BoolExpr>(true);
    if (match({TokenType::FALSE})) return std::make_unique<BoolExpr>(false);

    if (match({TokenType::IDENTIFIER})) {
        return std::make_unique<VarExpr>(previous().lexeme);
    }

    if (match({TokenType::LPAREN})) {
        ExprPtr expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')' after expression.");
        return expr;
    }

    throw ParseError(
        "Unexpected token '" + peek().lexeme + "' in expression.", peek().line);
}

// ======================== Token helpers ====================================

const Token& Parser::peek() const     { return tokens_[current_]; }
const Token& Parser::previous() const { return tokens_[current_ - 1]; }
bool Parser::isAtEnd() const          { return peek().type == TokenType::END_OF_FILE; }

bool Parser::check(TokenType t) const {
    if (isAtEnd()) return false;
    return peek().type == t;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (TokenType t : types) {
        if (check(t)) { advance(); return true; }
    }
    return false;
}

const Token& Parser::advance() {
    if (!isAtEnd()) current_++;
    return previous();
}

const Token& Parser::consume(TokenType t, const std::string& msg) {
    if (check(t)) return advance();
    throw ParseError(msg + " (got '" + peek().lexeme + "')", peek().line);
}

} // namespace cvm
