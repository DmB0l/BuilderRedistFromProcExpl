#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->label_4->setVisible(false);

    QSettings settings("BuilderRedistFromProcExpl", "Config");
    QString lastPEFileName = settings.value("last_PE_file", "").toString().replace("/", "\\");
    QString lastBuildDir = settings.value("last_build_dir", "").toString().replace("/", "\\");
    QString lastMingwDir = settings.value("last_mingw_dir", "").toString().replace("/", "\\");
    if(!lastPEFileName.isEmpty())
        ui->PTE_procExplFile->setText(lastPEFileName);
    if(!lastBuildDir.isEmpty())
        ui->PTE_buildDir->setText(lastBuildDir);
    if(!lastMingwDir.isEmpty())
        ui->PTE_mingwDir->setText(lastMingwDir);

    connect(ui->PB_startBuild, &QPushButton::clicked, this, &MainWindow::onPB_startBuild);

    connect(this, &MainWindow::outputReady, this, [=](QString output){
        ui->TE_features->appendPlainText(output);
    }, Qt::ConnectionType::QueuedConnection);

    connect(this, &MainWindow::statusChanged, this, [=](QString status){
        ui->L_status->setText(status);
    }, Qt::ConnectionType::QueuedConnection);

    connect(ui->PB_browsePE, &QPushButton::clicked, this, [=](){
        QSettings settings("BuilderRedistFromProcExpl", "Config");

        QString lastPEFileName = settings.value("last_PE_file", "").toString().replace("/", "\\");;
        QString fileName = QFileDialog::getOpenFileName(this, "Выберите текстовый файл", lastPEFileName, "Text Files (*.txt);;All Files (*)").replace("/", "\\");;
        if (!fileName.isEmpty()) {
            ui->PTE_procExplFile->setText(fileName);

            settings.setValue("last_PE_file", fileName);
        }
    }, Qt::ConnectionType::QueuedConnection);

    connect(ui->PB_browseBuildDir, &QPushButton::clicked, this, [=](){
        QSettings settings("BuilderRedistFromProcExpl", "Config");

        QString lastBuildDir = settings.value("last_build_dir", "").toString().replace("/", "\\");;
        QString dirName = QFileDialog::getExistingDirectory(this, "Выберите директорию", lastBuildDir).replace("/", "\\");;
        if (!dirName.isEmpty()) {
            ui->PTE_buildDir->setText(dirName);

            settings.setValue("last_build_dir", dirName);
        }
    }, Qt::ConnectionType::QueuedConnection);

    connect(ui->PB_browseMingwDir, &QPushButton::clicked, this, [=](){
        QSettings settings("BuilderRedistFromProcExpl", "Config");

        QString lastMingwDir = settings.value("last_mingw_dir", "").toString().replace("/", "\\");;
        QString dirName = QFileDialog::getExistingDirectory(this, "Выберите директорию", lastMingwDir).replace("/", "\\");;
        if (!dirName.isEmpty()) {
            ui->PTE_mingwDir->setText(dirName);

            settings.setValue("last_mingw_dir", dirName);
        }
    }, Qt::ConnectionType::QueuedConnection);
}

MainWindow::~MainWindow()
{
    if(m_procCmdScript && m_procCmdScript->state() != QProcess::NotRunning) {
        m_procCmdScript->kill();
        m_procCmdScript->deleteLater();
    }

    if(m_procThread) {
        m_procThread->quit();
        m_procThread->wait();
        m_procThread->deleteLater();
    }

    delete ui;
}

void MainWindow::onPB_startBuild()
{
    ui->PB_startBuild->setEnabled(false);
    ui->TE_features->clear();
    m_dllFullPaths.clear();
    m_dllDirs.clear();
    m_errorsCopyFiles.clear();

    ui->L_status->setText("Статус: ");
    m_pathMingwDir = ui->PTE_mingwDir->text();
    m_pathProcExplFile = ui->PTE_procExplFile->text();
    m_pathBuildDir = ui->PTE_buildDir->text();

    QFile file(m_pathProcExplFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to read process explorer txt file!";
        emit statusChanged(ui->L_status->text() + "<span style='color:red;'>Произошла ошибка</span>");

        ui->TE_features->appendPlainText("Failed to read process explorer txt file!");
        ui->PB_startBuild->setEnabled(true);
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

    if(m_dllFullPaths.size() == 0) {
        emit statusChanged(ui->L_status->text() + "<span style='color:red;'>Не найдены библиотеки</span>");
        ui->PB_startBuild->setEnabled(true);
        return;
    }

    QFile execSHfile(m_pathScript);
    if (execSHfile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&execSHfile);
        // out << "@echo off" << "\n\n";
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
                out << "copy \"" << dllPath << "\" /b \"" << m_pathBuildDir << dir << "\"\n";
            }
        }

        execSHfile.close();
        qDebug() << "Script file created successfully!";

        m_procCmdScript = new QProcess();
        m_procThread = new QThread();

        m_procCmdScript->moveToThread(m_procThread);
        connect(m_procThread, &QThread::started, [=](){
            QStringList arguments;
            // arguments << "buid.cmd"; // Используем /K, чтобы окно оставалось открытым

            connect(m_procCmdScript, &QProcess::readyReadStandardOutput, [&]() {
                QByteArray data = m_procCmdScript->readAllStandardOutput();

                QTextCodec *codec = QTextCodec::codecForName("CP866");
                QString output = codec->toUnicode(data);

                if(output.contains(COPY_ERROR_TEXT)) {
                    m_errorsCopyFiles.append(m_prevOutput);
                }

                emit outputReady(output);
            });

            m_procCmdScript->start(m_pathScript, QStringList());
            if (!m_procCmdScript->waitForStarted()) {
                qDebug() << "Failed. Cant run cmd script";
                emit statusChanged(ui->L_status->text() + "<span style='color:red;'>Произошла ошибка</span>");
                emit outputReady("Failed. Cant run cmd script");
                ui->PB_startBuild->setEnabled(true);
                return;
            }
            else if (!m_procCmdScript->waitForFinished()) {
                qDebug() << "Failed. Cant execute cmd script";
                emit statusChanged(ui->L_status->text() + "<span style='color:red;'>Произошла ошибка</span>");
                emit outputReady("Failed. Cant execute cmd script");
                ui->PB_startBuild->setEnabled(true);
                return;
            }
            else {
                qDebug() << "Complete! Check build dir";

                if(m_errorsCopyFiles.size() == 0) {
                    emit statusChanged(ui->L_status->text() + "<span style='color:green;'>Успешно</span>");
                    emit outputReady("Complete! Check build dir");
                    emit outputReady("libs: " + QString::number(m_dllFullPaths.size()) + " from " + QString::number(m_dllFullPaths.size()));
                }
                else {
                    emit statusChanged(ui->L_status->text() + "<span style='color:#FF4500;'>Скопированы не все dll</span>");
                    emit outputReady("Not full complete! Check build dir");
                    emit outputReady("libs: " + QString::number(m_dllFullPaths.size() - m_errorsCopyFiles.size()) + " from " + QString::number(m_dllFullPaths.size()));

                    QString text = ui->TE_features->toPlainText();
                    QStringList lines = text.split('\n');
                    QRegularExpression errorRegex(COPY_ERROR_TEXT);

                    QString previousCommand;

                    for (const QString &line : lines) {
                        if (errorRegex.match(line).hasMatch()) {
                            qDebug() << "Ошибка в команде:" << previousCommand;
                            emit outputReady("ERROR in command: " + previousCommand);
                        } else {
                            previousCommand = line;
                        }
                    }
                }
                ui->PB_startBuild->setEnabled(true);
            }
        });
        m_procThread->start();

    } else {
        qDebug() << "Failed to create script file!";
        emit statusChanged(ui->L_status->text() + "<span style='color:red;'>Произошла ошибка</span>");
        emit outputReady("Failed to create script file!");
        ui->PB_startBuild->setEnabled(true);
        return;
    }
}
