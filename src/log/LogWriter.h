#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

class LogWriter final : public QObject
{
    Q_OBJECT

public:
    explicit LogWriter(QObject *parent = nullptr);

public slots:
    void writeLine(const QString &filePath, const QByteArray &line);
    void pruneOldLogs(const QString &directoryPath, int retentionDays);

signals:
    void writeFailed(const QString &message);
};
