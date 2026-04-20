#ifndef LEXER_H
#define LEXER_H


#include <string>
#include <vector>


/*============ Token 类型编码 ============
 *  关键字   1~11
 *  标识符   20,  整常数 21
 *  运算符   30~44
 *  分隔符   50~55
 *  错误     99
 *=======================================*/
enum TokenCode {
    // 关键字
    KW_INT = 1, KW_BOOL = 2, KW_IF = 3, KW_THEN = 4, KW_ELSE = 5,
    KW_WHILE = 6, KW_DO = 7, KW_READ = 8, KW_WRITE = 9,
    KW_TRUE = 10, KW_FALSE = 11,


    // 标识符 & 整常数
    TK_ID  = 20,
    TK_NUM = 21,


    // 运算符
    OP_ADD  = 30, // +
    OP_SUB  = 31, // -
    OP_MUL  = 32, // *
    OP_DIV  = 33, // /
    OP_GT   = 34, // >
    OP_GE   = 35, // >=
    OP_LT   = 36, // <
    OP_LE   = 37, // <=
    OP_EQ   = 38, // ==
    OP_NEQ  = 39, // !=
    OP_OR   = 40, // ||
    OP_AND  = 41, // &&
    OP_NOT  = 42, // !
    OP_ASSIGN      = 43, // =   (整型赋值)
    OP_BOOL_ASSIGN = 44, // :=  (布尔型赋值)


    // 分隔符
    DL_LBRACE = 50, // {
    DL_RBRACE = 51, // }
    DL_LPAREN = 52, // (
    DL_RPAREN = 53, // )
    DL_SEMI   = 54, // ;
    DL_COMMA  = 55, // ,


    // 错误
    TK_ERR = 99
};


/* 二元组: (编码, 词素) */
struct Token {
    int         code;   // 类型编码
    std::string value;  // 词素字符串
    int         line;   // 所在行号
};


/* 词法分析器 — 工作模块 */
class Lexer {
public:
    // 核心接口：输入源码字符串，输出 Token 序列
    std::vector<Token> tokenize(const std::string& source);


    std::string getErrors() const { return errors_; }
    bool hasError() const { return !errors_.empty(); }


private:
    std::string src_;
    int pos_  = 0;
    int line_ = 1;
    std::string errors_;


    char cur()  const;          // 当前字符
    char peek() const;          // 下一个字符
    void next();                // 前进一步
    void skipWhitespace();      // 跳过空白
    bool skipComment();         // 跳过注释，返回是否成功跳过
    Token readNumber();         // 读整常数
    Token readWord();           // 读标识符 / 关键字
    void addError(const std::string& msg);
};


/* 辅助：编码 → 可读类型名 */
std::string tokenCodeName(int code);


#endif // LEXER_H
