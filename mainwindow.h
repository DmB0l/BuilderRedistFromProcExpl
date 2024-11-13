#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QDebug>
#include <QPushButton>
#include <QVector>
#include <QFileInfo>
#include <QProcess>
#include <QThread>
#include <QTextCodec>
#include <QRegularExpression>
#include <QFileDialog>
#include <QSettings>

#define COPY_ERROR_TEXT "Не удается найти указанный файл"

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

signals:
    void outputReady(QString output);
    void statusChanged(QString status);

private:
    Ui::MainWindow *ui;
    QVector<QString> m_dllFullPaths;
    QVector<QString> m_dllDirs;
    QVector<QString> m_errorsCopyFiles;

    QThread *m_procThread = nullptr;
    QProcess *m_procCmdScript = nullptr;

    QString m_pathScript = "build.cmd";
    QString m_pathMingwDir = "";
    QString m_pathProcExplFile = "";
    QString m_pathBuildDir = "";
    QString m_prevOutput = "";
};
#endif // MAINWINDOW_H
