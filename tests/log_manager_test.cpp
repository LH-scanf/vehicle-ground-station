#include "log/LogFilterModel.h"
#include "log/LogManager.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QtTest>

class LogManagerTest final : public QObject
{
    Q_OBJECT

private slots:
    void writesStructuredLogAndExposesModelRoles();
    void filtersDisplayLevelCategoryAndSearchText();
    void clearDisplayDoesNotDeleteTheLogFile();
};

void LogManagerTest::writesStructuredLogAndExposesModelRoles()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    LogManager manager;
    QVERIFY(manager.initialize(directory.path(), 30, QStringLiteral("info"), 100));
    manager.addEntry(LogManager::Level::Info,
                     LogManager::Category::Command,
                     LogManager::Display::Primary,
                     QStringLiteral("command_sent"),
                     QStringLiteral("ControlManager"),
                     QStringLiteral("发送模式切换命令"),
                     QStringLiteral("car_01"),
                     QStringLiteral("cmd_1002"),
                     {{QStringLiteral("mode"), QStringLiteral("ground")}});

    QCOMPARE(manager.rowCount(), 1);
    const QModelIndex index = manager.index(0, 0);
    QCOMPARE(manager.data(index, LogManager::LevelRole).toString(), QStringLiteral("info"));
    QCOMPARE(manager.data(index, LogManager::CategoryRole).toString(), QStringLiteral("command"));
    QCOMPARE(manager.data(index, LogManager::RequestIdRole).toString(), QStringLiteral("cmd_1002"));

    QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(manager.currentLogFilePath()), 2000);
    QFile file(manager.currentLogFilePath());
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QJsonDocument document = QJsonDocument::fromJson(file.readLine());
    QVERIFY(document.isObject());
    const QString timestamp = document.object().value(QStringLiteral("timestamp")).toString();
    QVERIFY(QRegularExpression(QStringLiteral("(Z|[+-]\\d{2}:\\d{2})$")).match(timestamp).hasMatch());
    QCOMPARE(document.object().value(QStringLiteral("event")).toString(), QStringLiteral("command_sent"));
    QCOMPARE(document.object().value(QStringLiteral("vehicle_id")).toString(), QStringLiteral("car_01"));
}

void LogManagerTest::filtersDisplayLevelCategoryAndSearchText()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    LogManager manager;
    QVERIFY(manager.initialize(directory.path(), 30, QStringLiteral("info"), 100));
    manager.addEntry(LogManager::Level::Info,
                     LogManager::Category::System,
                     LogManager::Display::Primary,
                     QStringLiteral("application_started"),
                     QStringLiteral("Application"),
                     QStringLiteral("地面站启动成功"));
    manager.addEntry(LogManager::Level::Warning,
                     LogManager::Category::Configuration,
                     LogManager::Display::Diagnostic,
                     QStringLiteral("configuration_invalid"),
                     QStringLiteral("ConfigManager"),
                     QStringLiteral("本机配置字段无效"));
    manager.addEntry(LogManager::Level::Error,
                     LogManager::Category::Error,
                     LogManager::Display::FileOnly,
                     QStringLiteral("internal_error"),
                     QStringLiteral("Test"),
                     QStringLiteral("仅写入文件"));

    QCOMPARE(manager.rowCount(), 2);

    LogFilterModel filter;
    filter.setSourceModel(&manager);
    QCOMPARE(filter.rowCount(), 1);

    filter.setShowTechnical(true);
    QCOMPARE(filter.rowCount(), 2);

    filter.setLevelFilter(QStringLiteral("warning"));
    QCOMPARE(filter.rowCount(), 1);

    filter.setCategoryFilter(QStringLiteral("configuration"));
    QCOMPARE(filter.rowCount(), 1);

    filter.setSearchText(QStringLiteral("字段无效"));
    QCOMPARE(filter.rowCount(), 1);
    filter.setSearchText(QStringLiteral("不存在"));
    QCOMPARE(filter.rowCount(), 0);
}

void LogManagerTest::clearDisplayDoesNotDeleteTheLogFile()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    LogManager manager;
    QVERIFY(manager.initialize(directory.path(), 30, QStringLiteral("info"), 100));
    manager.addEntry(LogManager::Level::Info,
                     LogManager::Category::System,
                     LogManager::Display::Primary,
                     QStringLiteral("application_started"),
                     QStringLiteral("Application"),
                     QStringLiteral("地面站启动成功"));
    QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(manager.currentLogFilePath()), 2000);

    manager.clearDisplay();

    QCOMPARE(manager.rowCount(), 0);
    QVERIFY(QFileInfo::exists(manager.currentLogFilePath()));
}

QTEST_MAIN(LogManagerTest)
#include "log_manager_test.moc"
