#include "config/ConfigManager.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

class ConfigManagerTest final : public QObject
{
    Q_OBJECT

private slots:
    void loadsDefaultsAndLocalOverrides();
    void ignoresMalformedLocalConfiguration();
    void validatesAndSavesConfiguration();

private:
    static void writeFile(const QString &path, const QByteArray &contents);
};

void ConfigManagerTest::loadsDefaultsAndLocalOverrides()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString defaults = directory.filePath(QStringLiteral("defaults.json"));
    const QString local = directory.filePath(QStringLiteral("local.json"));
    writeFile(defaults, R"({"connection":{"vehicle_ip":"192.168.1.10","vehicle_port":8765,"vehicle_id":"car_01","auto_connect":false,"auto_reconnect":true},"logging":{"directory":"logs","retention_days":30,"minimum_level":"info","max_display_entries":1000}})");
    writeFile(local, R"({"connection":{"vehicle_ip":"10.0.0.8","vehicle_port":9000,"auto_connect":true}})");

    ConfigManager manager;
    QVERIFY(manager.loadFromFiles(defaults, local));
    QCOMPARE(manager.vehicleIp(), QStringLiteral("10.0.0.8"));
    QCOMPARE(manager.vehiclePort(), 9000);
    QCOMPARE(manager.vehicleId(), QStringLiteral("car_01"));
    QVERIFY(manager.autoConnect());
    QVERIFY(manager.autoReconnect());
    QCOMPARE(manager.logDirectory(), QStringLiteral("logs"));
    QCOMPARE(manager.logRetentionDays(), 30);
    QVERIFY(manager.errorMessage().isEmpty());
}

void ConfigManagerTest::ignoresMalformedLocalConfiguration()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString defaults = directory.filePath(QStringLiteral("defaults.json"));
    const QString local = directory.filePath(QStringLiteral("local.json"));
    writeFile(defaults, R"({"connection":{"vehicle_ip":"192.168.1.10","vehicle_port":8765,"vehicle_id":"car_01","auto_connect":false,"auto_reconnect":true},"logging":{"directory":"logs","retention_days":30,"minimum_level":"info","max_display_entries":1000}})");
    writeFile(local, QByteArrayLiteral("{invalid json"));

    ConfigManager manager;
    QVERIFY(manager.loadFromFiles(defaults, local));
    QCOMPARE(manager.vehicleIp(), QStringLiteral("192.168.1.10"));
    QCOMPARE(manager.vehiclePort(), 8765);
    QVERIFY(!manager.errorMessage().isEmpty());
}

void ConfigManagerTest::validatesAndSavesConfiguration()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString output = directory.filePath(QStringLiteral("nested/local.json"));

    ConfigManager manager;
    manager.setVehicleIp(QStringLiteral("not-an-ip"));
    QVERIFY(!manager.saveToFile(output));
    QVERIFY(!manager.errorMessage().isEmpty());

    manager.setVehicleIp(QStringLiteral("172.16.0.5"));
    manager.setVehiclePort(8080);
    manager.setVehicleId(QStringLiteral("vehicle_test"));
    manager.setAutoConnect(true);
    QVERIFY(manager.saveToFile(output));
    QVERIFY(QFileInfo::exists(output));
    QVERIFY(manager.errorMessage().isEmpty());
}

void ConfigManagerTest::writeFile(const QString &path, const QByteArray &contents)
{
    QFile file(path);
    QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(file.errorString()));
    QCOMPARE(file.write(contents), contents.size());
}

QTEST_MAIN(ConfigManagerTest)
#include "config_manager_test.moc"
