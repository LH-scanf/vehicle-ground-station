#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QJsonObject>
#include <QThread>
#include <QUrl>
#include <QVector>

class LogWriter;

class LogManager final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString logDirectory READ logDirectory NOTIFY logDirectoryChanged)
    Q_PROPERTY(QUrl logDirectoryUrl READ logDirectoryUrl NOTIFY logDirectoryChanged)
    Q_PROPERTY(QString writerError READ writerError NOTIFY writerErrorChanged)

public:
    enum Role {
        TimestampRole = Qt::UserRole + 1,
        LevelRole,
        CategoryRole,
        EventRole,
        ComponentRole,
        MessageRole,
        VehicleIdRole,
        RequestIdRole,
        DisplayRole
    };

    enum class Level { Debug, Info, Warning, Error, Critical };
    Q_ENUM(Level)

    enum class Category {
        System,
        Configuration,
        Communication,
        Operation,
        Command,
        State,
        Alarm,
        Error
    };
    Q_ENUM(Category)

    enum class Display { Primary, Diagnostic, FileOnly };
    Q_ENUM(Display)

    explicit LogManager(QObject *parent = nullptr);
    ~LogManager() override;

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] int count() const;
    [[nodiscard]] QString logDirectory() const;
    [[nodiscard]] QUrl logDirectoryUrl() const;
    [[nodiscard]] QString writerError() const;
    [[nodiscard]] QString currentLogFilePath() const;

    bool initialize(const QString &directoryPath,
                    int retentionDays = 30,
                    const QString &minimumLevel = QStringLiteral("info"),
                    int maxDisplayEntries = 1000);

    void addEntry(Level level,
                  Category category,
                  Display display,
                  const QString &event,
                  const QString &component,
                  const QString &message,
                  const QString &vehicleId = {},
                  const QString &requestId = {},
                  const QJsonObject &details = {});

    Q_INVOKABLE void clearDisplay();

    static QString levelName(Level level);
    static QString categoryName(Category category);
    static QString displayName(Display display);

signals:
    void countChanged();
    void logDirectoryChanged();
    void writerErrorChanged();
    void writeRequested(const QString &filePath, const QByteArray &line);
    void pruneRequested(const QString &directoryPath, int retentionDays);

private:
    struct Entry {
        QDateTime timestamp;
        Level level;
        Category category;
        Display display;
        QString event;
        QString component;
        QString message;
        QString vehicleId;
        QString requestId;
        QJsonObject details;
    };

    static bool parseLevel(const QString &name, Level &level);
    static int levelRank(Level level);
    void setWriterError(const QString &message);
    QByteArray serialize(const Entry &entry) const;

    QVector<Entry> m_entries;
    QString m_logDirectory;
    QString m_writerError;
    Level m_minimumLevel = Level::Info;
    int m_maxDisplayEntries = 1000;
    bool m_initialized = false;
    QThread m_writerThread;
    LogWriter *m_writer = nullptr;
};
