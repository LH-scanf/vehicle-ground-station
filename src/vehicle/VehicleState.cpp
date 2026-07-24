#include "VehicleState.h"

#include <QtGlobal>

#include <cmath>

namespace {
constexpr double RadiansToDegrees = 180.0 / 3.14159265358979323846;
}

VehicleState::VehicleState(QObject *parent)
    : QObject(parent)
{
}

bool VehicleState::connected() const { return m_connected; }
QString VehicleState::mode() const { return m_mode; }
double VehicleState::x() const { return m_x; }
double VehicleState::y() const { return m_y; }
double VehicleState::yaw() const { return m_yaw; }
double VehicleState::headingDegrees() const
{
    // ROS yaw is counter-clockwise radians. Normalize only for operator display;
    // the authoritative protocol value remains m_yaw in radians.
    double degrees = std::fmod(m_yaw * RadiansToDegrees, 360.0);
    if (degrees < 0.0)
        degrees += 360.0;
    return degrees;
}
double VehicleState::speed() const { return m_speed; }
int VehicleState::batteryPercentage() const { return m_batteryPercentage; }
QString VehicleState::gpsStatus() const { return m_gpsStatus; }
bool VehicleState::emergencyStopActive() const { return m_emergencyStopActive; }
bool VehicleState::rcLink() const { return m_rcLink; }
bool VehicleState::communicationTimeout() const { return m_communicationTimeout; }
int VehicleState::errorCode() const { return m_errorCode; }
qint64 VehicleState::lastUpdateTimestamp() const { return m_lastUpdateTimestamp; }

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

void VehicleState::setX(double x)
{
    if (qFuzzyCompare(m_x, x))
        return;
    m_x = x;
    emit positionChanged();
}

void VehicleState::setY(double y)
{
    if (qFuzzyCompare(m_y, y))
        return;
    m_y = y;
    emit positionChanged();
}

void VehicleState::setYaw(double yaw)
{
    if (qFuzzyCompare(m_yaw, yaw))
        return;
    m_yaw = yaw;
    emit yawChanged();
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

void VehicleState::setRcLink(bool available)
{
    if (m_rcLink == available)
        return;
    m_rcLink = available;
    emit rcLinkChanged();
}

void VehicleState::setCommunicationTimeout(bool timedOut)
{
    if (m_communicationTimeout == timedOut)
        return;
    m_communicationTimeout = timedOut;
    emit communicationTimeoutChanged();
}

void VehicleState::setErrorCode(int code)
{
    code = qMax(0, code);
    if (m_errorCode == code)
        return;
    m_errorCode = code;
    emit errorCodeChanged();
}

void VehicleState::setLastUpdateTimestamp(qint64 timestamp)
{
    timestamp = qMax<qint64>(0, timestamp);
    if (m_lastUpdateTimestamp == timestamp)
        return;
    m_lastUpdateTimestamp = timestamp;
    emit lastUpdateTimestampChanged();
}
