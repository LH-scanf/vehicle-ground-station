#include "LogWriter.h"

#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>

LogWriter::LogWriter(QObject *parent)
    : QObject(parent)
{
}

void LogWriter::writeLine(const QString &filePath, const QByteArray &line)
{
    const QFileInfo fileInfo(filePath);
    QDir directory;
    if (!directory.mkpath(fileInfo.absolutePath())) {
        emit writeFailed(QStringLiteral("无法创建日志目录：%1").arg(fileInfo.absolutePath()));
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        emit writeFailed(QStringLiteral("无法写入日志文件：%1").arg(file.errorString()));
        return;
    }

    if (file.write(line) != line.size())
        emit writeFailed(QStringLiteral("日志内容未完整写入：%1").arg(file.errorString()));
}

void LogWriter::pruneOldLogs(const QString &directoryPath, int retentionDays)
{
    const QDate oldestDate = QDate::currentDate().addDays(-retentionDays + 1);
    const QDir directory(directoryPath);
    const QFileInfoList files = directory.entryInfoList(
        {QStringLiteral("*.jsonl")}, QDir::Files | QDir::NoSymLinks);

    for (const QFileInfo &file : files) {
        const QDate fileDate = QDate::fromString(file.completeBaseName(), QStringLiteral("yyyy-MM-dd"));
        if (fileDate.isValid() && fileDate < oldestDate && !QFile::remove(file.absoluteFilePath()))
            emit writeFailed(QStringLiteral("无法清理过期日志：%1").arg(file.absoluteFilePath()));
    }
}
