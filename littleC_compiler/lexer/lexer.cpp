#include "lexer.h"
#include <cctype>
#include <algorithm>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>


/* ================================================
 *             关 键 字 表
 * ================================================ */
static const std::unordered_map<std::string, int> kwTable = {
    {"int",   KW_INT},   {"bool",  KW_BOOL},
    {"if",    KW_IF},    {"then",  KW_THEN},  {"else", KW_ELSE},
    {"while", KW_WHILE}, {"do",    KW_DO},
    {"read",  KW_READ},  {"write", KW_WRITE},
    {"true",  KW_TRUE},  {"false", KW_FALSE},
};


/* ================================================
 *          Lexer 工作模块实现
 * ================================================ */


char Lexer::cur() const {
    return (pos_ < (int)src_.size()) ? src_[pos_] : '\0';
}


char Lexer::peek() const {
    return (pos_ + 1 < (int)src_.size()) ? src_[pos_ + 1] : '\0';
}


void Lexer::next() {
    if (pos_ < (int)src_.size()) {
        if (src_[pos_] == '\n') line_++;
        pos_++;
    }
}


void Lexer::addError(const std::string& msg) {
    errors_ += "Line " + std::to_string(line_) + ": " + msg + "\n";
}


void Lexer::skipWhitespace() {
    while (pos_ < (int)src_.size() && std::isspace((unsigned char)cur()))
        next();
}


bool Lexer::skipComment() {
    // 单行注释 //
    if (cur() == '/' && peek() == '/') {
        next(); next();
        while (pos_ < (int)src_.size() && cur() != '\n') next();
        return true;
    }
    // 多行注释 /* ... */
    if (cur() == '/' && peek() == '*') {
        int startLine = line_;
        next(); next();
        while (pos_ < (int)src_.size()) {
            if (cur() == '*' && peek() == '/') {
                next(); next();
                return true;
            }
            next();
        }
        addError("Unterminated comment (started at line " +
                 std::to_string(startLine) + ")");
        return true;
    }
    return false;
}


Token Lexer::readNumber() {
    int ln = line_;
    std::string num;
    while (pos_ < (int)src_.size() && std::isdigit((unsigned char)cur())) {
        num += cur();
        next();
    }
    // 检查长度上限 8
    if (num.size() > 8)
        addError("Integer too long (>8): " + num);
    return {TK_NUM, num, ln};
}


Token Lexer::readWord() {
    int ln = line_;
    std::string word;
    while (pos_ < (int)src_.size() &&
           (std::isalnum((unsigned char)cur()) || cur() == '_')) {
        word += cur();
        next();
    }
    // 检查长度上限 8
    if (word.size() > 8)
        addError("Identifier too long (>8): " + word);


    // 不区分大小写 → 统一转小写
    std::string lower = word;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });


    // 查关键字表
    auto it = kwTable.find(lower);
    if (it != kwTable.end())
        return {it->second, lower, ln};   // 关键字
    return {TK_ID, lower, ln};            // 标识符
}


/* ---------- 核心: tokenize ---------- */
std::vector<Token> Lexer::tokenize(const std::string& source) {
    src_ = source;
    pos_ = 0;
    line_ = 1;
    errors_.clear();


    std::vector<Token> tokens;


    while (pos_ < (int)src_.size()) {
        skipWhitespace();
        if (pos_ >= (int)src_.size()) break;


        // 尝试跳过注释
        if (skipComment()) continue;


        char c = cur();
        int  ln = line_;


        // ---- 数字 ----
        if (std::isdigit((unsigned char)c)) {
            tokens.push_back(readNumber());
            continue;
        }


        // ---- 标识符 / 关键字 ----
        if (std::isalpha((unsigned char)c) || c == '_') {
            tokens.push_back(readWord());
            continue;
        }


        // ---- 双字符运算符 (先检查) ----
        if (c == '>' && peek() == '=') { next(); next(); tokens.push_back({OP_GE,  ">=", ln}); continue; }
        if (c == '<' && peek() == '=') { next(); next(); tokens.push_back({OP_LE,  "<=", ln}); continue; }
        if (c == '=' && peek() == '=') { next(); next(); tokens.push_back({OP_EQ,  "==", ln}); continue; }
        if (c == '!' && peek() == '=') { next(); next(); tokens.push_back({OP_NEQ, "!=", ln}); continue; }
        if (c == '|' && peek() == '|') { next(); next(); tokens.push_back({OP_OR,  "||", ln}); continue; }
        if (c == '&' && peek() == '&') { next(); next(); tokens.push_back({OP_AND, "&&", ln}); continue; }
        if (c == ':' && peek() == '=') { next(); next(); tokens.push_back({OP_BOOL_ASSIGN, ":=", ln}); continue; }


        // ---- 单字符 ----
        next(); // 消耗当前字符
        switch (c) {
            case '+': tokens.push_back({OP_ADD,     "+", ln}); break;
            case '-': tokens.push_back({OP_SUB,     "-", ln}); break;
            case '*': tokens.push_back({OP_MUL,     "*", ln}); break;
            case '/': tokens.push_back({OP_DIV,     "/", ln}); break;
            case '>': tokens.push_back({OP_GT,      ">", ln}); break;
            case '<': tokens.push_back({OP_LT,      "<", ln}); break;
            case '!': tokens.push_back({OP_NOT,     "!", ln}); break;
            case '=': tokens.push_back({OP_ASSIGN,  "=", ln}); break;
            case '{': tokens.push_back({DL_LBRACE,  "{", ln}); break;
            case '}': tokens.push_back({DL_RBRACE,  "}", ln}); break;
            case '(': tokens.push_back({DL_LPAREN,  "(", ln}); break;
            case ')': tokens.push_back({DL_RPAREN,  ")", ln}); break;
            case ';': tokens.push_back({DL_SEMI,    ";", ln}); break;
            case ',': tokens.push_back({DL_COMMA,   ",", ln}); break;
            default:
                addError("Illegal character: '" + std::string(1, c) + "'");
                tokens.push_back({TK_ERR, std::string(1, c), ln});
                break;
        }
    }
    return tokens;
}


/* ================================================
 *          编码 → 类型名
 * ================================================ */
std::string tokenCodeName(int code) {
    switch (code) {
        case KW_INT:    return "KW_INT";
        case KW_BOOL:   return "KW_BOOL";
        case KW_IF:     return "KW_IF";
        case KW_THEN:   return "KW_THEN";
        case KW_ELSE:   return "KW_ELSE";
        case KW_WHILE:  return "KW_WHILE";
        case KW_DO:     return "KW_DO";
        case KW_READ:   return "KW_READ";
        case KW_WRITE:  return "KW_WRITE";
        case KW_TRUE:   return "KW_TRUE";
        case KW_FALSE:  return "KW_FALSE";
        case TK_ID:     return "IDENT";
        case TK_NUM:    return "INT_CONST";
        case OP_ADD:    return "OP_+";
        case OP_SUB:    return "OP_-";
        case OP_MUL:    return "OP_*";
        case OP_DIV:    return "OP_/";
        case OP_GT:     return "OP_>";
        case OP_GE:     return "OP_>=";
        case OP_LT:     return "OP_<";
        case OP_LE:     return "OP_<=";
        case OP_EQ:     return "OP_==";
        case OP_NEQ:    return "OP_!=";
        case OP_OR:     return "OP_||";
        case OP_AND:    return "OP_&&";
        case OP_NOT:    return "OP_!";
        case OP_ASSIGN:      return "OP_=";
        case OP_BOOL_ASSIGN: return "OP_:=";
        case DL_LBRACE: return "DL_{";
        case DL_RBRACE: return "DL_}";
        case DL_LPAREN: return "DL_(";
        case DL_RPAREN: return "DL_)";
        case DL_SEMI:   return "DL_;";
        case DL_COMMA:  return "DL_,";
        case TK_ERR:    return "ERROR";
        default:        return "UNKNOWN";
    }
}


/* ================================================
 *     驱 动 模 块 (main)
 *     负责: 读文件 → 调用工作模块 → 写结果文件
 * ================================================ */
#ifdef LEXER_STANDALONE   //编译语法分析器时候不许要这个main
int main(int argc, char* argv[]) {
    // 默认文件名
    std::string inFile  = "source.txt";
    std::string outFile = "lex_out.txt";
    if (argc >= 2) inFile  = argv[1];
    if (argc >= 3) outFile = argv[2];


    // ---- 1. 读入源文件 ----
    std::ifstream fin(inFile);
    if (!fin.is_open()) {
        std::cerr << "Error: cannot open " << inFile << std::endl;
        return 1;
    }
    std::string source((std::istreambuf_iterator<char>(fin)),
                        std::istreambuf_iterator<char>());
    fin.close();


    // ---- 2. 调用词法分析器 (工作模块) ----
    Lexer lexer;
    std::vector<Token> tokens = lexer.tokenize(source);


    // ---- 3. 格式化输出到文件 ----
    std::ofstream fout(outFile);
    if (!fout.is_open()) {
        std::cerr << "Error: cannot write " << outFile << std::endl;
        return 1;
    }


    fout << "=== LittleC Lexical Analysis ===" << std::endl;
    fout << "Source file: " << inFile << std::endl;
    fout << "Total tokens: " << tokens.size() << std::endl;
    fout << std::string(50, '-') << std::endl;
    fout << std::left
         << std::setw(6)  << "Line"
         << std::setw(28) << "Binary Tuple"
         << "Type" << std::endl;
    fout << std::string(50, '-') << std::endl;


    for (const auto& t : tokens) {
        // 等长二元组: (code, "value")
        std::string tuple = "(" + std::to_string(t.code) + ", \""
                          + t.value + "\")";
        fout << std::left
             << std::setw(6)  << t.line
             << std::setw(28) << tuple
             << tokenCodeName(t.code) << std::endl;
    }


    if (lexer.hasError()) {
        fout << "\n=== ERRORS ===" << std::endl;
        fout << lexer.getErrors();
    } else {
        fout << "\n=== No errors. ===" << std::endl;
    }
    fout.close();


    // ---- 4. 控制台提示 ----
    std::cout << "Done!  " << tokens.size() << " tokens." << std::endl;
    std::cout << "Result -> " << outFile << std::endl;
    if (lexer.hasError())
        std::cout << "\nErrors:\n" << lexer.getErrors();
    else
        std::cout << "No errors." << std::endl;


    return 0;
}
#endif 
