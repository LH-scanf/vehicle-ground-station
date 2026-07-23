#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class QJsonObject;

class ConfigManager final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString vehicleIp READ vehicleIp WRITE setVehicleIp NOTIFY vehicleIpChanged)
    Q_PROPERTY(int vehiclePort READ vehiclePort WRITE setVehiclePort NOTIFY vehiclePortChanged)
    Q_PROPERTY(QString vehicleId READ vehicleId WRITE setVehicleId NOTIFY vehicleIdChanged)
    Q_PROPERTY(bool autoConnect READ autoConnect WRITE setAutoConnect NOTIFY autoConnectChanged)
    Q_PROPERTY(bool autoReconnect READ autoReconnect WRITE setAutoReconnect NOTIFY autoReconnectChanged)
    Q_PROPERTY(QString logDirectory READ logDirectory WRITE setLogDirectory NOTIFY logDirectoryChanged)
    Q_PROPERTY(int logRetentionDays READ logRetentionDays WRITE setLogRetentionDays NOTIFY logRetentionDaysChanged)
    Q_PROPERTY(QString minimumLogLevel READ minimumLogLevel WRITE setMinimumLogLevel NOTIFY minimumLogLevelChanged)
    Q_PROPERTY(int maxDisplayLogEntries READ maxDisplayLogEntries WRITE setMaxDisplayLogEntries NOTIFY maxDisplayLogEntriesChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit ConfigManager(QObject *parent = nullptr);

    [[nodiscard]] QString vehicleIp() const;
    [[nodiscard]] int vehiclePort() const;
    [[nodiscard]] QString vehicleId() const;
    [[nodiscard]] bool autoConnect() const;
    [[nodiscard]] bool autoReconnect() const;
    [[nodiscard]] QString logDirectory() const;
    [[nodiscard]] int logRetentionDays() const;
    [[nodiscard]] QString minimumLogLevel() const;
    [[nodiscard]] int maxDisplayLogEntries() const;
    [[nodiscard]] QString errorMessage() const;

    bool load();
    bool loadFromFiles(const QString &defaultPath, const QString &localPath);
    Q_INVOKABLE bool save();
    bool saveToFile(const QString &path);

public slots:
    void setVehicleIp(const QString &ip);
    void setVehiclePort(int port);
    void setVehicleId(const QString &id);
    void setAutoConnect(bool enabled);
    void setAutoReconnect(bool enabled);
    void setLogDirectory(const QString &directory);
    void setLogRetentionDays(int days);
    void setMinimumLogLevel(const QString &level);
    void setMaxDisplayLogEntries(int entries);

signals:
    void vehicleIpChanged();
    void vehiclePortChanged();
    void vehicleIdChanged();
    void autoConnectChanged();
    void autoReconnectChanged();
    void logDirectoryChanged();
    void logRetentionDaysChanged();
    void minimumLogLevelChanged();
    void maxDisplayLogEntriesChanged();
    void configurationSaved();
    void errorMessageChanged();

private:
    bool readObject(const QString &path, QJsonObject &object, QString &error) const;
    bool applyConnectionObject(const QJsonObject &root, bool requireAllFields, QStringList &errors);
    bool applyLoggingObject(const QJsonObject &root, bool requireAllFields, QStringList &errors);
    [[nodiscard]] bool validate(QString &error) const;
    [[nodiscard]] QString localConfigPath() const;
    void setErrorMessage(const QString &message);

    QString m_vehicleIp = QStringLiteral("192.168.1.10");
    int m_vehiclePort = 8765;
    QString m_vehicleId = QStringLiteral("car_01");
    bool m_autoConnect = false;
    bool m_autoReconnect = true;
    QString m_logDirectory = QStringLiteral("logs");
    int m_logRetentionDays = 30;
    QString m_minimumLogLevel = QStringLiteral("info");
    int m_maxDisplayLogEntries = 1000;
    QString m_errorMessage;
};
