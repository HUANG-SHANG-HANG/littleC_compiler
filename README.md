# LittleC Compiler


> 一个面向简化版 C 语言（LittleC）的编译器前端实现，涵盖词法分析、语法分析、语义分析与 8086 汇编代码生成，并提供基于 Qt 的桌面可视化工具。


本项目为编译原理课程学习项目，按照编译器核心流程逐步实现各个模块，最终整合为完整的编译管线。


---


##  项目结构
```text
littleC_compiler/
├── lexer/             # 词法分析器（独立可运行）
│   ├── lexer.h
│   ├── lexer.cpp
│   └── source.txt     # 测试用源程序
├── parser/            # 语法分析器（独立可运行）
│   ├── parser.h
│   └── parser.cpp
├── semantic/          # 语义分析 + 8086 代码生成（独立可运行）
│   ├── semantic.h
│   ├── semantic.cpp
│   └── source.txt
├── qt_app/            # Qt 桌面可视化工具（整合全部模块）
│   ├── LittleCCompiler.pro
│   ├── main.cpp
│   ├── mainwindow.h
│   ├── mainwindow.cpp
│   ├── lexer.h / lexer.cpp
│   ├── parser.h / parser.cpp
│   └── semantic.h / semantic.cpp
└── README.md
```
---


##  LittleC 语言定义


### 一、词法规则


LittleC 源程序由以下词法单元组成：


| 类别 | 说明 | 示例 |
|------|------|------|
| **标识符** | 字母或下划线开头，后跟字母/数字/下划线，长度 ≤ 8，不区分大小写 | `a`, `count`, `_flag` |
| **整型常量** | 纯数字串，长度 ≤ 8；负数在正数前加 `-` | `0`, `42`, `-7` |
| **布尔常量** | `true` 和 `false` | |
| **关键字（保留）** | `int` `bool` `if` `then` `else` `while` `do` `read` `write` | |
| **算术运算符** | `+`  `-`  `*`  `/` | |
| **关系运算符** | `>`  `>=`  `<`  `<=`  `==`  `!=` | |
| **逻辑运算符** | `\|\|`  `&&`  `!` | |
| **赋值运算符** | `=`（整型赋值）、`:=`（布尔型赋值） | |
| **分隔符** | `{`  `}`  `(`  `)`  `;`  `,` | |
| **注释** | 单行 `//`，多行 `/* ... */`，注释内容跳过 | |


不在上述字符集中的符号视为**非法字符**。


### 二、语法规则（文法定义）
```text
PROG   → { DECLS STMTS }

DECLS  → DECLS DECL | ε
DECL   → int NAMES ; | bool NAMES ;
NAMES  → NAMES , NAME | NAME
NAME   → id

STMTS  → STMTS STMT | STMT
STMT   → id = EXPR ;              // 整型赋值
STMT   → id := BOOL ;             // 布尔型赋值
STMT   → if id then STMT
STMT   → if id then STMT else STMT
STMT   → while id do STMT
STMT   → { STMTS }
STMT   → read id ;
STMT   → write id ;

EXPR   → EXPR + TERM | EXPR - TERM | TERM
TERM   → TERM * NEGA | TERM / NEGA | NEGA
NEGA   → - FACTOR | FACTOR
FACTOR → ( EXPR ) | id | number

BOOL   → BOOL || JOIN | JOIN
JOIN   → JOIN && NOT | NOT
NOT    → ! REL | REL
REL    → EXPR ROP EXPR
ROP    → > | >= | < | <= | == | !=
```
定义解释

一个 LittleC 程序就是一个大花括号块<br>
{<br>
    - 声明部分<br>
    - 执行部分<br>
}<br>
<br>
声明部分可以有零到多条 int 变量名列表; 或 bool 变量名列表;<br>
变量名之间用逗号分隔。执行部分由一条或多条语句组成，语句包括：<br>
    整型赋值 id = 算术表达式、<br>
    布尔赋值 id := 布尔表达式、<br>
    条件语句 if id then 语句 或 if id then 语句 else 语句、<br>
    循环语句 while id do 语句、<br>
    复合语句 (花括号裹多条语句)、<br>
    I/O 语句 read id; 和 write id;<br>
算术表达式按 * / 优先于 + - 的规则递归定义，支持括号和取负;<br>
布尔表达式按 ! 优先于 && 优先于 || 的规则递归定义<br>
关系表达式 (两个算术表达式 + 关系运算符) 产生布尔值。<br>
### 三、语义规则


- 仅支持两种类型：**int**（4 字节）和 **bool**（1 字节），两者**不兼容**
- 整型变量用 `=` 赋值，布尔变量用 `:=` 赋值，不可混用
- 变量必须**先声明再使用**，同一作用域内不可重复声明
- `if` / `while` 的条件必须是一个 **bool 型变量**（非表达式）
- 算术运算操作数必须为 int，结果为 int
- 关系运算操作数必须为 int，结果为 bool
- 逻辑运算操作数必须为 bool，结果为 bool
- `read` / `write` 操作对象必须为 int 型变量
- 不支持数组、结构体、指针、函数调用


---


##  各模块独立运行


### 环境要求


- **C++ 编译器**：支持 C++17（g++ 7+ / clang++ 5+ / MSVC 2017+）
- **Qt**（仅 qt_app 需要）：Qt 5.15+ 或 Qt 6.x


### 1. 词法分析器


```bash
cd lexer
g++ -std=c++17 -DLEXER_STANDALONE -o lexer lexer.cpp
./lexer source.txt lex_out.txt
输入： source.txt （LittleC 源程序）
输出： lex_out.txt （等长二元组形式的 Token 序列）
输出格式示例：

Line  Binary Tuple                Type
--------------------------------------------------
2     (50, "{")                   DL_{
3     (1, "int")                  KW_INT
3     (20, "a")                   IDENT
3     (30, "+")                   OP_+

```


### 2. 语法分析器

```text
cd parser
g++ -std=c++17 -DPARSER_STANDALONE -o parser parser.cpp ../lexer/lexer.cpp
./parser source.txt parse_out.txt

输入： source.txt （LittleC 源程序）
输出： parse_out.txt （抽象语法树 AST 的树形文本表示）

输出格式示例：

Program  [line 2]
├── DeclList  [line 2]
│   └── VarDecl (int)  [line 3]
│       ├── Name: a  [line 3]
│       └── Name: b  [line 3]
└── StmtList  [line 4]
    └── IntAssign -> c  [line 6]
        └── BinOp (+)  [line 6]
            ├── Ident: a  [line 6]
            └── Ident: b  [line 6]
```


### 3. 语义分析 + 8086 代码生成

```text
cd semantic
g++ -std=c++17 -o compiler semantic.cpp ../parser/parser.cpp ../lexer/lexer.cpp
./compiler source.txt semantic_out.txt

输入： source.txt （LittleC 源程序）
输出： semantic_out.txt （编译报告：符号表、错误信息）
 asm_out.asm （8086 汇编代码，仅在无错误时生成）
```

 ## Qt 桌面程序快速开始


前置条件


安装  Qt （Qt 5.15+ 或 Qt 6.x），安装时勾选：
Qt 对应版本（如 Qt 6.5.x）
MinGW 64-bit（Windows）或使用系统编译器（Linux/Mac）
Qt Creator

构建与运行

使用 Qt Creator


1. 打开 Qt Creator
2. 文件 → 打开文件或项目 → 选择 qt_app/LittleCCompiler.pro
3. 选择 Kit（如 Desktop Qt 6.x MinGW 64-bit）→ Configure Project
4. 点击左下角 ▶ 运行（或 Ctrl+R）

窗口程序
使用方式

在左侧编辑框输入 LittleC 源代码，或点击 「选择源文件」 加载  .txt  文件
点击中间的 「开始编译」 按钮
右侧输出框显示结果：

若有错误 → 显示错误类型、行号和详细信息
若无错误 → 显示符号表和生成的 8086 汇编代码
点击 「保存输出到文件」 可将结果导出为  .txt  或  .asm  文件

##  错误类型一览

编译器能检测并报告以下 15 类错误：

| 编号 | 阶段 | 错误描述 |
|:-----|:-----|:---------|
| E01 | 词法 | 非法字符 |
| E02 | 词法 | 标识符超长（>8） |
| E03 | 词法 | 整常数超长（>8） |
| E04 | 词法 | 多行注释未闭合 |
| E05 | 语法 | 语法结构不匹配（缺少关键符号） |
| E06 | 语义 | 变量重复声明 |
| E07 | 语义 | 变量未声明就使用 |
| E08 | 语义 | 整型赋值 `=` 类型不匹配 |
| E09 | 语义 | 布尔赋值 `:=` 类型不匹配 |
| E10 | 语义 | 算术运算操作数非 int |
| E11 | 语义 | 逻辑运算操作数非 bool（`&&` `!` `\|\|`） |
| E12 | 语义 | 关系运算操作数非 int |
| E13 | 语义 | if／while 条件变量非 bool |
| E14 | 语义 | read 目标非 int 变量 |
| E15 | 语义 | write 目标非 int 变量 |


## 测试用例

```text
测试 1：基本算术


// source1.txt
{
    int a, b, c;
    a = 1;
    b = 2;
    c = a + b;
}

测试 2：布尔与控制流

// source2.txt
{
    int x, y;
    bool flag;
    x = 10;
    y = 20;
    flag := x < y;
    if flag then
        write x;
    else
        write y;
}


测试 3：循环


// source3.txt
{
    int n, sum;
    bool cond;
    read n;
    sum = 0;
    cond := n > 0;
    while cond do {
        sum = sum + n;
        n = n - 1;
        cond := n > 0;
    }
    write sum;
}



测试 4：错误检测


// source4.txt  —— 包含多种错误
{
    int a;
    b = 10;
    a := true;
}


预期输出：

Line 3: [E07] Undeclared variable 'b'
Line 4: [E09] Cannot use ':=' on int variable 'a'


测试 5：非法字符


// source5.txt
{
    int a;
    a = 1 @ 2;
}

预期输出：


Line 3: Illegal character: '@'

```

---
##  技术实现要点


| 模块 | 核心技术 |
|:-----|:---------|
| 词法分析器 | 手写有限状态机，关键字哈希表匹配，行号追踪 |
| 语法分析器 | 递归下降分析法，自顶向下构建 AST |
| 语义分析器 | 符号表（哈希表），递归遍历 AST 进行类型推导与检查 |
| 代码生成器 | AST 后序遍历，栈式表达式求值，生成 8086 MASM 格式汇编 |
| Qt 界面 | QMainWindow＋信号槽机制＋QFileDialog 文件读写 |

