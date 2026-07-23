#include "vehicle/VehicleState.h"

#include <QSignalSpy>
#include <QtTest>

class VehicleStateTest final : public QObject
{
    Q_OBJECT

private slots:
    void updatesPropertiesAndEmitsSignals();
    void clampsBatteryPercentage();
};

void VehicleStateTest::updatesPropertiesAndEmitsSignals()
{
    VehicleState state;
    QSignalSpy speedSpy(&state, &VehicleState::speedChanged);

    state.setSpeed(1.25);

    QCOMPARE(state.speed(), 1.25);
    QCOMPARE(speedSpy.count(), 1);
    state.setSpeed(1.25);
    QCOMPARE(speedSpy.count(), 1);
}

void VehicleStateTest::clampsBatteryPercentage()
{
    VehicleState state;

    state.setBatteryPercentage(120);
    QCOMPARE(state.batteryPercentage(), 100);

    state.setBatteryPercentage(-10);
    QCOMPARE(state.batteryPercentage(), 0);
}

QTEST_MAIN(VehicleStateTest)
#include "vehicle_state_test.moc"
