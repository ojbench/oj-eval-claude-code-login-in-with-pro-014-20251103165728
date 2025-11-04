#pragma once
#ifndef PYTHON_INTERPRETER_EVALVISITOR_H
#define PYTHON_INTERPRETER_EVALVISITOR_H

#include "Python3ParserBaseVisitor.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Forward declarations
class Value;
class BigInteger;

// BigInteger class for arbitrary precision integers
class BigInteger {
private:
    std::vector<int> digits;  // Store digits in reverse order (little-endian)
    bool negative;

    void removeLeadingZeros();

public:
    BigInteger();
    BigInteger(long long n);
    BigInteger(const std::string& s);
    BigInteger(const BigInteger& other);

    BigInteger& operator=(const BigInteger& other);

    BigInteger operator+(const BigInteger& other) const;
    BigInteger operator-(const BigInteger& other) const;
    BigInteger operator*(const BigInteger& other) const;
    BigInteger operator/(const BigInteger& other) const;
    BigInteger operator%(const BigInteger& other) const;
    BigInteger operator-() const;

    bool operator<(const BigInteger& other) const;
    bool operator<=(const BigInteger& other) const;
    bool operator>(const BigInteger& other) const;
    bool operator>=(const BigInteger& other) const;
    bool operator==(const BigInteger& other) const;
    bool operator!=(const BigInteger& other) const;

    std::string toString() const;
    double toDouble() const;
    bool isZero() const;
    bool isNegative() const;
};

// Value class to represent Python values
class Value {
public:
    enum Type { NONE, BOOL, INT, FLOAT, STRING, TUPLE, FUNCTION };

    Type type;
    bool boolVal;
    BigInteger intVal;
    double floatVal;
    std::string strVal;
    std::vector<Value> tupleVal;

    // For functions
    Python3Parser::FuncdefContext* funcCtx;

    Value();
    Value(Type t);
    Value(bool b);
    Value(const BigInteger& i);
    Value(long long i);
    Value(double f);
    Value(const std::string& s);
    Value(const std::vector<Value>& t);

    std::string toString() const;
    bool toBool() const;
    BigInteger toInt() const;
    double toFloat() const;

    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const;
    bool operator<(const Value& other) const;
    bool operator<=(const Value& other) const;
    bool operator>(const Value& other) const;
    bool operator>=(const Value& other) const;
};

// Control flow exceptions
class BreakException : public std::exception {};
class ContinueException : public std::exception {};
class ReturnException : public std::exception {
public:
    Value value;
    ReturnException(const Value& v) : value(v) {}
};

// Scope management
class Scope {
public:
    std::map<std::string, Value> variables;
    Scope* parent;

    Scope(Scope* p = nullptr) : parent(p) {}

    void set(const std::string& name, const Value& value);
    Value get(const std::string& name);
    bool has(const std::string& name);
};

class EvalVisitor : public Python3ParserBaseVisitor {
private:
    Scope* currentScope;
    std::map<std::string, Python3Parser::FuncdefContext*> functions;

    Value evaluateComparison(const std::vector<Value>& values, const std::vector<std::string>& ops);
    std::string evaluateFormatString(const std::string& fstr);

public:
    EvalVisitor();
    ~EvalVisitor();

    std::any visitFile_input(Python3Parser::File_inputContext *ctx) override;
    std::any visitFuncdef(Python3Parser::FuncdefContext *ctx) override;
    std::any visitStmt(Python3Parser::StmtContext *ctx) override;
    std::any visitSimple_stmt(Python3Parser::Simple_stmtContext *ctx) override;
    std::any visitSmall_stmt(Python3Parser::Small_stmtContext *ctx) override;
    std::any visitExpr_stmt(Python3Parser::Expr_stmtContext *ctx) override;
    std::any visitAugassign(Python3Parser::AugassignContext *ctx) override;
    std::any visitFlow_stmt(Python3Parser::Flow_stmtContext *ctx) override;
    std::any visitBreak_stmt(Python3Parser::Break_stmtContext *ctx) override;
    std::any visitContinue_stmt(Python3Parser::Continue_stmtContext *ctx) override;
    std::any visitReturn_stmt(Python3Parser::Return_stmtContext *ctx) override;
    std::any visitCompound_stmt(Python3Parser::Compound_stmtContext *ctx) override;
    std::any visitIf_stmt(Python3Parser::If_stmtContext *ctx) override;
    std::any visitWhile_stmt(Python3Parser::While_stmtContext *ctx) override;
    std::any visitSuite(Python3Parser::SuiteContext *ctx) override;
    std::any visitTest(Python3Parser::TestContext *ctx) override;
    std::any visitOr_test(Python3Parser::Or_testContext *ctx) override;
    std::any visitAnd_test(Python3Parser::And_testContext *ctx) override;
    std::any visitNot_test(Python3Parser::Not_testContext *ctx) override;
    std::any visitComparison(Python3Parser::ComparisonContext *ctx) override;
    std::any visitComp_op(Python3Parser::Comp_opContext *ctx) override;
    std::any visitArith_expr(Python3Parser::Arith_exprContext *ctx) override;
    std::any visitAddorsub_op(Python3Parser::Addorsub_opContext *ctx) override;
    std::any visitTerm(Python3Parser::TermContext *ctx) override;
    std::any visitMuldivmod_op(Python3Parser::Muldivmod_opContext *ctx) override;
    std::any visitFactor(Python3Parser::FactorContext *ctx) override;
    std::any visitAtom_expr(Python3Parser::Atom_exprContext *ctx) override;
    std::any visitTrailer(Python3Parser::TrailerContext *ctx) override;
    std::any visitAtom(Python3Parser::AtomContext *ctx) override;
    std::any visitFormat_string(Python3Parser::Format_stringContext *ctx) override;
    std::any visitTestlist(Python3Parser::TestlistContext *ctx) override;
    std::any visitArglist(Python3Parser::ArglistContext *ctx) override;
    std::any visitArgument(Python3Parser::ArgumentContext *ctx) override;
};

#endif//PYTHON_INTERPRETER_EVALVISITOR_H
