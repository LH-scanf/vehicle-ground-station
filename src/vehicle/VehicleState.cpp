#include "VehicleState.h"

#include <QtGlobal>

VehicleState::VehicleState(QObject *parent)
    : QObject(parent)
{
}

bool VehicleState::connected() const { return m_connected; }
QString VehicleState::mode() const { return m_mode; }
double VehicleState::speed() const { return m_speed; }
int VehicleState::batteryPercentage() const { return m_batteryPercentage; }
QString VehicleState::gpsStatus() const { return m_gpsStatus; }
bool VehicleState::emergencyStopActive() const { return m_emergencyStopActive; }

void VehicleState::setConnected(bool connected)
{
    if (m_connected == connected)
        return;
    m_connected = connected;
    emit connectedChanged();
}

void VehicleState::setMode(const QString &mode)
{
    if (m_mode == mode)
        return;
    m_mode = mode;
    emit modeChanged();
}

void VehicleState::setSpeed(double speed)
{
    if (qFuzzyCompare(m_speed, speed))
        return;
    m_speed = speed;
    emit speedChanged();
}

void VehicleState::setBatteryPercentage(int percentage)
{
    percentage = qBound(0, percentage, 100);
    if (m_batteryPercentage == percentage)
        return;
    m_batteryPercentage = percentage;
    emit batteryPercentageChanged();
}

void VehicleState::setGpsStatus(const QString &status)
{
    if (m_gpsStatus == status)
        return;
    m_gpsStatus = status;
    emit gpsStatusChanged();
}

void VehicleState::setEmergencyStopActive(bool active)
{
    if (m_emergencyStopActive == active)
        return;
    m_emergencyStopActive = active;
    emit emergencyStopActiveChanged();
}
