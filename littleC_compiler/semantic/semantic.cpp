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


void CodeGen8086::emit(const std::string& s) { out_.push_back(s); }


std::string CodeGen8086::generate(
    ASTPtr root,
    const std::unordered_map<std::string, std::string>& sym)
{
    sym_ = sym; out_.clear(); lbl_ = 0;
    useRead_ = false; useWrite_ = false;


    emit("; ==========================================");
    emit("; LittleC -> 8086 Assembly  (MASM format)");
    emit("; ==========================================");
    emit(".MODEL SMALL");
    emit(".STACK 256");
    emit("");
    emit(".DATA");


    // 声明变量 (统一用 DW, bool 也用 16 位存 0/1)
    ASTPtr decls = root->children[0];
    for (auto& decl : decls->children) {
        std::string comment = "; " + decl->value;
        for (auto& nm : decl->children)
            emit("    " + nm->value + " DW 0        " + comment);
    }


    emit("");
    emit(".CODE");
    emit("MAIN PROC");
    emit("    MOV AX, @DATA");
    emit("    MOV DS, AX");
    emit("");


    genStmtList(root->children[1]);


    emit("");
    emit("    ; exit program");
    emit("    MOV AH, 4CH");
    emit("    INT 21H");
    emit("MAIN ENDP");
    emit("");


    if (useRead_)  emitReadProc();
    if (useWrite_) emitWriteProc();


    emit("END MAIN");


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
        emit("    ; " + n->value + " " +
             (n->type == NodeType::IntAssign ? "=" : ":=") + " ...");
        genExpr(n->children[0]);
        emit("    MOV " + n->value + ", AX");
        break;
    }
    case NodeType::IfStmt: {
        emit("    ; if " + n->value + " then ...");
        emit("    MOV AX, " + n->value);
        emit("    CMP AX, 0");
        if (n->children.size() > 1) {
            std::string le = L(), lend = L();
            emit("    JE " + le);
            genStmt(n->children[0]);
            emit("    JMP " + lend);
            emit(le + ":");
            genStmt(n->children[1]);
            emit(lend + ":");
        } else {
            std::string lend = L();
            emit("    JE " + lend);
            genStmt(n->children[0]);
            emit(lend + ":");
        }
        break;
    }
    case NodeType::WhileStmt: {
        std::string ls = L(), le = L();
        emit("    ; while " + n->value + " do ...");
        emit(ls + ":");
        emit("    MOV AX, " + n->value);
        emit("    CMP AX, 0");
        emit("    JE " + le);
        genStmt(n->children[0]);
        emit("    JMP " + ls);
        emit(le + ":");
        break;
    }
    case NodeType::Block:
        for (auto& c : n->children) genStmt(c);
        break;
    case NodeType::ReadStmt:
        useRead_ = true;
        emit("    ; read " + n->value);
        emit("    CALL ReadInt");
        emit("    MOV " + n->value + ", AX");
        break;
    case NodeType::WriteStmt:
        useWrite_ = true;
        emit("    ; write " + n->value);
        emit("    MOV AX, " + n->value);
        emit("    CALL WriteInt");
        break;
    default: break;
    }
}


/* 表达式求值 → 结果在 AX */
void CodeGen8086::genExpr(ASTPtr n) {
    if (!n) return;


    if (n->type == NodeType::Number) {
        emit("    MOV AX, " + n->value);
        return;
    }
    if (n->type == NodeType::BoolLit) {
        emit("    MOV AX, " + std::string(n->value == "true" ? "1" : "0"));
        return;
    }
    if (n->type == NodeType::Ident) {
        emit("    MOV AX, " + n->value);
        return;
    }
    if (n->type == NodeType::UnaryOp) {
        genExpr(n->children[0]);
        if (n->value == "-")  emit("    NEG AX");
        if (n->value == "!")  emit("    XOR AX, 1");
        return;
    }
    if (n->type == NodeType::BinOp) {
        std::string op = n->value;


        genExpr(n->children[0]);          // left → AX
        emit("    PUSH AX");
        genExpr(n->children[1]);          // right → AX
        emit("    POP BX");              // BX=left, AX=right
        emit("    XCHG AX, BX");         // AX=left, BX=right


        if      (op == "+") emit("    ADD AX, BX");
        else if (op == "-") emit("    SUB AX, BX");
        else if (op == "*") emit("    IMUL BX");
        else if (op == "/") { emit("    CWD"); emit("    IDIV BX"); }
        else if (op == "||") emit("    OR AX, BX");
        else if (op == "&&") emit("    AND AX, BX");
        else {
            // 关系运算符: > >= < <= == !=
            std::string lt = L(), le = L();
            emit("    CMP AX, BX");
            std::string jmp;
            if      (op == ">")  jmp = "JG";
            else if (op == ">=") jmp = "JGE";
            else if (op == "<")  jmp = "JL";
            else if (op == "<=") jmp = "JLE";
            else if (op == "==") jmp = "JE";
            else if (op == "!=") jmp = "JNE";
            emit("    " + jmp + " " + lt);
            emit("    MOV AX, 0");
            emit("    JMP " + le);
            emit(lt + ":");
            emit("    MOV AX, 1");
            emit(le + ":");
        }
        return;
    }
}


/* ---------- ReadInt 子程序 ---------- */
void CodeGen8086::emitReadProc() {
    emit("; ---- Read signed int from keyboard -> AX ----");
    emit("ReadInt PROC");
    emit("    PUSH BX");
    emit("    PUSH CX");
    emit("    PUSH DX");
    emit("    XOR BX, BX");
    emit("    XOR CX, CX");
    emit("    MOV AH, 01H");
    emit("    INT 21H");
    emit("    CMP AL, '-'");
    emit("    JNE RI_D");
    emit("    MOV CX, 1");
    emit("    MOV AH, 01H");
    emit("    INT 21H");
    emit("RI_D:");
    emit("    CMP AL, '0'");
    emit("    JB RI_END");
    emit("    CMP AL, '9'");
    emit("    JA RI_END");
    emit("    SUB AL, '0'");
    emit("    XOR AH, AH");
    emit("    XCHG AX, BX");
    emit("    MOV DX, 10");
    emit("    MUL DX");
    emit("    ADD AX, BX");
    emit("    MOV BX, AX");
    emit("    MOV AH, 01H");
    emit("    INT 21H");
    emit("    JMP RI_D");
    emit("RI_END:");
    emit("    MOV AX, BX");
    emit("    CMP CX, 0");
    emit("    JE RI_RET");
    emit("    NEG AX");
    emit("RI_RET:");
    emit("    POP DX");
    emit("    POP CX");
    emit("    POP BX");
    emit("    RET");
    emit("ReadInt ENDP");
    emit("");
}


/* ---------- WriteInt 子程序 ---------- */
void CodeGen8086::emitWriteProc() {
    emit("; ---- Write signed int in AX to screen ----");
    emit("WriteInt PROC");
    emit("    PUSH AX");
    emit("    PUSH BX");
    emit("    PUSH CX");
    emit("    PUSH DX");
    emit("    XOR CX, CX");
    emit("    CMP AX, 0");
    emit("    JGE WI_POS");
    emit("    PUSH AX");
    emit("    MOV DL, '-'");
    emit("    MOV AH, 02H");
    emit("    INT 21H");
    emit("    POP AX");
    emit("    NEG AX");
    emit("WI_POS:");
    emit("    MOV BX, 10");
    emit("WI_LP:");
    emit("    XOR DX, DX");
    emit("    DIV BX");
    emit("    PUSH DX");
    emit("    INC CX");
    emit("    CMP AX, 0");
    emit("    JNE WI_LP");
    emit("WI_PR:");
    emit("    POP DX");
    emit("    ADD DL, '0'");
    emit("    MOV AH, 02H");
    emit("    INT 21H");
    emit("    LOOP WI_PR");
    emit("    MOV DL, 0DH");
    emit("    MOV AH, 02H");
    emit("    INT 21H");
    emit("    MOV DL, 0AH");
    emit("    INT 21H");
    emit("    POP DX");
    emit("    POP CX");
    emit("    POP BX");
    emit("    POP AX");
    emit("    RET");
    emit("WriteInt ENDP");
    emit("");
}


/* ============================================================
 *     驱动模块 (main)
 *     源文件 → 词法 → 语法 → 语义 → 代码生成
 * ============================================================ */
int main(int argc, char* argv[]) {
    std::string inFile  = "source.txt";
    std::string outFile = "semantic_out.txt";
    std::string asmFile = "asm_out.asm";
    if (argc >= 2) inFile  = argv[1];
    if (argc >= 3) outFile = argv[2];


    // 1. 读入源文件
    std::ifstream fin(inFile);
    if (!fin.is_open()) {
        std::cerr << "Error: cannot open " << inFile << std::endl;
        return 1;
    }
    std::string source((std::istreambuf_iterator<char>(fin)),
                        std::istreambuf_iterator<char>());
    fin.close();


    std::ofstream fout(outFile);
    fout << "=== LittleC Compilation Report ===\n";
    fout << "Source: " << inFile << "\n\n";


    bool hasAnyError = false;


    // 2. 词法分析
    Lexer lexer;
    std::vector<Token> tokens = lexer.tokenize(source);
    if (lexer.hasError()) {
        hasAnyError = true;
        fout << "[Lexer Errors]\n" << lexer.getErrors() << "\n";
    } else {
        fout << "Lexical analysis:  OK (" << tokens.size() << " tokens)\n";
    }


    // 3. 语法分析
    Parser parser(tokens);
    ASTPtr ast = parser.parse();
    if (parser.hasError()) {
        hasAnyError = true;
        fout << "[Parser Errors]\n" << parser.getErrors() << "\n";
    } else {
        fout << "Syntax analysis:   OK\n";
    }


    // 4. 语义分析
    SemanticAnalyzer sem;
    if (!hasAnyError && ast) {
        sem.analyze(ast);
        if (sem.hasError()) {
            hasAnyError = true;
            fout << "[Semantic Errors]\n" << sem.getErrors() << "\n";
        } else {
            fout << "Semantic analysis: OK\n";
        }
        fout << "\n=== Symbol Table ===\n" << sem.getSymbolTableStr() << "\n";
    }


    // 5. 代码生成 或 报错
    if (hasAnyError) {
        fout << "Compilation FAILED. No assembly generated.\n";
        fout.close();
        std::cout << "FAILED. See " << outFile << std::endl;
    } else {
        CodeGen8086 gen;
        std::string asmCode = gen.generate(ast, sem.symTable());
        fout << "=== 8086 Assembly Output ===\n";
        fout << "(Written to " << asmFile << ")\n";
        fout.close();


        // 写汇编到单独文件
        std::ofstream af(asmFile);
        af << asmCode;
        af.close();


        std::cout << "SUCCESS!\n";
        std::cout << "Report  -> " << outFile << std::endl;
        std::cout << "Assembly-> " << asmFile << std::endl;
        std::cout << "\n" << asmCode;
    }


    return hasAnyError ? 1 : 0;
}
