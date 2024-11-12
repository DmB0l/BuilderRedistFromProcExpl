#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QDebug>
#include <QPushButton>
#include <QVector>
#include <QFileInfo>
#include <QProcess>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onPB_startBuild();

private:
    Ui::MainWindow *ui;
    QVector<QString> m_dllFullPaths;
    QVector<QString> m_dllDirs;

    QString m_pathScript = "buid.cmd";
    QString m_pathMingwDir = "";
    QString m_pathProcExplFile = "";
    QString m_pathBuildDir = "";
};
#endif // MAINWINDOW_H
