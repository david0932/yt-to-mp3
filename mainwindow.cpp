#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QDir>
#include <QRegularExpression>
#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QUrl>

QString MainWindow::generateDownloadFilePath(const QString &extension) {
    // 使用日期和時間生成唯一的下載文件名
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMddHHmmss");
    //QString downloadFileName = "audio_" + timestamp + "." + extension;
    QString downloadFileName ; //= "audio_" + timestamp;
    if (ui->downloadPathLineEdit->text().isEmpty()){
        downloadFileName = "Download/";
    }
    else {
        downloadFileName = ui->downloadPathLineEdit->text() +"/";
    }
    //
    if (ui->filePrefixLineEdit->text().isEmpty()) {
        downloadFileName = downloadFileName + "audio_";
    }
    else {
        downloadFileName = downloadFileName + ui->filePrefixLineEdit->text(); // +"_" + timestamp;
    }
    //
    if (ui->fileSuffixLineEdit->text().isEmpty()) {
        downloadFileName = downloadFileName + "_" + timestamp;
    }
    else {
        downloadFileName = downloadFileName + "_" + ui->fileSuffixLineEdit->text();
    }
    return QDir::current().filePath(downloadFileName); // 保持下載文件在當前目錄
}

QString MainWindow::generateFinalFilePath(const QString &extension) {
    // 使用日期和時間生成最終的 MP3 文件名 -> NO USED
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMddHHmmss");
    QString finalFileName = "audio_" + timestamp + "." + extension;
    return QDir::current().filePath(finalFileName);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , downloadProcess(new QProcess(this))
    , convertProcess(new QProcess(this))
    , downloadedFilePath("")
    , finalMp3Path("")
    , totalDuration(0.0)
{
    ui->setupUi(this);

    // 連接下載過程的信號
    connect(downloadProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::onDownloadProcessReadyRead);
    connect(downloadProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::onDownloadProcessFinished);

    // 連接轉換過程的信號
    connect(convertProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::onConvertProcessReadyRead);
    connect(convertProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &MainWindow::onConvertProcessFinished);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_downloadButton_clicked()
{
    QString url_temp = ui->urlLineEdit->text().trimmed();

    QString url;

    if (url_temp.contains("&")) {
        url = url_temp.split("&").first();
    } else {
        url = url_temp;
    }

    if (url.isEmpty()) {
        QMessageBox::warning(this, "輸入錯誤", "請輸入有效的 YouTube URL。");
        return;
    }

    // 清空之前的狀態
    ui->statusLabel->setText("開始下載...");
    ui->progressBar->setValue(0);
    downloadedFilePath.clear();
    finalMp3Path.clear();
    totalDuration = 0.0;

    // 生成基於日期和時間的下載文件名
    QString downloadExtension = "opus"; // 您可以根據需要修改為其他格式，如 "m4a" 或 "webm"
    downloadedFilePath = generateDownloadFilePath(downloadExtension);
    qDebug() << "下載文件路徑:" << downloadedFilePath;

    // 配置 yt-dlp 下載參數
    QStringList arguments;
    arguments << "--extract-audio"            // 只提取音頻
              //<< "--audio-format" << "best"    // 使用最佳音頻格式
              << "--audio-format" << "mp3"    // 使用最佳音頻格式
              << "--audio-quality" << "0"       // 設置最高音質（可選）
              << "--newline"                    // 強制使用換行符號
              << "-o" << downloadedFilePath     // 設定輸出檔名格式，包含完整路徑
              << url;
    qDebug() << "yl-dlp參數:" << arguments;
    // 設置 yt-dlp 的工作目錄為當前應用程式目錄，避免相對路徑問題
    downloadProcess->setWorkingDirectory(QDir::currentPath());

    // 啟動 yt-dlp 進程
    downloadProcess->start("yt-dlp.exe", arguments);
    if (!downloadProcess->waitForStarted()) {
        QMessageBox::critical(this, "錯誤", "無法啟動 yt-dlp.exe。");
        ui->statusLabel->setText("無法啟動 yt-dlp.exe。");
        qDebug() << "無法啟動 yt-dlp.exe。";
    } else {
        qDebug() << "yt-dlp 進程已啟動。";
    }
}

void MainWindow::onDownloadProcessReadyRead()
{
    QByteArray output = downloadProcess->readAllStandardOutput();
    QString outputStr = QString::fromUtf8(output);
    ui->statusLabel->setText(outputStr);
    qDebug() << "yt-dlp 輸出:" << outputStr;

    // 使用 QRegularExpression 解析進度，例如 "100.0%"
    QRegularExpression rx("(\\d+\\.\\d+)%");
    QRegularExpressionMatch match = rx.match(outputStr);
    if (match.hasMatch()) {
        QString progress = match.captured(1);
        double progressDouble = progress.toDouble();
        int progressValue = static_cast<int>((progressDouble / 100.0) * 50.0); // 下載進度占總進度的50%
        ui->progressBar->setValue(progressValue);
        qDebug() << "下載進度:" << progressValue << "%";
    }

    // 解析視頻時長（如果有）
    QRegularExpression durationRx("Duration:\\s*(\\d{2}):(\\d{2}):(\\d{2})");
    QRegularExpressionMatch durationMatch = durationRx.match(outputStr);
    if (durationMatch.hasMatch()) {
        double hours = durationMatch.captured(1).toDouble();
        double minutes = durationMatch.captured(2).toDouble();
        double seconds = durationMatch.captured(3).toDouble();
        // 計算總時長（秒）
        totalDuration = hours * 3600 + minutes * 60 + seconds;
        qDebug() << "總時長:" << totalDuration << "秒";
    }
}

void MainWindow::onDownloadProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        ui->statusLabel->setText("下載完成。");
        ui->progressBar->setValue(100); // 下載完成後設置為50%
        qDebug() << "yt-dlp 進程已完成。";
        ui->urlLineEdit->clear();
/*
        if (QFile::exists(downloadedFilePath)) {
            qDebug() << "下載的音頻文件存在。";
            ui->statusLabel->setText("開始轉換為 MP3...");

            // 生成基於日期和時間的最終 MP3 文件名
            QString finalExtension = "mp3";
            finalMp3Path = generateFinalFilePath(finalExtension);
            qDebug() << "最終 MP3 文件路徑:" << finalMp3Path;

            // 配置 ffmpeg 轉換參數
            QStringList arguments;
            arguments << "-i" << QDir::toNativeSeparators(downloadedFilePath)    // 輸入文件，確保路徑使用本地分隔符
                      << "-ab" << "192k"                                      // 設定音頻位率為 192k
                      << "-ar" << "44100"                                     // 設定音頻採樣率為 44100Hz
                      << "-y"                                                 // 覆蓋輸出文件
                      << QDir::toNativeSeparators(finalMp3Path);             // 輸出 MP3 文件路徑

            // 設置 ffmpeg 的工作目錄為當前應用程式目錄
            convertProcess->setWorkingDirectory(QDir::currentPath());

            // 啟動 ffmpeg 進程
            convertProcess->start("ffmpeg.exe", arguments);
            if (!convertProcess->waitForStarted()) {
                QMessageBox::critical(this, "錯誤", "無法啟動 ffmpeg.exe。");
                ui->statusLabel->setText("無法啟動 ffmpeg.exe。");
                qDebug() << "無法啟動 ffmpeg.exe。";
            } else {
                qDebug() << "ffmpeg 進程已啟動。";
            }
        } else {
            QMessageBox::warning(this, "警告", "下載完成，但無法找到下載的文件。");
            ui->statusLabel->setText("下載完成，但無法找到下載的文件。");
            qDebug() << "無法找到下載的文件:" << downloadedFilePath;
        } */
    } else {
        QByteArray errorOutput = downloadProcess->readAllStandardError();
        QString errorStr = QString::fromUtf8(errorOutput);
        ui->statusLabel->setText("下載失敗。");
        QMessageBox::critical(this, "下載錯誤", errorStr);
        qDebug() << "yt-dlp 下載失敗:" << errorStr;
    }

}

void MainWindow::onConvertProcessReadyRead()
{
    QByteArray output = convertProcess->readAllStandardOutput();
    QString outputStr = QString::fromUtf8(output);
    ui->statusLabel->setText(outputStr);
    qDebug() << "ffmpeg 輸出:" << outputStr;

    // 使用 QRegularExpression 解析轉換進度，例如 "time=00:00:10.00"
    QRegularExpression rx("time=(\\d{2}):(\\d{2}):(\\d{2}\\.\\d{2})");
    QRegularExpressionMatch match = rx.match(outputStr);
    if (match.hasMatch()) {
        double hours = match.captured(1).toDouble();
        double minutes = match.captured(2).toDouble();
        double seconds = match.captured(3).toDouble();
        double currentTime = hours * 3600 + minutes * 60 + seconds;

        if (totalDuration > 0.0) {
            int progressValue = static_cast<int>((currentTime / totalDuration) * 50.0) + 50; // 轉換進度占總進度的50%，開始於50%
            if (progressValue > 100) progressValue = 100;
            ui->progressBar->setValue(progressValue);
            qDebug() << "轉換進度:" << progressValue << "%";
        }
    }
}

void MainWindow::onConvertProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        ui->statusLabel->setText("轉換完成。");
        ui->progressBar->setValue(100);
        qDebug() << "ffmpeg 轉換完成。";

        // 確認 MP3 文件是否存在
        if (QFile::exists(finalMp3Path)) {
            ui->statusLabel->setText("轉換完成: " + finalMp3Path);
            qDebug() << "成功生成 MP3 文件:" << finalMp3Path;

            // 刪除下載的音頻文件
            if (QFile::remove(downloadedFilePath)) {
                qDebug() << "成功刪除下載的音頻文件:" << downloadedFilePath;
            } else {
                qDebug() << "無法刪除下載的音頻文件:" << downloadedFilePath;
            }
        } else {
            QMessageBox::critical(this, "錯誤", "轉換完成，但無法找到 MP3 文件。");
            ui->statusLabel->setText("轉換完成，但無法找到 MP3 文件。");
            qDebug() << "轉換後的 MP3 文件不存在:" << finalMp3Path;
        }
    } else {
        QByteArray errorOutput = convertProcess->readAllStandardError();
        QString errorStr = QString::fromUtf8(errorOutput);
        ui->statusLabel->setText("轉換失敗。");
        QMessageBox::critical(this, "轉換錯誤", errorStr);
        qDebug() << "ffmpeg 轉換失敗:" << errorStr;
    }
}

void MainWindow::on_openButton_clicked()
{
    QString downloadPath;
    if (ui->downloadPathLineEdit->text().isEmpty()) {
        downloadPath = QDir::current().filePath("Download");
    } else {
        downloadPath = QDir::current().filePath(ui->downloadPathLineEdit->text());
    }

    // 確保目錄存在
    QDir dir(downloadPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 使用系統默認的文件管理器打開目錄
    QUrl url = QUrl::fromLocalFile(downloadPath);
    QDesktopServices::openUrl(url);
}
