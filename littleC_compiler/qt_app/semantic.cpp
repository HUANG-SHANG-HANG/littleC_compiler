#include "semantic.h"
#include <fstream>
#include <iostream>
#include <sstream>


/* ============================================================
 *               语义分析器实现
 * ============================================================ */


void SemanticAnalyzer::err(int line, const std::string& code,
                           const std::string& msg) {
    errors_ += "  Line " + std::to_string(line)
             + ": [" + code + "] " + msg + "\n";
    errCnt_++;
}


bool SemanticAnalyzer::analyze(ASTPtr root) {
    sym_.clear(); symLine_.clear(); errors_.clear(); errCnt_ = 0;
    if (!root || root->children.size() < 2) {
        err(0, "E05", "Invalid program structure");
        return false;
    }
    buildSymbol(root->children[0]);    // DeclList
    checkStmtList(root->children[1]);  // StmtList
    return errCnt_ == 0;
}


/* 从 DeclList 构建符号表 */
void SemanticAnalyzer::buildSymbol(ASTPtr declList) {
    if (!declList) return;
    for (auto& decl : declList->children) {          // 每条 VarDecl
        std::string tp = decl->value;                 // "int" 或 "bool"
        for (auto& name : decl->children) {           // 每个 Name
            if (sym_.count(name->value)) {
                err(name->line, "E06",
                    "Variable '" + name->value + "' redeclared "
                    "(first at line " + std::to_string(symLine_[name->value]) + ")");
            } else {
                sym_[name->value]     = tp;
                symLine_[name->value] = name->line;
            }
        }
    }
}


/* 检查语句列表 */
void SemanticAnalyzer::checkStmtList(ASTPtr list) {
    if (!list) return;
    for (auto& c : list->children) checkStmt(c);
}


/* 检查单条语句 */
void SemanticAnalyzer::checkStmt(ASTPtr s) {
    if (!s) return;


    switch (s->type) {
    /* ---- id = EXPR ; ---- */
    case NodeType::IntAssign: {
        auto it = sym_.find(s->value);
        if (it == sym_.end())
            err(s->line, "E07", "Undeclared variable '" + s->value + "'");
        else if (it->second != "int")
            err(s->line, "E08",
                "Cannot use '=' on bool variable '" + s->value + "'");
        if (!s->children.empty()) {
            std::string t = infer(s->children[0]);
            if (t != "int" && t != "error")
                err(s->line, "E08",
                    "Right side of '=' is not int type");
        }
        break;
    }
    /* ---- id := BOOL ; ---- */
    case NodeType::BoolAssign: {
        auto it = sym_.find(s->value);
        if (it == sym_.end())
            err(s->line, "E07", "Undeclared variable '" + s->value + "'");
        else if (it->second != "bool")
            err(s->line, "E09",
                "Cannot use ':=' on int variable '" + s->value + "'");
        if (!s->children.empty()) {
            std::string t = infer(s->children[0]);
            if (t != "bool" && t != "error")
                err(s->line, "E09",
                    "Right side of ':=' is not bool type");
        }
        break;
    }
    /* ---- if id then S [else S] ---- */
    case NodeType::IfStmt: {
        auto it = sym_.find(s->value);
        if (it == sym_.end())
            err(s->line, "E07", "Undeclared variable '" + s->value + "'");
        else if (it->second != "bool")
            err(s->line, "E13",
                "If condition '" + s->value + "' is not bool");
        for (auto& c : s->children) checkStmt(c);
        break;
    }
    /* ---- while id do S ---- */
    case NodeType::WhileStmt: {
        auto it = sym_.find(s->value);
        if (it == sym_.end())
            err(s->line, "E07", "Undeclared variable '" + s->value + "'");
        else if (it->second != "bool")
            err(s->line, "E13",
                "While condition '" + s->value + "' is not bool");
        for (auto& c : s->children) checkStmt(c);
        break;
    }
    /* ---- { STMTS } ---- */
    case NodeType::Block:
        for (auto& c : s->children) checkStmt(c);
        break;
    /* ---- read id ; ---- */
    case NodeType::ReadStmt: {
        auto it = sym_.find(s->value);
        if (it == sym_.end())
            err(s->line, "E07", "Undeclared variable '" + s->value + "'");
        else if (it->second != "int")
            err(s->line, "E14",
                "Read target '" + s->value + "' is not int");
        break;
    }
    /* ---- write id ; ---- */
    case NodeType::WriteStmt: {
        auto it = sym_.find(s->value);
        if (it == sym_.end())
            err(s->line, "E07", "Undeclared variable '" + s->value + "'");
        else if (it->second != "int")
            err(s->line, "E15",
                "Write target '" + s->value + "' is not int");
        break;
    }
    default: break;
    }
}


/* 推导表达式类型 */
std::string SemanticAnalyzer::infer(ASTPtr e) {
    if (!e) return "error";


    if (e->type == NodeType::Number)  return "int";
    if (e->type == NodeType::BoolLit) return "bool";


    if (e->type == NodeType::Ident) {
        auto it = sym_.find(e->value);
        if (it == sym_.end()) {
            err(e->line, "E07", "Undeclared variable '" + e->value + "'");
            return "error";
        }
        return it->second;
    }


    if (e->type == NodeType::UnaryOp) {
        std::string ct = infer(e->children[0]);
        if (e->value == "-") {
            if (ct != "int" && ct != "error")
                err(e->line, "E10", "Operand of '-' is not int");
            return "int";
        }
        if (e->value == "!") {
            if (ct != "bool" && ct != "error")
                err(e->line, "E11", "Operand of '!' is not bool");
            return "bool";
        }
    }


    if (e->type == NodeType::BinOp) {
        std::string op = e->value;
        std::string lt = infer(e->children[0]);
        std::string rt = infer(e->children[1]);


        // 算术: + - * /
        if (op == "+" || op == "-" || op == "*" || op == "/") {
            if (lt != "int" && lt != "error")
                err(e->line, "E10", "Left of '" + op + "' is not int");
            if (rt != "int" && rt != "error")
                err(e->line, "E10", "Right of '" + op + "' is not int");
            return "int";
        }
        // 关系: > >= < <= == !=
        if (op==">" || op==">=" || op=="<" || op=="<=" || op=="==" || op=="!=") {
            if (lt != "int" && lt != "error")
                err(e->line, "E12", "Left of '" + op + "' is not int");
            if (rt != "int" && rt != "error")
                err(e->line, "E12", "Right of '" + op + "' is not int");
            return "bool";
        }
        // 逻辑: || &&
        if (op == "||" || op == "&&") {
            if (lt != "bool" && lt != "error")
                err(e->line, "E11", "Left of '" + op + "' is not bool");
            if (rt != "bool" && rt != "error")
                err(e->line, "E11", "Right of '" + op + "' is not bool");
            return "bool";
        }
    }
    return "error";
}


std::string SemanticAnalyzer::getSymbolTableStr() const {
    std::string r = "Name      Type    Line\n";
    r +=            "----      ----    ----\n";
    for (auto& [name, tp] : sym_) {
        r += name;
        r += std::string(10 - (int)name.size(), ' ');
        r += tp;
        r += std::string(8 - (int)tp.size(), ' ');
        if (symLine_.count(name))
            r += std::to_string(symLine_.at(name));
        r += "\n";
    }
    return r;
}


/* ============================================================
 *               8086 汇编代码生成器
 * ============================================================ */


std::string CodeGen8086::L() { return "_L" + std::to_string(lbl_++); }


void CodeGen8086::emitCode(const std::string& s) { out_.push_back(s); }


std::string CodeGen8086::generate(
    ASTPtr root,
    const std::unordered_map<std::string, std::string>& sym)
{
    sym_ = sym; out_.clear(); lbl_ = 0;
    useRead_ = false; useWrite_ = false;


    emitCode("; ==========================================");
    emitCode("; LittleC -> 8086 Assembly  (MASM format)");
    emitCode("; ==========================================");
    emitCode(".MODEL SMALL");
    emitCode(".STACK 256");
    emitCode("");
    emitCode(".DATA");


    // 声明变量 (统一用 DW, bool 也用 16 位存 0/1)
    ASTPtr decls = root->children[0];
    for (auto& decl : decls->children) {
        std::string comment = "; " + decl->value;
        for (auto& nm : decl->children)
            emitCode("    " + nm->value + " DW 0        " + comment);
    }


    emitCode("");
    emitCode(".CODE");
    emitCode("MAIN PROC");
    emitCode("    MOV AX, @DATA");
    emitCode("    MOV DS, AX");
    emitCode("");


    genStmtList(root->children[1]);


    emitCode("");
    emitCode("    ; exit program");
    emitCode("    MOV AH, 4CH");
    emitCode("    INT 21H");
    emitCode("MAIN ENDP");
    emitCode("");


    if (useRead_)  emitReadProc();
    if (useWrite_) emitWriteProc();


    emitCode("END MAIN");


    std::string result;
    for (auto& line : out_) result += line + "\n";
    return result;
}


void CodeGen8086::genStmtList(ASTPtr n) {
    if (!n) return;
    for (auto& c : n->children) genStmt(c);
}


void CodeGen8086::genStmt(ASTPtr n) {
    if (!n) return;


    switch (n->type) {
    case NodeType::IntAssign:
    case NodeType::BoolAssign: {
        emitCode("    ; " + n->value + " " +
             (n->type == NodeType::IntAssign ? "=" : ":=") + " ...");
        genExpr(n->children[0]);
        emitCode("    MOV " + n->value + ", AX");
        break;
    }
    case NodeType::IfStmt: {
        emitCode("    ; if " + n->value + " then ...");
        emitCode("    MOV AX, " + n->value);
        emitCode("    CMP AX, 0");
        if (n->children.size() > 1) {
            std::string le = L(), lend = L();
            emitCode("    JE " + le);
            genStmt(n->children[0]);
            emitCode("    JMP " + lend);
            emitCode(le + ":");
            genStmt(n->children[1]);
            emitCode(lend + ":");
        } else {
            std::string lend = L();
            emitCode("    JE " + lend);
            genStmt(n->children[0]);
            emitCode(lend + ":");
        }
        break;
    }
    case NodeType::WhileStmt: {
        std::string ls = L(), le = L();
        emitCode("    ; while " + n->value + " do ...");
        emitCode(ls + ":");
        emitCode("    MOV AX, " + n->value);
        emitCode("    CMP AX, 0");
        emitCode("    JE " + le);
        genStmt(n->children[0]);
        emitCode("    JMP " + ls);
        emitCode(le + ":");
        break;
    }
    case NodeType::Block:
        for (auto& c : n->children) genStmt(c);
        break;
    case NodeType::ReadStmt:
        useRead_ = true;
        emitCode("    ; read " + n->value);
        emitCode("    CALL ReadInt");
        emitCode("    MOV " + n->value + ", AX");
        break;
    case NodeType::WriteStmt:
        useWrite_ = true;
        emitCode("    ; write " + n->value);
        emitCode("    MOV AX, " + n->value);
        emitCode("    CALL WriteInt");
        break;
    default: break;
    }
}


/* 表达式求值 → 结果在 AX */
void CodeGen8086::genExpr(ASTPtr n) {
    if (!n) return;


    if (n->type == NodeType::Number) {
        emitCode("    MOV AX, " + n->value);
        return;
    }
    if (n->type == NodeType::BoolLit) {
        emitCode("    MOV AX, " + std::string(n->value == "true" ? "1" : "0"));
        return;
    }
    if (n->type == NodeType::Ident) {
        emitCode("    MOV AX, " + n->value);
        return;
    }
    if (n->type == NodeType::UnaryOp) {
        genExpr(n->children[0]);
        if (n->value == "-")  emitCode("    NEG AX");
        if (n->value == "!")  emitCode("    XOR AX, 1");
        return;
    }
    if (n->type == NodeType::BinOp) {
        std::string op = n->value;


        genExpr(n->children[0]);          // left → AX
        emitCode("    PUSH AX");
        genExpr(n->children[1]);          // right → AX
        emitCode("    POP BX");              // BX=left, AX=right
        emitCode("    XCHG AX, BX");         // AX=left, BX=right


        if      (op == "+") emitCode("    ADD AX, BX");
        else if (op == "-") emitCode("    SUB AX, BX");
        else if (op == "*") emitCode("    IMUL BX");
        else if (op == "/") { emitCode("    CWD"); emitCode("    IDIV BX"); }
        else if (op == "||") emitCode("    OR AX, BX");
        else if (op == "&&") emitCode("    AND AX, BX");
        else {
            // 关系运算符: > >= < <= == !=
            std::string lt = L(), le = L();
            emitCode("    CMP AX, BX");
            std::string jmp;
            if      (op == ">")  jmp = "JG";
            else if (op == ">=") jmp = "JGE";
            else if (op == "<")  jmp = "JL";
            else if (op == "<=") jmp = "JLE";
            else if (op == "==") jmp = "JE";
            else if (op == "!=") jmp = "JNE";
            emitCode("    " + jmp + " " + lt);
            emitCode("    MOV AX, 0");
            emitCode("    JMP " + le);
            emitCode(lt + ":");
            emitCode("    MOV AX, 1");
            emitCode(le + ":");
        }
        return;
    }
}


/* ---------- ReadInt 子程序 ---------- */
void CodeGen8086::emitReadProc() {
    emitCode("; ---- Read signed int from keyboard -> AX ----");
    emitCode("ReadInt PROC");
    emitCode("    PUSH BX");
    emitCode("    PUSH CX");
    emitCode("    PUSH DX");
    emitCode("    XOR BX, BX");
    emitCode("    XOR CX, CX");
    emitCode("    MOV AH, 01H");
    emitCode("    INT 21H");
    emitCode("    CMP AL, '-'");
    emitCode("    JNE RI_D");
    emitCode("    MOV CX, 1");
    emitCode("    MOV AH, 01H");
    emitCode("    INT 21H");
    emitCode("RI_D:");
    emitCode("    CMP AL, '0'");
    emitCode("    JB RI_END");
    emitCode("    CMP AL, '9'");
    emitCode("    JA RI_END");
    emitCode("    SUB AL, '0'");
    emitCode("    XOR AH, AH");
    emitCode("    XCHG AX, BX");
    emitCode("    MOV DX, 10");
    emitCode("    MUL DX");
    emitCode("    ADD AX, BX");
    emitCode("    MOV BX, AX");
    emitCode("    MOV AH, 01H");
    emitCode("    INT 21H");
    emitCode("    JMP RI_D");
    emitCode("RI_END:");
    emitCode("    MOV AX, BX");
    emitCode("    CMP CX, 0");
    emitCode("    JE RI_RET");
    emitCode("    NEG AX");
    emitCode("RI_RET:");
    emitCode("    POP DX");
    emitCode("    POP CX");
    emitCode("    POP BX");
    emitCode("    RET");
    emitCode("ReadInt ENDP");
    emitCode("");
}


/* ---------- WriteInt 子程序 ---------- */
void CodeGen8086::emitWriteProc() {
    emitCode("; ---- Write signed int in AX to screen ----");
    emitCode("WriteInt PROC");
    emitCode("    PUSH AX");
    emitCode("    PUSH BX");
    emitCode("    PUSH CX");
    emitCode("    PUSH DX");
    emitCode("    XOR CX, CX");
    emitCode("    CMP AX, 0");
    emitCode("    JGE WI_POS");
    emitCode("    PUSH AX");
    emitCode("    MOV DL, '-'");
    emitCode("    MOV AH, 02H");
    emitCode("    INT 21H");
    emitCode("    POP AX");
    emitCode("    NEG AX");
    emitCode("WI_POS:");
    emitCode("    MOV BX, 10");
    emitCode("WI_LP:");
    emitCode("    XOR DX, DX");
    emitCode("    DIV BX");
    emitCode("    PUSH DX");
    emitCode("    INC CX");
    emitCode("    CMP AX, 0");
    emitCode("    JNE WI_LP");
    emitCode("WI_PR:");
    emitCode("    POP DX");
    emitCode("    ADD DL, '0'");
    emitCode("    MOV AH, 02H");
    emitCode("    INT 21H");
    emitCode("    LOOP WI_PR");
    emitCode("    MOV DL, 0DH");
    emitCode("    MOV AH, 02H");
    emitCode("    INT 21H");
    emitCode("    MOV DL, 0AH");
    emitCode("    INT 21H");
    emitCode("    POP DX");
    emitCode("    POP CX");
    emitCode("    POP BX");
    emitCode("    POP AX");
    emitCode("    RET");
    emitCode("WriteInt ENDP");
    emitCode("");
}


