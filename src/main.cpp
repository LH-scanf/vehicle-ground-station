#include "config/ConfigManager.h"
#include "log/LogFilterModel.h"
#include "log/LogManager.h"
#include "vehicle/VehicleState.h"

#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>
#include <QDebug>

#include <cmath>

int main(int argc, char *argv[])
{
    QGuiApplication application(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("Vehicle Ground Station"));
    QGuiApplication::setOrganizationName(QStringLiteral("VehicleGroundStation"));

    ConfigManager configManager;
    const bool configurationLoaded = configManager.load();
    if (!configurationLoaded)
        qWarning() << "Unable to load the shared default configuration:" << configManager.errorMessage();

    QString logDirectory = configManager.logDirectory();
    if (QDir::isRelativePath(logDirectory)) {
        logDirectory = QDir(QCoreApplication::applicationDirPath()).filePath(logDirectory);
    }

    LogManager logManager;
    const bool loggingInitialized = logManager.initialize(
        logDirectory,
        configManager.logRetentionDays(),
        configManager.minimumLogLevel(),
        configManager.maxDisplayLogEntries());

    LogFilterModel logFilterModel;
    logFilterModel.setSourceModel(&logManager);

    logManager.addEntry(
        loggingInitialized ? LogManager::Level::Info : LogManager::Level::Critical,
        loggingInitialized ? LogManager::Category::System : LogManager::Category::Error,
        LogManager::Display::Primary,
        loggingInitialized ? QStringLiteral("application_started") : QStringLiteral("logging_initialization_failed"),
        QStringLiteral("Application"),
        loggingInitialized ? QStringLiteral("地面站启动成功") : QStringLiteral("日志模块初始化失败"),
        configManager.vehicleId());

    if (configurationLoaded && configManager.errorMessage().isEmpty()) {
        logManager.addEntry(LogManager::Level::Info,
                            LogManager::Category::Configuration,
                            LogManager::Display::Diagnostic,
                            QStringLiteral("configuration_loaded"),
                            QStringLiteral("ConfigManager"),
                            QStringLiteral("配置加载完成"),
                            configManager.vehicleId());
    } else if (!configManager.errorMessage().isEmpty()) {
        logManager.addEntry(LogManager::Level::Warning,
                            LogManager::Category::Configuration,
                            LogManager::Display::Primary,
                            QStringLiteral("configuration_warning"),
                            QStringLiteral("ConfigManager"),
                            configManager.errorMessage(),
                            configManager.vehicleId());
    }

    QObject::connect(&configManager, &ConfigManager::configurationSaved, &logManager, [&] {
        logManager.addEntry(LogManager::Level::Info,
                            LogManager::Category::Operation,
                            LogManager::Display::Primary,
                            QStringLiteral("configuration_saved"),
                            QStringLiteral("ConfigManager"),
                            QStringLiteral("车辆连接设置已保存"),
                            configManager.vehicleId());
    });
    QObject::connect(&configManager, &ConfigManager::errorMessageChanged, &logManager, [&] {
        if (!configManager.errorMessage().isEmpty()) {
            logManager.addEntry(LogManager::Level::Warning,
                                LogManager::Category::Configuration,
                                LogManager::Display::Primary,
                                QStringLiteral("configuration_error"),
                                QStringLiteral("ConfigManager"),
                                configManager.errorMessage(),
                                configManager.vehicleId());
        }
    });
    QObject::connect(&application, &QCoreApplication::aboutToQuit, &logManager, [&] {
        logManager.addEntry(LogManager::Level::Info,
                            LogManager::Category::System,
                            LogManager::Display::Diagnostic,
                            QStringLiteral("application_stopped"),
                            QStringLiteral("Application"),
                            QStringLiteral("地面站正常退出"),
                            configManager.vehicleId());
    });

    VehicleState vehicleState;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("configManager"), &configManager);
    engine.rootContext()->setContextProperty(QStringLiteral("logManager"), &logManager);
    engine.rootContext()->setContextProperty(QStringLiteral("logFilterModel"), &logFilterModel);
    engine.rootContext()->setContextProperty(QStringLiteral("vehicleState"), &vehicleState);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &application,
        [] { QCoreApplication::exit(EXIT_FAILURE); },
        Qt::QueuedConnection);

    engine.loadFromModule(QStringLiteral("VehicleGroundStation"), QStringLiteral("Main"));

    if (application.arguments().contains(QStringLiteral("--smoke-test")))
        return engine.rootObjects().isEmpty() ? EXIT_FAILURE : EXIT_SUCCESS;

    QTimer mockTimer;
    int mockTick = 0;
    QObject::connect(&mockTimer, &QTimer::timeout, &vehicleState, [&vehicleState, &mockTick] {
        ++mockTick;
        vehicleState.setSpeed(0.8 + 0.25 * std::sin(mockTick * 0.35));
        if (mockTick % 20 == 0)
            vehicleState.setBatteryPercentage(vehicleState.batteryPercentage() - 1);
    });
    mockTimer.start(500);

    return application.exec();
}
