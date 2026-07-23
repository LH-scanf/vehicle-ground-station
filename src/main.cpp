#include "vehicle/VehicleState.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTimer>

#include <cmath>

int main(int argc, char *argv[])
{
    QGuiApplication application(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("Vehicle Ground Station"));
    QGuiApplication::setOrganizationName(QStringLiteral("VehicleGroundStation"));

    VehicleState vehicleState;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("vehicleState"), &vehicleState);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &application,
        [] { QCoreApplication::exit(EXIT_FAILURE); },
        Qt::QueuedConnection);

    engine.loadFromModule(QStringLiteral("VehicleGroundStation"), QStringLiteral("Main"));

    QTimer mockTimer;
    int mockTick = 0;
    QObject::connect(&mockTimer, &QTimer::timeout, &vehicleState, [&vehicleState, &mockTick] {
        ++mockTick;
        vehicleState.setSpeed(0.8 + 0.25 * std::sin(mockTick * 0.35));
        if (mockTick % 20 == 0)
            vehicleState.setBatteryPercentage(vehicleState.batteryPercentage() - 1);
    });
    mockTimer.start(500);

    if (application.arguments().contains(QStringLiteral("--smoke-test")))
        QTimer::singleShot(250, &application, &QCoreApplication::quit);

    return application.exec();
}
