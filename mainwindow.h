#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_downloadButton_clicked();
    void onDownloadProcessReadyRead();
    void onDownloadProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onConvertProcessReadyRead();
    void onConvertProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void on_openButton_clicked();

private:
    Ui::MainWindow *ui;
    QProcess *downloadProcess;
    QProcess *convertProcess;
    QString downloadedFilePath;    // 下載的音頻文件路徑（基於日期和時間）
    QString finalMp3Path;          // 最終 MP3 文件路徑（基於日期和時間）
    double totalDuration;          // 視頻總時長（秒）

    // 用於生成基於日期和時間的唯一文件名稱
    QString generateDownloadFilePath(const QString &extension);
    QString generateFinalFilePath(const QString &extension);
};

#endif // MAINWINDOW_H
