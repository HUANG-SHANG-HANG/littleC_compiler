#include "mainwindow.h"
#include "lexer.h"
#include "parser.h"
#include "semantic.h"


#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QFileDialog>
#include <QFont>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("LittleC Compiler");
    resize(1000, 600);


    // ========== 创建控件 ==========


    // 左侧：源码区
    srcEdit = new QPlainTextEdit;
    QFont mono("Consolas", 11);
    mono.setStyleHint(QFont::Monospace);
    srcEdit->setFont(mono);
    srcEdit->setPlaceholderText("// 在此输入 LittleC 源代码，或点击下方按钮选择文件...");


    btnOpen = new QPushButton("📂 选择源文件");


    // 中间：编译按钮
    btnCompile = new QPushButton("▶  开始编译");
    btnCompile->setFixedHeight(50);
    btnCompile->setStyleSheet(
        "QPushButton {"
        "  background-color: #4a90d9;"
        "  color: white;"
        "  font-size: 16px;"
        "  font-weight: bold;"
        "  border-radius: 6px;"
        "}"
        "QPushButton:hover {"
        "  background-color: #357abd;"
        "}"
    );


    // 右侧：输出区
    outEdit = new QTextEdit;
    outEdit->setFont(mono);
    outEdit->setReadOnly(true);
    outEdit->setPlaceholderText("编译结果将显示在此处...");


    btnSave = new QPushButton("💾 保存输出到文件");


    // 状态栏标签
    statusLabel = new QLabel("就绪");


    // ========== 布局 ==========


    // 左侧垂直布局
    QVBoxLayout *leftLayout = new QVBoxLayout;
    QLabel *leftTitle = new QLabel("📝 源代码");
    leftTitle->setStyleSheet("font-size:14px; font-weight:bold;");
    leftLayout->addWidget(leftTitle);
    leftLayout->addWidget(srcEdit);
    leftLayout->addWidget(btnOpen);


    // 右侧垂直布局
    QVBoxLayout *rightLayout = new QVBoxLayout;
    QLabel *rightTitle = new QLabel("📋 编译输出");
    rightTitle->setStyleSheet("font-size:14px; font-weight:bold;");
    rightLayout->addWidget(rightTitle);
    rightLayout->addWidget(outEdit);
    rightLayout->addWidget(btnSave);


    // 总体水平布局：左 | 按钮 | 右
    QHBoxLayout *mainLayout = new QHBoxLayout;


    // 左侧容器
    QWidget *leftWidget = new QWidget;
    leftWidget->setLayout(leftLayout);


    // 中间容器（编译按钮竖向居中）
    QVBoxLayout *midLayout = new QVBoxLayout;
    midLayout->addStretch();
    midLayout->addWidget(btnCompile);
    midLayout->addStretch();
    QWidget *midWidget = new QWidget;
    midWidget->setFixedWidth(130);
    midWidget->setLayout(midLayout);


    // 右侧容器
    QWidget *rightWidget = new QWidget;
    rightWidget->setLayout(rightLayout);


    mainLayout->addWidget(leftWidget, 1);   // 左占 1 份
    mainLayout->addWidget(midWidget);        // 中间固定宽
    mainLayout->addWidget(rightWidget, 1);   // 右占 1 份


    // 设置为中心窗口
    QWidget *central = new QWidget;
    central->setLayout(mainLayout);
    setCentralWidget(central);


    // 状态栏
    statusBar()->addWidget(statusLabel);


    // ========== 连接信号槽 ==========
    connect(btnOpen,    &QPushButton::clicked, this, &MainWindow::onOpenFile);
    connect(btnCompile, &QPushButton::clicked, this, &MainWindow::onCompile);
    connect(btnSave,    &QPushButton::clicked, this, &MainWindow::onSaveOutput);
}


// ========== 选择文件 ==========
void MainWindow::onOpenFile() {
    QString path = QFileDialog::getOpenFileName(
        this, "选择源文件", "",
        "Text Files (*.txt *.lc);;All Files (*)");


    if (path.isEmpty()) return;


    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法打开文件: " + path);
        return;
    }
    QTextStream in(&file);
    srcEdit->setPlainText(in.readAll());
    file.close();
    statusLabel->setText("已加载: " + path);
}


// ========== 开始编译 ==========
void MainWindow::onCompile() {
    outEdit->clear();
    std::string source = srcEdit->toPlainText().toStdString();


    if (source.empty()) {
        outEdit->setPlainText("⚠ 源代码为空，请输入代码或选择文件。");
        statusLabel->setText("源代码为空");
        return;
    }


    QString report;
    bool hasError = false;


    // ---- 1. 词法分析 ----
    Lexer lexer;
    std::vector<Token> tokens = lexer.tokenize(source);


    if (lexer.hasError()) {
        hasError = true;
        report += "===== 词法错误 =====\n";
        report += QString::fromStdString(lexer.getErrors()) + "\n";
    } else {
        report += "词法分析: OK (" + QString::number(tokens.size()) + " tokens)\n";
    }


    // ---- 2. 语法分析 ----
    Parser parser(tokens);
    ASTPtr ast = parser.parse();


    if (parser.hasError()) {
        hasError = true;
        report += "===== 语法错误 =====\n";
        report += QString::fromStdString(parser.getErrors()) + "\n";
    } else {
        report += "语法分析: OK\n";
    }


    // ---- 3. 语义分析 ----
    SemanticAnalyzer sem;
    if (!hasError && ast) {
        sem.analyze(ast);
        if (sem.hasError()) {
            hasError = true;
            report += "===== 语义错误 =====\n";
            report += QString::fromStdString(sem.getErrors()) + "\n";
        } else {
            report += "语义分析: OK\n";
        }
        report += "\n--- 符号表 ---\n";
        report += QString::fromStdString(sem.getSymbolTableStr()) + "\n";
    }


    // ---- 4. 输出结果 ----
    if (hasError) {
        report += "\n❌ 编译失败，存在错误。\n";
        outEdit->setStyleSheet("color: #c0392b;");  // 红色
        outEdit->setPlainText(report);
        statusLabel->setText("❌ 编译失败");
    } else {
        // 无错误 → 生成 8086 汇编
        CodeGen8086 gen;
        std::string asmCode = gen.generate(ast, sem.symTable());


        QString result;
        result += report + "\n";
        result += "✅ 编译成功！\n\n";
        result += "========== 8086 汇编代码 ==========\n\n";
        result += QString::fromStdString(asmCode);


        outEdit->setStyleSheet("color: #2c3e50;");  // 正常颜色
        outEdit->setPlainText(result);
        statusLabel->setText("✅ 编译成功");
    }
}


// ========== 保存输出 ==========
void MainWindow::onSaveOutput() {
    if (outEdit->toPlainText().isEmpty()) {
        QMessageBox::information(this, "提示", "输出为空，请先编译。");
        return;
    }


    QString path = QFileDialog::getSaveFileName(
        this, "保存输出", "output.txt",
        "Text Files (*.txt);;ASM Files (*.asm);;All Files (*)");


    if (path.isEmpty()) return;


    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法写入: " + path);
        return;
    }
    QTextStream out(&file);
    out << outEdit->toPlainText();
    file.close();
    statusLabel->setText("已保存: " + path);
}
