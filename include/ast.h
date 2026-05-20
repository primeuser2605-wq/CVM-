#pragma once
//
// ast.h
// ------
// Abstract Syntax Tree node definitions.
// Two hierarchies: Expr (things that produce a value) and Stmt (things that
// do something). All nodes are owned via std::unique_ptr.
//

#include "token.h"
#include <memory>
#include <vector>
#include <string>

namespace cvm {

// ---------- Forward declarations ----------
struct NumberExpr;
struct BoolExpr;
struct VarExpr;
struct BinaryExpr;

struct AssignStmt;
struct PrintStmt;
struct IfStmt;
struct WhileStmt;
struct BlockStmt;

// ---------- Visitors ----------
// The Visitor pattern lets us add new operations (pretty-printer, compiler,
// interpreter) without modifying the AST classes.
struct ExprVisitor {
    virtual ~ExprVisitor() = default;
    virtual void visit(const NumberExpr&) = 0;
    virtual void visit(const BoolExpr&)   = 0;
    virtual void visit(const VarExpr&)    = 0;
    virtual void visit(const BinaryExpr&) = 0;
};

struct StmtVisitor {
    virtual ~StmtVisitor() = default;
    virtual void visit(const AssignStmt&) = 0;
    virtual void visit(const PrintStmt&)  = 0;
    virtual void visit(const IfStmt&)     = 0;
    virtual void visit(const WhileStmt&)  = 0;
    virtual void visit(const BlockStmt&)  = 0;
};

// ---------- Expression base ----------
struct Expr {
    virtual ~Expr() = default;
    virtual void accept(ExprVisitor& v) const = 0;
};
using ExprPtr = std::unique_ptr<Expr>;

// ---------- Statement base ----------
struct Stmt {
    virtual ~Stmt() = default;
    virtual void accept(StmtVisitor& v) const = 0;
};
using StmtPtr = std::unique_ptr<Stmt>;

// ---------- Expression nodes ----------

// Integer literal: 42
struct NumberExpr : Expr {
    int value;
    explicit NumberExpr(int v) : value(v) {}
    void accept(ExprVisitor& v) const override { v.visit(*this); }
};

// Boolean literal: true / false
struct BoolExpr : Expr {
    bool value;
    explicit BoolExpr(bool v) : value(v) {}
    void accept(ExprVisitor& v) const override { v.visit(*this); }
};

// Variable reference: x
struct VarExpr : Expr {
    std::string name;
    explicit VarExpr(std::string n) : name(std::move(n)) {}
    void accept(ExprVisitor& v) const override { v.visit(*this); }
};

// Binary op: a + b, a < b, etc.
// We store the *TokenType* of the operator to keep it simple.
struct BinaryExpr : Expr {
    TokenType op;
    ExprPtr   left;
    ExprPtr   right;
    BinaryExpr(TokenType o, ExprPtr l, ExprPtr r)
        : op(o), left(std::move(l)), right(std::move(r)) {}
    void accept(ExprVisitor& v) const override { v.visit(*this); }
};

// ---------- Statement nodes ----------

// Assignment: x = expr;
struct AssignStmt : Stmt {
    std::string name;
    ExprPtr     value;
    AssignStmt(std::string n, ExprPtr v)
        : name(std::move(n)), value(std::move(v)) {}
    void accept(StmtVisitor& v) const override { v.visit(*this); }
};

// Print: print expr;
struct PrintStmt : Stmt {
    ExprPtr value;
    explicit PrintStmt(ExprPtr v) : value(std::move(v)) {}
    void accept(StmtVisitor& v) const override { v.visit(*this); }
};

// if (cond) { ... } else { ... }
// elseBranch is optional (may be nullptr).
struct IfStmt : Stmt {
    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch;
    IfStmt(ExprPtr c, StmtPtr t, StmtPtr e)
        : condition(std::move(c)),
          thenBranch(std::move(t)),
          elseBranch(std::move(e)) {}
    void accept(StmtVisitor& v) const override { v.visit(*this); }
};

// while (cond) { ... }
struct WhileStmt : Stmt {
    ExprPtr condition;
    StmtPtr body;
    WhileStmt(ExprPtr c, StmtPtr b)
        : condition(std::move(c)), body(std::move(b)) {}
    void accept(StmtVisitor& v) const override { v.visit(*this); }
};

// { stmt1; stmt2; ... }
struct BlockStmt : Stmt {
    std::vector<StmtPtr> statements;
    explicit BlockStmt(std::vector<StmtPtr> s) : statements(std::move(s)) {}
    void accept(StmtVisitor& v) const override { v.visit(*this); }
};

} // namespace cvm
