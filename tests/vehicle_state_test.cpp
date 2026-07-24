#include "vehicle/VehicleState.h"

#include <QSignalSpy>
#include <QtTest>

class VehicleStateTest final : public QObject
{
    Q_OBJECT

private slots:
    void startsInConservativeOfflineState();
    void updatesPropertiesAndEmitsSignals();
    void convertsYawToDisplayHeading();
    void clampsBatteryPercentage();
};

void VehicleStateTest::startsInConservativeOfflineState()
{
    VehicleState state;

    QVERIFY(!state.connected());
    QCOMPARE(state.mode(), QStringLiteral("unknown"));
    QCOMPARE(state.speed(), 0.0);
    QCOMPARE(state.batteryPercentage(), 0);
    QVERIFY(!state.rcLink());
    QVERIFY(!state.emergencyStopActive());
    QVERIFY(!state.communicationTimeout());
    QCOMPARE(state.errorCode(), 0);
    QCOMPARE(state.lastUpdateTimestamp(), 0);
}

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

void VehicleStateTest::convertsYawToDisplayHeading()
{
    VehicleState state;

    state.setYaw(-3.14159265358979323846 / 2.0);
    QVERIFY(qAbs(state.headingDegrees() - 270.0) < 0.0001);

    state.setYaw(5.0 * 3.14159265358979323846 / 2.0);
    QVERIFY(qAbs(state.headingDegrees() - 90.0) < 0.0001);
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
