#include "LogManager.h"

#include "LogWriter.h"

#include <QDir>
#include <QJsonDocument>
#include <QMetaObject>

LogManager::LogManager(QObject *parent)
    : QAbstractListModel(parent)
    , m_writer(new LogWriter)
{
    m_writerThread.setObjectName(QStringLiteral("LogWriterThread"));
    m_writer->moveToThread(&m_writerThread);

    connect(&m_writerThread, &QThread::finished, m_writer, &QObject::deleteLater);
    connect(this, &LogManager::writeRequested, m_writer, &LogWriter::writeLine, Qt::QueuedConnection);
    connect(this, &LogManager::pruneRequested, m_writer, &LogWriter::pruneOldLogs, Qt::QueuedConnection);
    connect(m_writer, &LogWriter::writeFailed, this, &LogManager::setWriterError, Qt::QueuedConnection);
    m_writerThread.start();
}

LogManager::~LogManager()
{
    if (m_writerThread.isRunning()) {
        // The blocking barrier preserves queued log lines before the writer event loop stops.
        QMetaObject::invokeMethod(m_writer, [] {}, Qt::BlockingQueuedConnection);
        m_writerThread.quit();
        m_writerThread.wait();
    }
}

int LogManager::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant LogManager::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};

    const Entry &entry = m_entries.at(index.row());
    switch (role) {
    case TimestampRole:
        return entry.timestamp.toString(Qt::ISODateWithMs);
    case LevelRole:
        return levelName(entry.level);
    case CategoryRole:
        return categoryName(entry.category);
    case EventRole:
        return entry.event;
    case ComponentRole:
        return entry.component;
    case MessageRole:
        return entry.message;
    case VehicleIdRole:
        return entry.vehicleId;
    case RequestIdRole:
        return entry.requestId;
    case DisplayRole:
        return displayName(entry.display);
    default:
        return {};
    }
}

QHash<int, QByteArray> LogManager::roleNames() const
{
    return {
        {TimestampRole, "timestamp"},
        {LevelRole, "logLevel"},
        {CategoryRole, "logCategory"},
        {EventRole, "eventName"},
        {ComponentRole, "component"},
        {MessageRole, "logMessage"},
        {VehicleIdRole, "vehicleId"},
        {RequestIdRole, "requestId"},
        {DisplayRole, "displayMode"}
    };
}

int LogManager::count() const { return m_entries.size(); }
QString LogManager::logDirectory() const { return m_logDirectory; }
QUrl LogManager::logDirectoryUrl() const { return QUrl::fromLocalFile(m_logDirectory); }
QString LogManager::writerError() const { return m_writerError; }

QString LogManager::currentLogFilePath() const
{
    return QDir(m_logDirectory).filePath(QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd.jsonl")));
}

bool LogManager::initialize(const QString &directoryPath,
                            int retentionDays,
                            const QString &minimumLevel,
                            int maxDisplayEntries)
{
    Level parsedLevel;
    if (!parseLevel(minimumLevel, parsedLevel)) {
        setWriterError(QStringLiteral("不支持的最低日志等级：%1").arg(minimumLevel));
        return false;
    }
    if (retentionDays < 1 || maxDisplayEntries < 1) {
        setWriterError(QStringLiteral("日志保留天数和界面条数必须大于零"));
        return false;
    }

    QDir directory(directoryPath);
    if (!directory.mkpath(QStringLiteral("."))) {
        setWriterError(QStringLiteral("无法创建日志目录：%1").arg(directoryPath));
        return false;
    }

    m_logDirectory = directory.absolutePath();
    m_minimumLevel = parsedLevel;
    m_maxDisplayEntries = maxDisplayEntries;
    m_initialized = true;
    setWriterError({});
    emit logDirectoryChanged();
    emit pruneRequested(m_logDirectory, retentionDays);
    return true;
}

void LogManager::addEntry(Level level,
                          Category category,
                          Display display,
                          const QString &event,
                          const QString &component,
                          const QString &message,
                          const QString &vehicleId,
                          const QString &requestId,
                          const QJsonObject &details)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, [=, this] {
            addEntry(level, category, display, event, component, message, vehicleId, requestId, details);
        }, Qt::QueuedConnection);
        return;
    }

    if (levelRank(level) < levelRank(m_minimumLevel))
        return;

    QDateTime timestamp = QDateTime::currentDateTime();
    timestamp = timestamp.toOffsetFromUtc(timestamp.offsetFromUtc());
    const Entry entry {
        timestamp, level, category, display, event, component,
        message, vehicleId, requestId, details
    };

    if (display != Display::FileOnly) {
        if (m_entries.size() >= m_maxDisplayEntries) {
            const int removeCount = m_entries.size() - m_maxDisplayEntries + 1;
            beginRemoveRows({}, 0, removeCount - 1);
            m_entries.remove(0, removeCount);
            endRemoveRows();
        }
        const int row = m_entries.size();
        beginInsertRows({}, row, row);
        m_entries.append(entry);
        endInsertRows();
        emit countChanged();
    }

    if (m_initialized)
        emit writeRequested(currentLogFilePath(), serialize(entry));
}

void LogManager::clearDisplay()
{
    if (!m_entries.isEmpty()) {
        beginResetModel();
        m_entries.clear();
        endResetModel();
        emit countChanged();
    }

    addEntry(Level::Info,
             Category::Operation,
             Display::FileOnly,
             QStringLiteral("log_display_cleared"),
             QStringLiteral("LogManager"),
             QStringLiteral("清空当前日志显示"));
}

QString LogManager::levelName(Level level)
{
    switch (level) {
    case Level::Debug: return QStringLiteral("debug");
    case Level::Info: return QStringLiteral("info");
    case Level::Warning: return QStringLiteral("warning");
    case Level::Error: return QStringLiteral("error");
    case Level::Critical: return QStringLiteral("critical");
    }
    return QStringLiteral("info");
}

QString LogManager::categoryName(Category category)
{
    switch (category) {
    case Category::System: return QStringLiteral("system");
    case Category::Configuration: return QStringLiteral("configuration");
    case Category::Communication: return QStringLiteral("communication");
    case Category::Operation: return QStringLiteral("operation");
    case Category::Command: return QStringLiteral("command");
    case Category::State: return QStringLiteral("state");
    case Category::Alarm: return QStringLiteral("alarm");
    case Category::Error: return QStringLiteral("error");
    }
    return QStringLiteral("system");
}

QString LogManager::displayName(Display display)
{
    switch (display) {
    case Display::Primary: return QStringLiteral("primary");
    case Display::Diagnostic: return QStringLiteral("diagnostic");
    case Display::FileOnly: return QStringLiteral("file_only");
    }
    return QStringLiteral("diagnostic");
}

bool LogManager::parseLevel(const QString &name, Level &level)
{
    const QString normalized = name.trimmed().toLower();
    if (normalized == QStringLiteral("debug")) level = Level::Debug;
    else if (normalized == QStringLiteral("info")) level = Level::Info;
    else if (normalized == QStringLiteral("warning")) level = Level::Warning;
    else if (normalized == QStringLiteral("error")) level = Level::Error;
    else if (normalized == QStringLiteral("critical")) level = Level::Critical;
    else return false;
    return true;
}

int LogManager::levelRank(Level level)
{
    return static_cast<int>(level);
}

void LogManager::setWriterError(const QString &message)
{
    if (m_writerError == message)
        return;
    m_writerError = message;
    emit writerErrorChanged();
}

QByteArray LogManager::serialize(const Entry &entry) const
{
    QJsonObject object;
    object.insert(QStringLiteral("version"), 1);
    object.insert(QStringLiteral("timestamp"), entry.timestamp.toString(Qt::ISODateWithMs));
    object.insert(QStringLiteral("level"), levelName(entry.level));
    object.insert(QStringLiteral("category"), categoryName(entry.category));
    object.insert(QStringLiteral("display"), displayName(entry.display));
    object.insert(QStringLiteral("event"), entry.event);
    object.insert(QStringLiteral("component"), entry.component);
    object.insert(QStringLiteral("message"), entry.message);
    if (!entry.vehicleId.isEmpty())
        object.insert(QStringLiteral("vehicle_id"), entry.vehicleId);
    if (!entry.requestId.isEmpty())
        object.insert(QStringLiteral("request_id"), entry.requestId);
    if (!entry.details.isEmpty())
        object.insert(QStringLiteral("details"), entry.details);

    return QJsonDocument(object).toJson(QJsonDocument::Compact) + '\n';
}
