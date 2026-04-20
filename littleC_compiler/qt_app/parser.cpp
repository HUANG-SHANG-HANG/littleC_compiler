#include "parser.h"
#include <fstream>
#include <iostream>
#include <sstream>


/* ============================================================
 *                  Token 访问辅助
 * ============================================================ */
static Token eofTok{-1, "EOF", 0};   // 哨兵


const Token& Parser::cur() const {
    if (pos_ >= (int)tokens_.size()) return eofTok;
    return tokens_[pos_];
}
const Token& Parser::look() const {
    if (pos_ + 1 >= (int)tokens_.size()) return eofTok;
    return tokens_[pos_ + 1];
}
void Parser::advance() {
    if (pos_ < (int)tokens_.size()) pos_++;
}
bool Parser::check(int code) const { return cur().code == code; }
bool Parser::match(int code) {
    if (check(code)) { advance(); return true; }
    return false;
}
bool Parser::expect(int code, const std::string& msg) {
    if (check(code)) { advance(); return true; }
    error(msg + " (got '" + cur().value + "')");
    return false;
}
void Parser::error(const std::string& msg) {
    errors_ += "Line " + std::to_string(cur().line) + ": " + msg + "\n";
}


bool Parser::isRelOp() const {
    int c = cur().code;
    return c == OP_GT || c == OP_GE || c == OP_LT ||
           c == OP_LE || c == OP_EQ || c == OP_NEQ;
}
bool Parser::canStartStmt() const {
    int c = cur().code;
    return c == TK_ID || c == KW_IF || c == KW_WHILE ||
           c == DL_LBRACE || c == KW_READ || c == KW_WRITE;
}


Parser::Parser(const std::vector<Token>& tokens)
    : tokens_(tokens), pos_(0) {}


/* ============================================================
 *                  递归下降语法分析
 * ============================================================ */


ASTPtr Parser::parse() { return parseProgram(); }


// PROG → { DECLS STMTS }
ASTPtr Parser::parseProgram() {
    auto prog = makeNode(NodeType::Program, "program", cur().line);
    expect(DL_LBRACE, "Expected '{'");


    prog->addChild(parseDeclList());
    prog->addChild(parseStmtList());


    expect(DL_RBRACE, "Expected '}'");
    return prog;
}


// DECLS → (DECL)*     (零或多条声明)
ASTPtr Parser::parseDeclList() {
    auto list = makeNode(NodeType::DeclList, "", cur().line);
    while (check(KW_INT) || check(KW_BOOL)) {
        list->addChild(parseDecl());
    }
    return list;
}


// DECL → int NAMES ; | bool NAMES ;
// NAMES → NAME (, NAME)*
ASTPtr Parser::parseDecl() {
    int ln = cur().line;
    std::string typeName = cur().value;            // "int" 或 "bool"
    advance();                                      // 跳过关键字


    auto decl = makeNode(NodeType::VarDecl, typeName, ln);


    // 第一个名字
    if (check(TK_ID)) {
        decl->addChild(makeNode(NodeType::Name, cur().value, cur().line));
        advance();
    } else {
        error("Expected variable name");
    }
    // 后续名字 (, NAME)*
    while (match(DL_COMMA)) {
        if (check(TK_ID)) {
            decl->addChild(makeNode(NodeType::Name, cur().value, cur().line));
            advance();
        } else {
            error("Expected variable name after ','");
        }
    }
    expect(DL_SEMI, "Expected ';' after declaration");
    return decl;
}


// STMTS → STMT+     (一条或多条语句)
ASTPtr Parser::parseStmtList() {
    auto list = makeNode(NodeType::StmtList, "", cur().line);
    if (!canStartStmt()) {
        error("Expected at least one statement");
    }
    while (canStartStmt()) {
        list->addChild(parseStmt());
    }
    return list;
}


// STMT → 各种形式
ASTPtr Parser::parseStmt() {
    int ln = cur().line;


    // ---- id = EXPR ;  或  id := BOOL ; ----
    if (check(TK_ID)) {
        std::string name = cur().value;
        // 向前看一个 token 决定是 = 还是 :=
        if (look().code == OP_ASSIGN) {
            advance();              // 跳过 id
            advance();              // 跳过 =
            auto node = makeNode(NodeType::IntAssign, name, ln);
            node->addChild(parseExpr());
            expect(DL_SEMI, "Expected ';'");
            return node;
        }
        if (look().code == OP_BOOL_ASSIGN) {
            advance();              // 跳过 id
            advance();              // 跳过 :=
            auto node = makeNode(NodeType::BoolAssign, name, ln);
            node->addChild(parseBoolExpr());
            expect(DL_SEMI, "Expected ';'");
            return node;
        }
        error("Expected '=' or ':=' after identifier '" + name + "'");
        advance();
        return nullptr;
    }


    // ---- if id then STMT [else STMT] ----
    if (match(KW_IF)) {
        std::string condVar;
        if (check(TK_ID)) {
            condVar = cur().value;
            advance();
        } else {
            error("Expected identifier after 'if'");
        }
        expect(KW_THEN, "Expected 'then'");
        auto node = makeNode(NodeType::IfStmt, condVar, ln);
        node->addChild(parseStmt());               // then 分支
        if (match(KW_ELSE)) {
            node->addChild(parseStmt());            // else 分支
        }
        return node;
    }


    // ---- while id do STMT ----
    if (match(KW_WHILE)) {
        std::string condVar;
        if (check(TK_ID)) {
            condVar = cur().value;
            advance();
        } else {
            error("Expected identifier after 'while'");
        }
        expect(KW_DO, "Expected 'do'");
        auto node = makeNode(NodeType::WhileStmt, condVar, ln);
        node->addChild(parseStmt());
        return node;
    }


    // ---- { STMTS } (复合语句) ----
    if (check(DL_LBRACE)) {
        return parseBlock();
    }


    // ---- read id ; ----
    if (match(KW_READ)) {
        std::string var;
        if (check(TK_ID)) { var = cur().value; advance(); }
        else error("Expected identifier after 'read'");
        expect(DL_SEMI, "Expected ';'");
        return makeNode(NodeType::ReadStmt, var, ln);
    }


    // ---- write id ; ----
    if (match(KW_WRITE)) {
        std::string var;
        if (check(TK_ID)) { var = cur().value; advance(); }
        else error("Expected identifier after 'write'");
        expect(DL_SEMI, "Expected ';'");
        return makeNode(NodeType::WriteStmt, var, ln);
    }


    error("Unexpected token '" + cur().value + "'");
    advance();
    return nullptr;
}


// { STMTS }  (复合语句, 内部无声明)
ASTPtr Parser::parseBlock() {
    auto blk = makeNode(NodeType::Block, "", cur().line);
    expect(DL_LBRACE, "Expected '{'");
    while (!check(DL_RBRACE) && cur().code != -1) {
        if (canStartStmt()) {
            blk->addChild(parseStmt());
        } else {
            error("Unexpected token in block");
            advance();
        }
    }
    expect(DL_RBRACE, "Expected '}'");
    return blk;
}


/* ============================================================
 *                  算术表达式
 * ============================================================ */


// EXPR → TERM ((+|-) TERM)*
ASTPtr Parser::parseExpr() {
    auto left = parseTerm();
    while (check(OP_ADD) || check(OP_SUB)) {
        std::string op = cur().value;
        int ln = cur().line;
        advance();
        auto right = parseTerm();
        auto node = makeNode(NodeType::BinOp, op, ln);
        node->addChild(left);
        node->addChild(right);
        left = node;
    }
    return left;
}


// TERM → NEGA ((*|/) NEGA)*
ASTPtr Parser::parseTerm() {
    auto left = parseNega();
    while (check(OP_MUL) || check(OP_DIV)) {
        std::string op = cur().value;
        int ln = cur().line;
        advance();
        auto right = parseNega();
        auto node = makeNode(NodeType::BinOp, op, ln);
        node->addChild(left);
        node->addChild(right);
        left = node;
    }
    return left;
}


// NEGA → - FACTOR | FACTOR
ASTPtr Parser::parseNega() {
    if (check(OP_SUB)) {
        int ln = cur().line;
        advance();
        auto node = makeNode(NodeType::UnaryOp, "-", ln);
        node->addChild(parseFactor());
        return node;
    }
    return parseFactor();
}


// FACTOR → ( EXPR ) | id | number
ASTPtr Parser::parseFactor() {
    if (match(DL_LPAREN)) {
        auto expr = parseExpr();
        expect(DL_RPAREN, "Expected ')'");
        return expr;
    }
    if (check(TK_ID)) {
        auto node = makeNode(NodeType::Ident, cur().value, cur().line);
        advance();
        return node;
    }
    if (check(TK_NUM)) {
        auto node = makeNode(NodeType::Number, cur().value, cur().line);
        advance();
        return node;
    }
    error("Expected expression, got '" + cur().value + "'");
    advance();
    return makeNode(NodeType::Number, "0", cur().line);   // 错误恢复
}


/* ============================================================
 *                  布尔表达式
 * ============================================================ */


// BOOL → JOIN (|| JOIN)*
ASTPtr Parser::parseBoolExpr() {
    auto left = parseJoin();
    while (check(OP_OR)) {
        int ln = cur().line;
        advance();
        auto right = parseJoin();
        auto node = makeNode(NodeType::BinOp, "||", ln);
        node->addChild(left);
        node->addChild(right);
        left = node;
    }
    return left;
}


// JOIN → NOT (&& NOT)*
ASTPtr Parser::parseJoin() {
    auto left = parseNot();
    while (check(OP_AND)) {
        int ln = cur().line;
        advance();
        auto right = parseNot();
        auto node = makeNode(NodeType::BinOp, "&&", ln);
        node->addChild(left);
        node->addChild(right);
        left = node;
    }
    return left;
}


// NOT → ! REL | REL | true | false
// (扩展: 允许 ! 后跟 true/false/id 以及 REL)
ASTPtr Parser::parseNot() {
    if (check(OP_NOT)) {
        int ln = cur().line;
        advance();
        auto node = makeNode(NodeType::UnaryOp, "!", ln);
        node->addChild(parseRel());       // ! 后面跟 REL 或 布尔基元
        return node;
    }
    // true / false
    if (check(KW_TRUE)) {
        auto node = makeNode(NodeType::BoolLit, "true", cur().line);
        advance(); return node;
    }
    if (check(KW_FALSE)) {
        auto node = makeNode(NodeType::BoolLit, "false", cur().line);
        advance(); return node;
    }
    return parseRel();
}


// REL → EXPR ROP EXPR  |  (如果后面没有 ROP, 退化为单个 EXPR / id)
ASTPtr Parser::parseRel() {
    // 先检查 true/false (可能出现在 ! 后面)
    if (check(KW_TRUE)) {
        auto n = makeNode(NodeType::BoolLit, "true", cur().line);
        advance(); return n;
    }
    if (check(KW_FALSE)) {
        auto n = makeNode(NodeType::BoolLit, "false", cur().line);
        advance(); return n;
    }


    auto left = parseExpr();
    if (isRelOp()) {
        std::string op = cur().value;
        int ln = cur().line;
        advance();
        auto right = parseExpr();
        auto node = makeNode(NodeType::BinOp, op, ln);
        node->addChild(left);
        node->addChild(right);
        return node;
    }
    // 没有关系运算符 → 可能是一个布尔变量, 直接返回
    return left;
}


/* ============================================================
 *                  AST 可视化输出
 * ============================================================ */


std::string Parser::typeName(NodeType t) {
    switch (t) {
        case NodeType::Program:    return "Program";
        case NodeType::DeclList:   return "DeclList";
        case NodeType::StmtList:   return "StmtList";
        case NodeType::VarDecl:    return "VarDecl";
        case NodeType::IntAssign:  return "IntAssign";
        case NodeType::BoolAssign: return "BoolAssign";
        case NodeType::IfStmt:     return "IfStmt";
        case NodeType::WhileStmt:  return "WhileStmt";
        case NodeType::Block:      return "Block";
        case NodeType::ReadStmt:   return "ReadStmt";
        case NodeType::WriteStmt:  return "WriteStmt";
        case NodeType::BinOp:      return "BinOp";
        case NodeType::UnaryOp:    return "UnaryOp";
        case NodeType::Number:     return "Number";
        case NodeType::BoolLit:    return "BoolLit";
        case NodeType::Ident:      return "Ident";
        case NodeType::Name:       return "Name";
    }
    return "?";
}


// 生成每个节点的描述文字
std::string Parser::describe(ASTPtr node) {
    if (!node) return "(null)";
    std::string s = typeName(node->type);
    switch (node->type) {
        case NodeType::VarDecl:
            s += " (" + node->value + ")";  break;    // VarDecl (int)
        case NodeType::IntAssign:
        case NodeType::BoolAssign:
            s += " -> " + node->value;      break;    // IntAssign -> a
        case NodeType::IfStmt:
            s += " [cond: " + node->value + "]"; break;
        case NodeType::WhileStmt:
            s += " [cond: " + node->value + "]"; break;
        case NodeType::ReadStmt:
        case NodeType::WriteStmt:
            s += " (" + node->value + ")";  break;
        case NodeType::BinOp:
        case NodeType::UnaryOp:
            s += " (" + node->value + ")";  break;    // BinOp (+)
        case NodeType::Number:
        case NodeType::BoolLit:
        case NodeType::Ident:
        case NodeType::Name:
            s += ": " + node->value;        break;    // Ident: a
        default: break;
    }
    if (node->line > 0) s += "  [line " + std::to_string(node->line) + "]";
    return s;
}


// 递归画树  (├── └── │   )
void Parser::drawTree(ASTPtr node, const std::string& prefix,
                      bool isLast, std::string& out) {
    if (!node) return;
    out += prefix;
    out += isLast ? "\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80 "   // └──
                  : "\xe2\x94\x9c\xe2\x94\x80\xe2\x94\x80 ";  // ├──
    out += describe(node) + "\n";


    std::string childPrefix = prefix +
        (isLast ? "    "
                : "\xe2\x94\x82   ");                          // │
    for (int i = 0; i < (int)node->children.size(); i++) {
        bool last = (i == (int)node->children.size() - 1);
        drawTree(node->children[i], childPrefix, last, out);
    }
}


std::string Parser::astToString(ASTPtr root) {
    if (!root) return "(empty)\n";
    std::string out;
    out += describe(root) + "\n";
    for (int i = 0; i < (int)root->children.size(); i++) {
        bool last = (i == (int)root->children.size() - 1);
        drawTree(root->children[i], "", last, out);
    }
    return out;
}


