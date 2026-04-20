#ifndef SEMANTIC_H
#define SEMANTIC_H


#include "../parser/parser.h"
#include <string>
#include <vector>
#include <unordered_map>


/* ======== 语义分析器 ======== */
class SemanticAnalyzer {
public:
    bool analyze(ASTPtr root);
    std::string getErrors()        const { return errors_; }
    bool hasError()                const { return errCnt_ > 0; }
    std::string getSymbolTableStr()const;
    const std::unordered_map<std::string, std::string>& symTable() const
    { return sym_; }


private:
    std::unordered_map<std::string, std::string> sym_;   // name → "int"/"bool"
    std::unordered_map<std::string, int> symLine_;       // name → 声明行号
    std::string errors_;
    int errCnt_ = 0;


    void err(int line, const std::string& code, const std::string& msg);
    void buildSymbol(ASTPtr declList);
    void checkStmtList(ASTPtr list);
    void checkStmt(ASTPtr s);
    std::string infer(ASTPtr expr);   // 返回 "int"/"bool"/"error"
};


/* ======== 8086 汇编代码生成器 ======== */
class CodeGen8086 {
public:
    std::string generate(ASTPtr root,
        const std::unordered_map<std::string, std::string>& sym);


private:
    std::unordered_map<std::string, std::string> sym_;
    std::vector<std::string> out_;
    int lbl_ = 0;
    bool useRead_  = false;
    bool useWrite_ = false;


    std::string L();                      // 生成新标签
    void emit(const std::string& s);
    void genStmtList(ASTPtr n);
    void genStmt(ASTPtr n);
    void genExpr(ASTPtr n);               // 结果存 AX
    void emitReadProc();
    void emitWriteProc();
};


#endif
