#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->PB_startBuild, &QPushButton::clicked, this, &MainWindow::onPB_startBuild);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onPB_startBuild()
{
    m_pathMingwDir = ui->PTE_mingwDir->toPlainText();
    m_pathProcExplFile = ui->PTE_procExplFile->toPlainText();
    m_pathBuildDir = ui->PTE_buildDir->toPlainText();

    QFile file(m_pathProcExplFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to read process explorer txt file!";
        return;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString string = in.readLine();
        int firstIndMingwDir = string.indexOf(m_pathMingwDir);
        if(firstIndMingwDir >= 0) {
            int lastIndMingwDir = firstIndMingwDir + m_pathMingwDir.length();

            QString path = string.mid(firstIndMingwDir);
            m_dllFullPaths.append(path);

            QFileInfo fileInfo(path);
            QString fileName = fileInfo.fileName();

            QString dir = string.mid(lastIndMingwDir, string.length() - fileName.length() - lastIndMingwDir);
            if(!m_dllDirs.contains(dir)) {
                m_dllDirs.append(dir);
            }
        }
    }

    QFile execSHfile(m_pathScript);
    if (execSHfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&execSHfile);
        out << "@echo off" << "\n\n";
        for(QString dllDir : m_dllDirs) {
            out << "mkdir " << m_pathBuildDir << dllDir << "\n";
        }
        out << "\n";

        for(QString dllPath : m_dllFullPaths) {
            QString dir = "";
            for(QString dllDir : m_dllDirs) {
                if(dllPath.indexOf(dllDir) != -1) {
                    dir = dllDir;
                }
            }
            if(dir != "") {
                // qDebug() <<
                out << "copy " << dllPath << " /b " << m_pathBuildDir << dir << "\n";
            }
        }

        execSHfile.close();
        qDebug() << "Script file created successfully!";

        QProcess *procCmdScript = new QProcess(this);
        procCmdScript->start(m_pathScript, QStringList());
        if (!procCmdScript->waitForStarted()) {
            qDebug() << "Failed. Cant run cmd script";
            return;
        }
        if (!procCmdScript->waitForFinished()) {
            qDebug() << "Failed. Cant execute cmd script";
            return;
        }
        else {
            qDebug() << "Complete! Check build dir";
        }

    } else {
        qDebug() << "Failed to create script file!";
        return;
    }
}
