#include "mainwindow.h"

#include <QTextBlock> // Include the QTextCursor header
#include <QTextCursor> // Include the QTextCursor header
#include <QMetaType>   // Include the QMetaType header

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qRegisterMetaType<QTextBlock>("QTextBlock");
    qRegisterMetaType<QTextCursor>("QTextCursor");
    MainWindow w;
    w.show();
    return a.exec();
}
