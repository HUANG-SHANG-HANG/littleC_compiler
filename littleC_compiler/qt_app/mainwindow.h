#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QMainWindow>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QStatusBar>


class MainWindow : public QMainWindow {
    Q_OBJECT


public:
    MainWindow(QWidget *parent = nullptr);


private slots:
    void onOpenFile();       // 选择源文件
    void onCompile();        // 开始编译
    void onSaveOutput();     // 保存输出


private:
    // 左侧
    QPlainTextEdit *srcEdit;     // 源码编辑框
    QPushButton    *btnOpen;     // 选择文件按钮


    // 中间
    QPushButton    *btnCompile;  // 编译按钮


    // 右侧
    QTextEdit      *outEdit;     // 输出框
    QPushButton    *btnSave;     // 保存输出按钮


    // 状态
    QLabel         *statusLabel;
};


#endif
