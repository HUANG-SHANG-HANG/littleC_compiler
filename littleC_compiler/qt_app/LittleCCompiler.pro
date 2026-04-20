QT       += core gui widgets
TARGET = LittleCCompiler
TEMPLATE = app
CONFIG += c++17


SOURCES += \
    main.cpp \
    mainwindow.cpp \
    lexer.cpp \
    parser.cpp \
    semantic.cpp


HEADERS += \
    mainwindow.h \
    lexer.h \
    parser.h \
    semantic.h
