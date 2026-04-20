#ifndef PARSER_H
#define PARSER_H


#include "../lexer/lexer.h"
#include <string>
#include <vector>
#include <memory>


/* ======== AST 节点类型 ======== */
enum class NodeType {
    Program,                    // 程序根节点
    DeclList,                   // 声明列表
    StmtList,                   // 语句列表
    VarDecl,                    // 变量声明       value="int"/"bool"
    IntAssign,                  // 整型赋值 id=E  value=变量名
    BoolAssign,                 // 布尔赋值 id:=B value=变量名
    IfStmt,                     // if            value=条件变量名
    WhileStmt,                  // while         value=条件变量名
    Block,                      // 复合语句 { }
    ReadStmt,                   // read id       value=变量名
    WriteStmt,                  // write id      value=变量名
    BinOp,                      // 二元运算       value=运算符
    UnaryOp,                    // 一元运算       value="-"或"!"
    Number,                     // 整常数         value=数值
    BoolLit,                    // 布尔常量       value="true"/"false"
    Ident,                      // 标识符引用     value=变量名
    Name,                       // 声明中的名字   value=变量名
};


/* ======== AST 节点 ======== */
struct ASTNode {
    NodeType    type;
    std::string value;
    int         line = 0;
    std::vector<std::shared_ptr<ASTNode>> children;


    ASTNode(NodeType t, const std::string& v = "", int ln = 0)
        : type(t), value(v), line(ln) {}
    void addChild(std::shared_ptr<ASTNode> ch) {
        if (ch) children.push_back(ch);
    }
};


using ASTPtr = std::shared_ptr<ASTNode>;


inline ASTPtr makeNode(NodeType t, const std::string& v = "", int ln = 0) {
    return std::make_shared<ASTNode>(t, v, ln);
}


/* ======== 语法分析器 ======== */
class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);


    ASTPtr parse();                              // 入口
    std::string getErrors() const { return errors_; }
    bool hasError() const { return !errors_.empty(); }


    static std::string astToString(ASTPtr root); // AST → 可视化文本


private:
    std::vector<Token> tokens_;
    int pos_;
    std::string errors_;


    /* Token 访问 */
    const Token& cur()  const;
    const Token& look() const;          // 向前看 1 个
    void   advance();
    bool   check(int code)   const;
    bool   match(int code);
    bool   expect(int code, const std::string& msg);
    void   error(const std::string& msg);
    bool   isRelOp()         const;
    bool   canStartStmt()    const;


    /* 递归下降 */
    ASTPtr parseProgram();               // PROG → { DECLS STMTS }
    ASTPtr parseDeclList();              // DECLS → (DECL)*
    ASTPtr parseDecl();                  // DECL → int/bool NAMES ;
    ASTPtr parseStmtList();              // STMTS → (STMT)+
    ASTPtr parseStmt();
    ASTPtr parseBlock();                 // { STMTS }


    /* 算术表达式 */
    ASTPtr parseExpr();                  // EXPR → TERM ((+|-) TERM)*
    ASTPtr parseTerm();                  // TERM → NEGA ((*|/) NEGA)*
    ASTPtr parseNega();                  // NEGA → -FACTOR | FACTOR
    ASTPtr parseFactor();                // FACTOR → (EXPR) | id | number


    /* 布尔表达式 */
    ASTPtr parseBoolExpr();              // BOOL → JOIN (|| JOIN)*
    ASTPtr parseJoin();                  // JOIN → NOT (&& NOT)*
    ASTPtr parseNot();                   // NOT  → !REL | REL | true | false
    ASTPtr parseRel();                   // REL  → EXPR ROP EXPR | id


    /* 打印辅助 */
    static std::string typeName(NodeType t);
    static std::string describe(ASTPtr node);
    static void drawTree(ASTPtr node, const std::string& prefix,
                         bool isLast, std::string& out);
};


#endif // PARSER_H
