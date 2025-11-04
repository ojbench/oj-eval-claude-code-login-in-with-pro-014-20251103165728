#include "Evalvisitor.h"
#include "Python3Parser.h"
#include "Python3Lexer.h"
#include <cmath>
#include <cctype>

using namespace antlr4;

// ============================================================================
// BigInteger Implementation
// ============================================================================

BigInteger::BigInteger() : negative(false) {
    digits.push_back(0);
}

BigInteger::BigInteger(long long n) : negative(n < 0) {
    if (n == 0) {
        digits.push_back(0);
        return;
    }
    n = std::abs(n);
    while (n > 0) {
        digits.push_back(n % 10);
        n /= 10;
    }
}

BigInteger::BigInteger(const std::string& s) {
    if (s.empty() || s == "0") {
        digits.push_back(0);
        negative = false;
        return;
    }
    
    size_t start = 0;
    negative = (s[0] == '-');
    if (negative || s[0] == '+') start = 1;
    
    for (int i = s.length() - 1; i >= (int)start; i--) {
        digits.push_back(s[i] - '0');
    }
    removeLeadingZeros();
}

BigInteger::BigInteger(const BigInteger& other) 
    : digits(other.digits), negative(other.negative) {}

BigInteger& BigInteger::operator=(const BigInteger& other) {
    if (this != &other) {
        digits = other.digits;
        negative = other.negative;
    }
    return *this;
}

void BigInteger::removeLeadingZeros() {
    while (digits.size() > 1 && digits.back() == 0) {
        digits.pop_back();
    }
    if (digits.size() == 1 && digits[0] == 0) {
        negative = false;
    }
}

bool BigInteger::isZero() const {
    return digits.size() == 1 && digits[0] == 0;
}

bool BigInteger::isNegative() const {
    return negative && !isZero();
}

std::string BigInteger::toString() const {
    std::string result;
    if (negative && !isZero()) result += "-";
    for (int i = digits.size() - 1; i >= 0; i--) {
        result += char('0' + digits[i]);
    }
    return result;
}

double BigInteger::toDouble() const {
    double result = 0;
    double multiplier = 1;
    for (size_t i = 0; i < digits.size(); i++) {
        result += digits[i] * multiplier;
        multiplier *= 10;
    }
    return negative ? -result : result;
}

bool BigInteger::operator<(const BigInteger& other) const {
    if (negative != other.negative) return negative;
    
    if (digits.size() != other.digits.size()) {
        return negative ? (digits.size() > other.digits.size()) 
                       : (digits.size() < other.digits.size());
    }
    
    for (int i = digits.size() - 1; i >= 0; i--) {
        if (digits[i] != other.digits[i]) {
            return negative ? (digits[i] > other.digits[i]) 
                           : (digits[i] < other.digits[i]);
        }
    }
    return false;
}

bool BigInteger::operator<=(const BigInteger& other) const {
    return *this < other || *this == other;
}

bool BigInteger::operator>(const BigInteger& other) const {
    return !(*this <= other);
}

bool BigInteger::operator>=(const BigInteger& other) const {
    return !(*this < other);
}

bool BigInteger::operator==(const BigInteger& other) const {
    return negative == other.negative && digits == other.digits;
}

bool BigInteger::operator!=(const BigInteger& other) const {
    return !(*this == other);
}

BigInteger BigInteger::operator-() const {
    BigInteger result = *this;
    if (!isZero()) {
        result.negative = !result.negative;
    }
    return result;
}

BigInteger BigInteger::operator+(const BigInteger& other) const {
    if (negative != other.negative) {
        if (negative) {
            return other - (-(*this));
        } else {
            return *this - (-other);
        }
    }
    
    BigInteger result;
    result.negative = negative;
    result.digits.clear();
    
    int carry = 0;
    size_t maxSize = std::max(digits.size(), other.digits.size());
    
    for (size_t i = 0; i < maxSize || carry; i++) {
        int sum = carry;
        if (i < digits.size()) sum += digits[i];
        if (i < other.digits.size()) sum += other.digits[i];
        result.digits.push_back(sum % 10);
        carry = sum / 10;
    }
    
    result.removeLeadingZeros();
    return result;
}

BigInteger BigInteger::operator-(const BigInteger& other) const {
    if (negative != other.negative) {
        if (negative) {
            return -((-(*this)) + other);
        } else {
            return *this + (-other);
        }
    }
    
    if ((!negative && *this < other) || (negative && *this > other)) {
        return -(other - *this);
    }
    
    BigInteger result;
    result.negative = negative;
    result.digits.clear();
    
    int borrow = 0;
    for (size_t i = 0; i < digits.size(); i++) {
        int diff = digits[i] - borrow;
        if (i < other.digits.size()) diff -= other.digits[i];
        
        if (diff < 0) {
            diff += 10;
            borrow = 1;
        } else {
            borrow = 0;
        }
        result.digits.push_back(diff);
    }
    
    result.removeLeadingZeros();
    return result;
}

BigInteger BigInteger::operator*(const BigInteger& other) const {
    BigInteger result;
    result.digits.assign(digits.size() + other.digits.size(), 0);
    
    for (size_t i = 0; i < digits.size(); i++) {
        int carry = 0;
        for (size_t j = 0; j < other.digits.size() || carry; j++) {
            long long cur = result.digits[i + j] + 
                           digits[i] * 1LL * (j < other.digits.size() ? other.digits[j] : 0) + carry;
            result.digits[i + j] = cur % 10;
            carry = cur / 10;
        }
    }
    
    result.removeLeadingZeros();
    result.negative = (negative != other.negative) && !result.isZero();
    return result;
}

BigInteger BigInteger::operator/(const BigInteger& other) const {
    if (other.isZero()) {
        throw std::runtime_error("Division by zero");
    }
    
    BigInteger a = *this;
    BigInteger b = other;
    bool resultNegative = (negative != other.negative);
    a.negative = false;
    b.negative = false;
    
    if (a < b) {
        // Floor division: if signs differ and remainder exists, return -1
        if (resultNegative && !a.isZero()) {
            return BigInteger(-1);
        }
        return BigInteger(0);
    }
    
    BigInteger result;
    result.digits.clear();
    BigInteger current;
    current.digits.clear();
    
    for (int i = a.digits.size() - 1; i >= 0; i--) {
        current.digits.insert(current.digits.begin(), a.digits[i]);
        current.removeLeadingZeros();
        
        int quotient = 0;
        while (current >= b) {
            current = current - b;
            quotient++;
        }
        result.digits.insert(result.digits.begin(), quotient);
    }
    
    result.removeLeadingZeros();
    
    // Python floor division
    if (resultNegative && !current.isZero()) {
        result = result + BigInteger(1);
        result.negative = true;
    } else if (resultNegative && !result.isZero()) {
        result.negative = true;
    }
    
    return result;
}

BigInteger BigInteger::operator%(const BigInteger& other) const {
    // a % b = a - (a // b) * b
    return *this - (*this / other) * other;
}

// ============================================================================
// Value Implementation
// ============================================================================

Value::Value() : type(NONE), funcCtx(nullptr) {}

Value::Value(Type t) : type(t), funcCtx(nullptr) {
    if (type == BOOL) boolVal = false;
    else if (type == INT) intVal = BigInteger(0);
    else if (type == FLOAT) floatVal = 0.0;
}

Value::Value(bool b) : type(BOOL), boolVal(b), funcCtx(nullptr) {}

Value::Value(const BigInteger& i) : type(INT), intVal(i), funcCtx(nullptr) {}

Value::Value(long long i) : type(INT), intVal(i), funcCtx(nullptr) {}

Value::Value(double f) : type(FLOAT), floatVal(f), funcCtx(nullptr) {}

Value::Value(const std::string& s) : type(STRING), strVal(s), funcCtx(nullptr) {}

Value::Value(const std::vector<Value>& t) : type(TUPLE), tupleVal(t), funcCtx(nullptr) {}

std::string Value::toString() const {
    std::ostringstream oss;
    switch (type) {
        case NONE:
            return "None";
        case BOOL:
            return boolVal ? "True" : "False";
        case INT:
            return intVal.toString();
        case FLOAT:
            oss << std::fixed << std::setprecision(6) << floatVal;
            return oss.str();
        case STRING:
            return strVal;
        case TUPLE: {
            std::string result = "(";
            for (size_t i = 0; i < tupleVal.size(); i++) {
                result += tupleVal[i].toString();
                if (i < tupleVal.size() - 1) result += ", ";
            }
            if (tupleVal.size() == 1) result += ",";
            result += ")";
            return result;
        }
        default:
            return "";
    }
}

bool Value::toBool() const {
    switch (type) {
        case NONE:
            return false;
        case BOOL:
            return boolVal;
        case INT:
            return !intVal.isZero();
        case FLOAT:
            return floatVal != 0.0;
        case STRING:
            return !strVal.empty();
        case TUPLE:
            return !tupleVal.empty();
        default:
            return false;
    }
}

BigInteger Value::toInt() const {
    switch (type) {
        case BOOL:
            return BigInteger(boolVal ? 1 : 0);
        case INT:
            return intVal;
        case FLOAT:
            return BigInteger((long long)floatVal);
        case STRING: {
            // Remove leading/trailing whitespace
            std::string s = strVal;
            size_t start = s.find_first_not_of(" \t\n\r");
            size_t end = s.find_last_not_of(" \t\n\r");
            if (start == std::string::npos) return BigInteger(0);
            s = s.substr(start, end - start + 1);
            return BigInteger(s);
        }
        default:
            return BigInteger(0);
    }
}

double Value::toFloat() const {
    switch (type) {
        case BOOL:
            return boolVal ? 1.0 : 0.0;
        case INT:
            return intVal.toDouble();
        case FLOAT:
            return floatVal;
        case STRING:
            return std::stod(strVal);
        default:
            return 0.0;
    }
}

bool Value::operator==(const Value& other) const {
    // Try to convert to the same type
    if (type == other.type) {
        switch (type) {
            case NONE: return true;
            case BOOL: return boolVal == other.boolVal;
            case INT: return intVal == other.intVal;
            case FLOAT: return floatVal == other.floatVal;
            case STRING: return strVal == other.strVal;
            default: return false;
        }
    }
    
    // Type conversions for comparison (but not to string)
    if ((type == INT || type == FLOAT || type == BOOL) && 
        (other.type == INT || other.type == FLOAT || other.type == BOOL)) {
        if (type == FLOAT || other.type == FLOAT) {
            return toFloat() == other.toFloat();
        } else {
            return toInt() == other.toInt();
        }
    }
    
    return false;
}

bool Value::operator!=(const Value& other) const {
    return !(*this == other);
}

bool Value::operator<(const Value& other) const {
    if (type == STRING && other.type == STRING) {
        return strVal < other.strVal;
    }
    
    if ((type == INT || type == FLOAT || type == BOOL) && 
        (other.type == INT || other.type == FLOAT || other.type == BOOL)) {
        if (type == FLOAT || other.type == FLOAT) {
            return toFloat() < other.toFloat();
        } else {
            return toInt() < other.toInt();
        }
    }
    
    return false;
}

bool Value::operator<=(const Value& other) const {
    return *this < other || *this == other;
}

bool Value::operator>(const Value& other) const {
    return !(*this <= other);
}

bool Value::operator>=(const Value& other) const {
    return !(*this < other);
}

// ============================================================================
// Scope Implementation
// ============================================================================

void Scope::set(const std::string& name, const Value& value) {
    variables[name] = value;
}

Value Scope::get(const std::string& name) {
    // Search in current scope
    if (variables.find(name) != variables.end()) {
        return variables[name];
    }
    // Search in parent scopes (global access)
    if (parent) {
        return parent->get(name);
    }
    // Variable not found - return function name as string for built-in functions
    return Value(name);
}

bool Scope::has(const std::string& name) {
    if (variables.find(name) != variables.end()) {
        return true;
    }
    if (parent) {
        return parent->has(name);
    }
    return false;
}

// ============================================================================
// EvalVisitor Implementation
// ============================================================================

EvalVisitor::EvalVisitor() {
    currentScope = new Scope();
}

EvalVisitor::~EvalVisitor() {
    // Clean up scopes
    while (currentScope) {
        Scope* parent = currentScope->parent;
        delete currentScope;
        currentScope = parent;
    }
}

std::any EvalVisitor::visitFile_input(Python3Parser::File_inputContext *ctx) {
    for (auto stmt : ctx->stmt()) {
        visit(stmt);
    }
    return nullptr;
}

std::any EvalVisitor::visitFuncdef(Python3Parser::FuncdefContext *ctx) {
    std::string funcName = ctx->NAME()->getText();
    functions[funcName] = ctx;
    return nullptr;
}

std::any EvalVisitor::visitStmt(Python3Parser::StmtContext *ctx) {
    if (ctx->simple_stmt()) {
        return visit(ctx->simple_stmt());
    } else if (ctx->compound_stmt()) {
        return visit(ctx->compound_stmt());
    }
    return nullptr;
}

std::any EvalVisitor::visitSimple_stmt(Python3Parser::Simple_stmtContext *ctx) {
    return visit(ctx->small_stmt());
}

std::any EvalVisitor::visitSmall_stmt(Python3Parser::Small_stmtContext *ctx) {
    if (ctx->expr_stmt()) {
        return visit(ctx->expr_stmt());
    } else if (ctx->flow_stmt()) {
        return visit(ctx->flow_stmt());
    }
    return nullptr;
}

std::any EvalVisitor::visitExpr_stmt(Python3Parser::Expr_stmtContext *ctx) {
    auto testlists = ctx->testlist();
    
    if (testlists.size() == 1) {
        // Just evaluate the expression
        visit(testlists[0]);
        return nullptr;
    }
    
    // Assignment or augmented assignment
    if (ctx->augassign()) {
        // Augmented assignment: a += b
        auto leftTestlist = testlists[0];
        auto leftTests = leftTestlist->test();
        std::string varName = leftTests[0]->getText();
        
        Value currentVal = currentScope->get(varName);
        Value rightVal = std::any_cast<Value>(visit(testlists[1]));
        std::string op = ctx->augassign()->getText();
        
        Value result;
        if (op == "+=") {
            if (currentVal.type == Value::STRING && rightVal.type == Value::STRING) {
                result = Value(currentVal.strVal + rightVal.strVal);
            } else if (currentVal.type == Value::STRING && rightVal.type == Value::INT) {
                std::string repeated;
                BigInteger count = rightVal.intVal;
                long long n = count.toDouble();
                for (long long i = 0; i < n; i++) {
                    repeated += currentVal.strVal;
                }
                result = Value(repeated);
            } else if (currentVal.type == Value::FLOAT || rightVal.type == Value::FLOAT) {
                result = Value(currentVal.toFloat() + rightVal.toFloat());
            } else {
                result = Value(currentVal.toInt() + rightVal.toInt());
            }
        } else if (op == "-=") {
            if (currentVal.type == Value::FLOAT || rightVal.type == Value::FLOAT) {
                result = Value(currentVal.toFloat() - rightVal.toFloat());
            } else {
                result = Value(currentVal.toInt() - rightVal.toInt());
            }
        } else if (op == "*=") {
            if (currentVal.type == Value::FLOAT || rightVal.type == Value::FLOAT) {
                result = Value(currentVal.toFloat() * rightVal.toFloat());
            } else if (currentVal.type == Value::STRING && rightVal.type == Value::INT) {
                std::string repeated;
                long long n = rightVal.intVal.toDouble();
                for (long long i = 0; i < n; i++) {
                    repeated += currentVal.strVal;
                }
                result = Value(repeated);
            } else {
                result = Value(currentVal.toInt() * rightVal.toInt());
            }
        } else if (op == "/=") {
            result = Value(currentVal.toFloat() / rightVal.toFloat());
        } else if (op == "//=") {
            result = Value(currentVal.toInt() / rightVal.toInt());
        } else if (op == "%=") {
            result = Value(currentVal.toInt() % rightVal.toInt());
        }
        
        currentScope->set(varName, result);
    } else {
        // Multiple assignment: a = b = value or a, b = 1, 2
        Value rightVal = std::any_cast<Value>(visit(testlists.back()));
        
        // Check for multiple variable assignment
        for (size_t i = 0; i < testlists.size() - 1; i++) {
            auto tests = testlists[i]->test();
            
            if (tests.size() > 1) {
                // Multiple assignment: a, b = ...
                std::vector<Value> values;
                if (rightVal.type == Value::TUPLE) {
                    values = rightVal.tupleVal;
                } else {
                    auto rightTests = testlists.back()->test();
                    for (auto t : rightTests) {
                        values.push_back(std::any_cast<Value>(visit(t)));
                    }
                }
                
                for (size_t j = 0; j < tests.size() && j < values.size(); j++) {
                    std::string varName = tests[j]->getText();
                    currentScope->set(varName, values[j]);
                }
            } else {
                // Single variable assignment
                std::string varName = tests[0]->getText();
                currentScope->set(varName, rightVal);
            }
        }
    }
    
    return nullptr;
}

std::any EvalVisitor::visitAugassign(Python3Parser::AugassignContext *ctx) {
    return ctx->getText();
}

std::any EvalVisitor::visitFlow_stmt(Python3Parser::Flow_stmtContext *ctx) {
    if (ctx->break_stmt()) {
        return visit(ctx->break_stmt());
    } else if (ctx->continue_stmt()) {
        return visit(ctx->continue_stmt());
    } else if (ctx->return_stmt()) {
        return visit(ctx->return_stmt());
    }
    return nullptr;
}

std::any EvalVisitor::visitBreak_stmt(Python3Parser::Break_stmtContext *ctx) {
    throw BreakException();
}

std::any EvalVisitor::visitContinue_stmt(Python3Parser::Continue_stmtContext *ctx) {
    throw ContinueException();
}

std::any EvalVisitor::visitReturn_stmt(Python3Parser::Return_stmtContext *ctx) {
    if (ctx->testlist()) {
        auto tests = ctx->testlist()->test();
        if (tests.size() == 1) {
            Value val = std::any_cast<Value>(visit(tests[0]));
            throw ReturnException(val);
        } else {
            std::vector<Value> values;
            for (auto t : tests) {
                values.push_back(std::any_cast<Value>(visit(t)));
            }
            throw ReturnException(Value(values));
        }
    }
    throw ReturnException(Value());
}

std::any EvalVisitor::visitCompound_stmt(Python3Parser::Compound_stmtContext *ctx) {
    if (ctx->if_stmt()) {
        return visit(ctx->if_stmt());
    } else if (ctx->while_stmt()) {
        return visit(ctx->while_stmt());
    } else if (ctx->funcdef()) {
        return visit(ctx->funcdef());
    }
    return nullptr;
}

std::any EvalVisitor::visitIf_stmt(Python3Parser::If_stmtContext *ctx) {
    auto tests = ctx->test();
    auto suites = ctx->suite();
    
    // if condition
    Value condition = std::any_cast<Value>(visit(tests[0]));
    if (condition.toBool()) {
        visit(suites[0]);
        return nullptr;
    }
    
    // elif conditions
    for (size_t i = 1; i < tests.size(); i++) {
        condition = std::any_cast<Value>(visit(tests[i]));
        if (condition.toBool()) {
            visit(suites[i]);
            return nullptr;
        }
    }
    
    // else
    if (suites.size() > tests.size()) {
        visit(suites.back());
    }
    
    return nullptr;
}

std::any EvalVisitor::visitWhile_stmt(Python3Parser::While_stmtContext *ctx) {
    while (true) {
        Value condition = std::any_cast<Value>(visit(ctx->test()));
        if (!condition.toBool()) break;
        
        try {
            visit(ctx->suite());
        } catch (BreakException&) {
            break;
        } catch (ContinueException&) {
            continue;
        }
    }
    return nullptr;
}

std::any EvalVisitor::visitSuite(Python3Parser::SuiteContext *ctx) {
    if (ctx->simple_stmt()) {
        return visit(ctx->simple_stmt());
    }
    for (auto stmt : ctx->stmt()) {
        visit(stmt);
    }
    return nullptr;
}

std::any EvalVisitor::visitTest(Python3Parser::TestContext *ctx) {
    return visit(ctx->or_test());
}

std::any EvalVisitor::visitOr_test(Python3Parser::Or_testContext *ctx) {
    auto andTests = ctx->and_test();
    Value result = std::any_cast<Value>(visit(andTests[0]));
    
    for (size_t i = 1; i < andTests.size(); i++) {
        if (result.toBool()) {
            return result;  // Short-circuit
        }
        result = std::any_cast<Value>(visit(andTests[i]));
    }
    
    return result;
}

std::any EvalVisitor::visitAnd_test(Python3Parser::And_testContext *ctx) {
    auto notTests = ctx->not_test();
    Value result = std::any_cast<Value>(visit(notTests[0]));
    
    for (size_t i = 1; i < notTests.size(); i++) {
        if (!result.toBool()) {
            return result;  // Short-circuit
        }
        result = std::any_cast<Value>(visit(notTests[i]));
    }
    
    return result;
}

std::any EvalVisitor::visitNot_test(Python3Parser::Not_testContext *ctx) {
    if (ctx->NOT()) {
        Value val = std::any_cast<Value>(visit(ctx->not_test()));
        return Value(!val.toBool());
    }
    return visit(ctx->comparison());
}

std::any EvalVisitor::visitComparison(Python3Parser::ComparisonContext *ctx) {
    auto arithExprs = ctx->arith_expr();
    
    if (arithExprs.size() == 1) {
        return visit(arithExprs[0]);
    }
    
    // Chained comparison: evaluate each value once
    std::vector<Value> values;
    for (auto expr : arithExprs) {
        values.push_back(std::any_cast<Value>(visit(expr)));
    }
    
    std::vector<std::string> ops;
    for (auto op : ctx->comp_op()) {
        ops.push_back(op->getText());
    }
    
    return evaluateComparison(values, ops);
}

Value EvalVisitor::evaluateComparison(const std::vector<Value>& values, const std::vector<std::string>& ops) {
    for (size_t i = 0; i < ops.size(); i++) {
        bool result;
        const Value& left = values[i];
        const Value& right = values[i + 1];
        
        if (ops[i] == "<") {
            result = left < right;
        } else if (ops[i] == ">") {
            result = left > right;
        } else if (ops[i] == "<=") {
            result = left <= right;
        } else if (ops[i] == ">=") {
            result = left >= right;
        } else if (ops[i] == "==") {
            result = left == right;
        } else if (ops[i] == "!=") {
            result = left != right;
        } else {
            result = false;
        }
        
        if (!result) {
            return Value(false);
        }
    }
    return Value(true);
}

std::any EvalVisitor::visitComp_op(Python3Parser::Comp_opContext *ctx) {
    return ctx->getText();
}

std::any EvalVisitor::visitArith_expr(Python3Parser::Arith_exprContext *ctx) {
    auto terms = ctx->term();
    Value result = std::any_cast<Value>(visit(terms[0]));
    
    for (size_t i = 1; i < terms.size(); i++) {
        std::string op = ctx->addorsub_op(i - 1)->getText();
        Value right = std::any_cast<Value>(visit(terms[i]));
        
        if (op == "+") {
            if (result.type == Value::STRING && right.type == Value::STRING) {
                result = Value(result.strVal + right.strVal);
            } else if (result.type == Value::STRING && right.type == Value::INT) {
                std::string repeated;
                long long n = right.intVal.toDouble();
                for (long long i = 0; i < n; i++) {
                    repeated += result.strVal;
                }
                result = Value(repeated);
            } else if (result.type == Value::FLOAT || right.type == Value::FLOAT) {
                result = Value(result.toFloat() + right.toFloat());
            } else {
                result = Value(result.toInt() + right.toInt());
            }
        } else if (op == "-") {
            if (result.type == Value::FLOAT || right.type == Value::FLOAT) {
                result = Value(result.toFloat() - right.toFloat());
            } else {
                result = Value(result.toInt() - right.toInt());
            }
        }
    }
    
    return result;
}

std::any EvalVisitor::visitAddorsub_op(Python3Parser::Addorsub_opContext *ctx) {
    return ctx->getText();
}

std::any EvalVisitor::visitTerm(Python3Parser::TermContext *ctx) {
    auto factors = ctx->factor();
    Value result = std::any_cast<Value>(visit(factors[0]));
    
    for (size_t i = 1; i < factors.size(); i++) {
        std::string op = ctx->muldivmod_op(i - 1)->getText();
        Value right = std::any_cast<Value>(visit(factors[i]));
        
        if (op == "*") {
            if (result.type == Value::STRING && right.type == Value::INT) {
                std::string repeated;
                long long n = right.intVal.toDouble();
                for (long long i = 0; i < n; i++) {
                    repeated += result.strVal;
                }
                result = Value(repeated);
            } else if (result.type == Value::INT && right.type == Value::STRING) {
                std::string repeated;
                long long n = result.intVal.toDouble();
                for (long long i = 0; i < n; i++) {
                    repeated += right.strVal;
                }
                result = Value(repeated);
            } else if (result.type == Value::FLOAT || right.type == Value::FLOAT) {
                result = Value(result.toFloat() * right.toFloat());
            } else {
                result = Value(result.toInt() * right.toInt());
            }
        } else if (op == "/") {
            result = Value(result.toFloat() / right.toFloat());
        } else if (op == "//") {
            result = Value(result.toInt() / right.toInt());
        } else if (op == "%") {
            result = Value(result.toInt() % right.toInt());
        }
    }
    
    return result;
}

std::any EvalVisitor::visitMuldivmod_op(Python3Parser::Muldivmod_opContext *ctx) {
    return ctx->getText();
}

std::any EvalVisitor::visitFactor(Python3Parser::FactorContext *ctx) {
    if (ctx->ADD()) {
        return visit(ctx->factor());
    } else if (ctx->MINUS()) {
        Value val = std::any_cast<Value>(visit(ctx->factor()));
        if (val.type == Value::INT) {
            return Value(-val.intVal);
        } else if (val.type == Value::FLOAT) {
            return Value(-val.floatVal);
        }
        return val;
    }
    return visit(ctx->atom_expr());
}

std::any EvalVisitor::visitAtom_expr(Python3Parser::Atom_exprContext *ctx) {
    Value atom = std::any_cast<Value>(visit(ctx->atom()));
    
    // Handle function call
    if (ctx->trailer()) {
        auto trailer = ctx->trailer();
        
        // Function call
        std::string funcName = atom.type == Value::STRING ? atom.strVal : "";
        
        // Built-in functions
        if (funcName == "print") {
            std::vector<Value> args;
            if (trailer->arglist()) {
                auto arguments = trailer->arglist()->argument();
                for (auto arg : arguments) {
                    args.push_back(std::any_cast<Value>(visit(arg->test(0))));
                }
            }
            
            for (size_t i = 0; i < args.size(); i++) {
                if (i > 0) std::cout << " ";
                std::cout << args[i].toString();
            }
            std::cout << std::endl;
            
            return Value();
        } else if (funcName == "int") {
            if (trailer->arglist()) {
                Value arg = std::any_cast<Value>(visit(trailer->arglist()->argument(0)->test(0)));
                return Value(arg.toInt());
            }
            return Value(BigInteger(0));
        } else if (funcName == "float") {
            if (trailer->arglist()) {
                Value arg = std::any_cast<Value>(visit(trailer->arglist()->argument(0)->test(0)));
                return Value(arg.toFloat());
            }
            return Value(0.0);
        } else if (funcName == "str") {
            if (trailer->arglist()) {
                Value arg = std::any_cast<Value>(visit(trailer->arglist()->argument(0)->test(0)));
                return Value(arg.toString());
            }
            return Value("");
        } else if (funcName == "bool") {
            if (trailer->arglist()) {
                Value arg = std::any_cast<Value>(visit(trailer->arglist()->argument(0)->test(0)));
                return Value(arg.toBool());
            }
            return Value(false);
        }
        
        // User-defined functions
        if (functions.find(funcName) != functions.end()) {
            auto funcCtx = functions[funcName];
            
            // Create new scope
            Scope* funcScope = new Scope(currentScope);
            Scope* prevScope = currentScope;
            currentScope = funcScope;
            
            // Parse parameters
            std::vector<std::string> paramNames;
            std::map<std::string, Value> defaultValues;
            
            if (funcCtx->parameters() && funcCtx->parameters()->typedargslist()) {
                auto typedargslist = funcCtx->parameters()->typedargslist();
                auto tfpdefs = typedargslist->tfpdef();
                auto tests = typedargslist->test();
                
                for (size_t i = 0; i < tfpdefs.size(); i++) {
                    std::string paramName = tfpdefs[i]->NAME()->getText();
                    paramNames.push_back(paramName);
                    
                    // Check for default value
                    if (i >= tfpdefs.size() - tests.size()) {
                        size_t testIdx = i - (tfpdefs.size() - tests.size());
                        defaultValues[paramName] = std::any_cast<Value>(visit(tests[testIdx]));
                    }
                }
            }
            
            // Parse arguments
            std::vector<Value> positionalArgs;
            std::map<std::string, Value> keywordArgs;
            
            if (trailer->arglist()) {
                auto arguments = trailer->arglist()->argument();
                for (auto arg : arguments) {
                    if (arg->test().size() == 2) {
                        // Keyword argument
                        std::string key = arg->test(0)->getText();
                        Value value = std::any_cast<Value>(visit(arg->test(1)));
                        keywordArgs[key] = value;
                    } else {
                        // Positional argument
                        positionalArgs.push_back(std::any_cast<Value>(visit(arg->test(0))));
                    }
                }
            }
            
            // Bind arguments to parameters
            for (size_t i = 0; i < paramNames.size(); i++) {
                if (i < positionalArgs.size()) {
                    currentScope->set(paramNames[i], positionalArgs[i]);
                } else if (keywordArgs.find(paramNames[i]) != keywordArgs.end()) {
                    currentScope->set(paramNames[i], keywordArgs[paramNames[i]]);
                } else if (defaultValues.find(paramNames[i]) != defaultValues.end()) {
                    currentScope->set(paramNames[i], defaultValues[paramNames[i]]);
                }
            }
            
            // Execute function body
            Value returnValue;
            try {
                visit(funcCtx->suite());
            } catch (ReturnException& e) {
                returnValue = e.value;
            }
            
            // Restore scope
            currentScope = prevScope;
            delete funcScope;
            
            return returnValue;
        }
    }
    
    return atom;
}

std::any EvalVisitor::visitTrailer(Python3Parser::TrailerContext *ctx) {
    return nullptr;
}

std::any EvalVisitor::visitAtom(Python3Parser::AtomContext *ctx) {
    if (ctx->NUMBER()) {
        std::string numStr = ctx->NUMBER()->getText();
        if (numStr.find('.') != std::string::npos) {
            return Value(std::stod(numStr));
        } else {
            return Value(BigInteger(numStr));
        }
    } else if (ctx->TRUE()) {
        return Value(true);
    } else if (ctx->FALSE()) {
        return Value(false);
    } else if (ctx->NONE()) {
        return Value();
    } else if (ctx->NAME()) {
        std::string name = ctx->NAME()->getText();
        return currentScope->get(name);
    } else if (!ctx->STRING().empty()) {
        std::string result;
        for (auto str : ctx->STRING()) {
            std::string s = str->getText();
            // Remove quotes
            s = s.substr(1, s.length() - 2);
            result += s;
        }
        return Value(result);
    } else if (ctx->format_string()) {
        return visit(ctx->format_string());
    } else if (ctx->OPEN_PAREN()) {
        if (ctx->test()) {
            return visit(ctx->test());
        }
        // Empty tuple
        return Value(std::vector<Value>());
    }
    
    return Value();
}

std::any EvalVisitor::visitFormat_string(Python3Parser::Format_stringContext *ctx) {
    std::string result;
    
    auto literals = ctx->FORMAT_STRING_LITERAL();
    auto testlists = ctx->testlist();
    
    size_t literalIdx = 0;
    size_t testlistIdx = 0;
    
    // Iterate through children to maintain order
    for (size_t i = 0; i < ctx->children.size(); i++) {
        auto child = ctx->children[i];
        std::string childText = child->getText();
        
        if (childText == "f\"" || childText == "f'" || childText == "\"" || childText == "'") {
            // Skip quotes
            continue;
        } else if (childText == "{") {
            // Expression
            if (testlistIdx < testlists.size()) {
                Value val = std::any_cast<Value>(visit(testlists[testlistIdx++]));
                result += val.toString();
            }
        } else if (childText == "}") {
            // Skip closing brace
            continue;
        } else {
            // Literal text - unescape {{ and }}
            std::string text = childText;
            std::string unescaped;
            for (size_t j = 0; j < text.length(); j++) {
                if (j + 1 < text.length() && text[j] == '{' && text[j + 1] == '{') {
                    unescaped += '{';
                    j++;
                } else if (j + 1 < text.length() && text[j] == '}' && text[j + 1] == '}') {
                    unescaped += '}';
                    j++;
                } else {
                    unescaped += text[j];
                }
            }
            result += unescaped;
        }
    }
    
    return Value(result);
}

std::string EvalVisitor::evaluateFormatString(const std::string& fstr) {
    // This is a helper method that's not actually needed with the correct parser usage
    return fstr;
}

std::any EvalVisitor::visitTestlist(Python3Parser::TestlistContext *ctx) {
    auto tests = ctx->test();
    if (tests.size() == 1) {
        return visit(tests[0]);
    }
    
    std::vector<Value> values;
    for (auto test : tests) {
        values.push_back(std::any_cast<Value>(visit(test)));
    }
    return Value(values);
}

std::any EvalVisitor::visitArglist(Python3Parser::ArglistContext *ctx) {
    return nullptr;
}

std::any EvalVisitor::visitArgument(Python3Parser::ArgumentContext *ctx) {
    return nullptr;
}
