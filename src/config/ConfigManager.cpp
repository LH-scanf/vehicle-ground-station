#include "ConfigManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>

namespace {
constexpr auto connectionKey = "connection";
constexpr auto vehicleIpKey = "vehicle_ip";
constexpr auto vehiclePortKey = "vehicle_port";
constexpr auto vehicleIdKey = "vehicle_id";
constexpr auto autoConnectKey = "auto_connect";
constexpr auto autoReconnectKey = "auto_reconnect";
constexpr auto loggingKey = "logging";
constexpr auto logDirectoryKey = "directory";
constexpr auto retentionDaysKey = "retention_days";
constexpr auto minimumLevelKey = "minimum_level";
constexpr auto maxDisplayEntriesKey = "max_display_entries";

bool isValidIpv4(const QString &text)
{
    const QStringList parts = text.split(u'.', Qt::KeepEmptyParts);
    if (parts.size() != 4)
        return false;

    for (const QString &part : parts) {
        if (part.isEmpty() || part.size() > 3)
            return false;
        for (const QChar character : part) {
            if (character < u'0' || character > u'9')
                return false;
        }
        bool converted = false;
        const int value = part.toInt(&converted);
        if (!converted || value > 255)
            return false;
    }
    return true;
}
}

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
{
}

QString ConfigManager::vehicleIp() const { return m_vehicleIp; }
int ConfigManager::vehiclePort() const { return m_vehiclePort; }
QString ConfigManager::vehicleId() const { return m_vehicleId; }
bool ConfigManager::autoConnect() const { return m_autoConnect; }
bool ConfigManager::autoReconnect() const { return m_autoReconnect; }
QString ConfigManager::logDirectory() const { return m_logDirectory; }
int ConfigManager::logRetentionDays() const { return m_logRetentionDays; }
QString ConfigManager::minimumLogLevel() const { return m_minimumLogLevel; }
int ConfigManager::maxDisplayLogEntries() const { return m_maxDisplayLogEntries; }
QString ConfigManager::errorMessage() const { return m_errorMessage; }

bool ConfigManager::load()
{
    return loadFromFiles(QStringLiteral(":/config/default_config.json"), localConfigPath());
}

bool ConfigManager::loadFromFiles(const QString &defaultPath, const QString &localPath)
{
    setErrorMessage({});

    QJsonObject defaults;
    QString readError;
    if (!readObject(defaultPath, defaults, readError)) {
        setErrorMessage(readError);
        return false;
    }

    QStringList errors;
    applyConnectionObject(defaults, true, errors);
    applyLoggingObject(defaults, true, errors);
    if (!errors.isEmpty()) {
        setErrorMessage(QStringLiteral("默认配置无效：%1").arg(errors.join(QStringLiteral("；"))));
        return false;
    }

    if (localPath.isEmpty() || !QFileInfo::exists(localPath))
        return true;

    QJsonObject localOverrides;
    if (!readObject(localPath, localOverrides, readError)) {
        setErrorMessage(QStringLiteral("本地配置已忽略：%1").arg(readError));
        return true;
    }

    errors.clear();
    applyConnectionObject(localOverrides, false, errors);
    applyLoggingObject(localOverrides, false, errors);
    if (!errors.isEmpty())
        setErrorMessage(QStringLiteral("部分本地配置已忽略：%1").arg(errors.join(QStringLiteral("；"))));
    return true;
}

bool ConfigManager::save()
{
    return saveToFile(localConfigPath());
}

bool ConfigManager::saveToFile(const QString &path)
{
    QString validationError;
    if (!validate(validationError)) {
        setErrorMessage(validationError);
        return false;
    }

    const QFileInfo fileInfo(path);
    QDir directory;
    if (!directory.mkpath(fileInfo.absolutePath())) {
        setErrorMessage(QStringLiteral("无法创建配置目录：%1").arg(fileInfo.absolutePath()));
        return false;
    }

    QJsonObject connection;
    connection.insert(QString::fromLatin1(vehicleIpKey), m_vehicleIp.trimmed());
    connection.insert(QString::fromLatin1(vehiclePortKey), m_vehiclePort);
    connection.insert(QString::fromLatin1(vehicleIdKey), m_vehicleId.trimmed());
    connection.insert(QString::fromLatin1(autoConnectKey), m_autoConnect);
    connection.insert(QString::fromLatin1(autoReconnectKey), m_autoReconnect);

    QJsonObject root;
    root.insert(QString::fromLatin1(connectionKey), connection);

    QJsonObject logging;
    logging.insert(QString::fromLatin1(logDirectoryKey), m_logDirectory.trimmed());
    logging.insert(QString::fromLatin1(retentionDaysKey), m_logRetentionDays);
    logging.insert(QString::fromLatin1(minimumLevelKey), m_minimumLogLevel.trimmed().toLower());
    logging.insert(QString::fromLatin1(maxDisplayEntriesKey), m_maxDisplayLogEntries);
    root.insert(QString::fromLatin1(loggingKey), logging);

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        setErrorMessage(QStringLiteral("无法写入本地配置：%1").arg(file.errorString()));
        return false;
    }
    if (file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0 || !file.commit()) {
        setErrorMessage(QStringLiteral("保存本地配置失败：%1").arg(file.errorString()));
        return false;
    }

    setVehicleIp(m_vehicleIp.trimmed());
    setVehicleId(m_vehicleId.trimmed());
    setLogDirectory(m_logDirectory.trimmed());
    setMinimumLogLevel(m_minimumLogLevel.trimmed().toLower());
    setErrorMessage({});
    emit configurationSaved();
    return true;
}

void ConfigManager::setVehicleIp(const QString &ip)
{
    if (m_vehicleIp == ip)
        return;
    m_vehicleIp = ip;
    emit vehicleIpChanged();
}

void ConfigManager::setVehiclePort(int port)
{
    if (m_vehiclePort == port)
        return;
    m_vehiclePort = port;
    emit vehiclePortChanged();
}

void ConfigManager::setVehicleId(const QString &id)
{
    if (m_vehicleId == id)
        return;
    m_vehicleId = id;
    emit vehicleIdChanged();
}

void ConfigManager::setAutoConnect(bool enabled)
{
    if (m_autoConnect == enabled)
        return;
    m_autoConnect = enabled;
    emit autoConnectChanged();
}

void ConfigManager::setAutoReconnect(bool enabled)
{
    if (m_autoReconnect == enabled)
        return;
    m_autoReconnect = enabled;
    emit autoReconnectChanged();
}

void ConfigManager::setLogDirectory(const QString &directory)
{
    if (m_logDirectory == directory)
        return;
    m_logDirectory = directory;
    emit logDirectoryChanged();
}

void ConfigManager::setLogRetentionDays(int days)
{
    if (m_logRetentionDays == days)
        return;
    m_logRetentionDays = days;
    emit logRetentionDaysChanged();
}

void ConfigManager::setMinimumLogLevel(const QString &level)
{
    if (m_minimumLogLevel == level)
        return;
    m_minimumLogLevel = level;
    emit minimumLogLevelChanged();
}

void ConfigManager::setMaxDisplayLogEntries(int entries)
{
    if (m_maxDisplayLogEntries == entries)
        return;
    m_maxDisplayLogEntries = entries;
    emit maxDisplayLogEntriesChanged();
}

bool ConfigManager::readObject(const QString &path, QJsonObject &object, QString &error) const
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("无法读取 %1：%2").arg(path, file.errorString());
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        error = QStringLiteral("%1 不是有效的 JSON 对象：%2").arg(path, parseError.errorString());
        return false;
    }

    object = document.object();
    return true;
}

bool ConfigManager::applyConnectionObject(const QJsonObject &root, bool requireAllFields, QStringList &errors)
{
    const auto connectionValue = root.value(QString::fromLatin1(connectionKey));
    if (!connectionValue.isObject()) {
        if (requireAllFields)
            errors.append(QStringLiteral("缺少 connection 对象"));
        return !requireAllFields;
    }
    const QJsonObject connection = connectionValue.toObject();

    const auto require = [&](const char *key) {
        return requireAllFields && !connection.contains(QString::fromLatin1(key));
    };

    if (require(vehicleIpKey)) {
        errors.append(QStringLiteral("缺少 vehicle_ip"));
    } else if (connection.contains(QString::fromLatin1(vehicleIpKey))) {
        const auto value = connection.value(QString::fromLatin1(vehicleIpKey));
        if (!value.isString() || !isValidIpv4(value.toString().trimmed()))
            errors.append(QStringLiteral("vehicle_ip 格式无效"));
        else
            setVehicleIp(value.toString().trimmed());
    }

    if (require(vehiclePortKey)) {
        errors.append(QStringLiteral("缺少 vehicle_port"));
    } else if (connection.contains(QString::fromLatin1(vehiclePortKey))) {
        const auto value = connection.value(QString::fromLatin1(vehiclePortKey));
        const int port = value.toInt(-1);
        if (!value.isDouble() || value.toDouble() != port || port < 1 || port > 65535)
            errors.append(QStringLiteral("vehicle_port 必须在 1 到 65535 之间"));
        else
            setVehiclePort(port);
    }

    if (require(vehicleIdKey)) {
        errors.append(QStringLiteral("缺少 vehicle_id"));
    } else if (connection.contains(QString::fromLatin1(vehicleIdKey))) {
        const auto value = connection.value(QString::fromLatin1(vehicleIdKey));
        if (!value.isString() || value.toString().trimmed().isEmpty())
            errors.append(QStringLiteral("vehicle_id 不能为空"));
        else
            setVehicleId(value.toString().trimmed());
    }

    const auto applyBoolean = [&](const char *key, auto setter) {
        const QString jsonKey = QString::fromLatin1(key);
        if (require(key)) {
            errors.append(QStringLiteral("缺少 %1").arg(jsonKey));
        } else if (connection.contains(jsonKey)) {
            const auto value = connection.value(jsonKey);
            if (!value.isBool())
                errors.append(QStringLiteral("%1 必须是布尔值").arg(jsonKey));
            else
                (this->*setter)(value.toBool());
        }
    };
    applyBoolean(autoConnectKey, &ConfigManager::setAutoConnect);
    applyBoolean(autoReconnectKey, &ConfigManager::setAutoReconnect);

    return errors.isEmpty();
}

bool ConfigManager::applyLoggingObject(const QJsonObject &root, bool requireAllFields, QStringList &errors)
{
    const auto loggingValue = root.value(QString::fromLatin1(loggingKey));
    if (!loggingValue.isObject()) {
        if (requireAllFields)
            errors.append(QStringLiteral("缺少 logging 对象"));
        return !requireAllFields;
    }
    const QJsonObject logging = loggingValue.toObject();
    const auto require = [&](const char *key) {
        return requireAllFields && !logging.contains(QString::fromLatin1(key));
    };

    if (require(logDirectoryKey)) {
        errors.append(QStringLiteral("缺少 logging.directory"));
    } else if (logging.contains(QString::fromLatin1(logDirectoryKey))) {
        const auto value = logging.value(QString::fromLatin1(logDirectoryKey));
        if (!value.isString() || value.toString().trimmed().isEmpty())
            errors.append(QStringLiteral("logging.directory 不能为空"));
        else
            setLogDirectory(value.toString().trimmed());
    }

    if (require(retentionDaysKey)) {
        errors.append(QStringLiteral("缺少 logging.retention_days"));
    } else if (logging.contains(QString::fromLatin1(retentionDaysKey))) {
        const auto value = logging.value(QString::fromLatin1(retentionDaysKey));
        const int days = value.toInt(-1);
        if (!value.isDouble() || value.toDouble() != days || days < 1 || days > 3650)
            errors.append(QStringLiteral("logging.retention_days 必须在 1 到 3650 之间"));
        else
            setLogRetentionDays(days);
    }

    if (require(minimumLevelKey)) {
        errors.append(QStringLiteral("缺少 logging.minimum_level"));
    } else if (logging.contains(QString::fromLatin1(minimumLevelKey))) {
        const auto value = logging.value(QString::fromLatin1(minimumLevelKey));
        const QString level = value.toString().trimmed().toLower();
        const QStringList levels {
            QStringLiteral("debug"), QStringLiteral("info"), QStringLiteral("warning"),
            QStringLiteral("error"), QStringLiteral("critical")
        };
        if (!value.isString() || !levels.contains(level))
            errors.append(QStringLiteral("logging.minimum_level 无效"));
        else
            setMinimumLogLevel(level);
    }

    if (require(maxDisplayEntriesKey)) {
        errors.append(QStringLiteral("缺少 logging.max_display_entries"));
    } else if (logging.contains(QString::fromLatin1(maxDisplayEntriesKey))) {
        const auto value = logging.value(QString::fromLatin1(maxDisplayEntriesKey));
        const int entries = value.toInt(-1);
        if (!value.isDouble() || value.toDouble() != entries || entries < 100 || entries > 100000)
            errors.append(QStringLiteral("logging.max_display_entries 必须在 100 到 100000 之间"));
        else
            setMaxDisplayLogEntries(entries);
    }

    return errors.isEmpty();
}

bool ConfigManager::validate(QString &error) const
{
    if (!isValidIpv4(m_vehicleIp.trimmed())) {
        error = QStringLiteral("请输入有效的车辆 IP 地址");
        return false;
    }
    if (m_vehiclePort < 1 || m_vehiclePort > 65535) {
        error = QStringLiteral("车辆端口必须在 1 到 65535 之间");
        return false;
    }
    if (m_vehicleId.trimmed().isEmpty()) {
        error = QStringLiteral("车辆编号不能为空");
        return false;
    }
    if (m_logDirectory.trimmed().isEmpty()) {
        error = QStringLiteral("日志目录不能为空");
        return false;
    }
    if (m_logRetentionDays < 1 || m_logRetentionDays > 3650) {
        error = QStringLiteral("日志保留天数必须在 1 到 3650 之间");
        return false;
    }
    const QStringList levels {
        QStringLiteral("debug"), QStringLiteral("info"), QStringLiteral("warning"),
        QStringLiteral("error"), QStringLiteral("critical")
    };
    if (!levels.contains(m_minimumLogLevel.trimmed().toLower())) {
        error = QStringLiteral("最低日志等级无效");
        return false;
    }
    if (m_maxDisplayLogEntries < 100 || m_maxDisplayLogEntries > 100000) {
        error = QStringLiteral("界面日志条数必须在 100 到 100000 之间");
        return false;
    }
    return true;
}

QString ConfigManager::localConfigPath() const
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("config/local_config.json"));
}

void ConfigManager::setErrorMessage(const QString &message)
{
    if (m_errorMessage == message)
        return;
    m_errorMessage = message;
    emit errorMessageChanged();
}
