#pragma once

#include <QObject>
#include <QString>

class VehicleState final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected WRITE setConnected NOTIFY connectedChanged)
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(double x READ x WRITE setX NOTIFY positionChanged)
    Q_PROPERTY(double y READ y WRITE setY NOTIFY positionChanged)
    Q_PROPERTY(double yaw READ yaw WRITE setYaw NOTIFY yawChanged)
    Q_PROPERTY(double headingDegrees READ headingDegrees NOTIFY yawChanged)
    Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(int batteryPercentage READ batteryPercentage WRITE setBatteryPercentage NOTIFY batteryPercentageChanged)
    Q_PROPERTY(QString gpsStatus READ gpsStatus WRITE setGpsStatus NOTIFY gpsStatusChanged)
    Q_PROPERTY(bool emergencyStopActive READ emergencyStopActive WRITE setEmergencyStopActive NOTIFY emergencyStopActiveChanged)
    Q_PROPERTY(bool rcLink READ rcLink WRITE setRcLink NOTIFY rcLinkChanged)
    Q_PROPERTY(bool communicationTimeout READ communicationTimeout WRITE setCommunicationTimeout NOTIFY communicationTimeoutChanged)
    Q_PROPERTY(int errorCode READ errorCode WRITE setErrorCode NOTIFY errorCodeChanged)
    Q_PROPERTY(qint64 lastUpdateTimestamp READ lastUpdateTimestamp WRITE setLastUpdateTimestamp NOTIFY lastUpdateTimestampChanged)

public:
    explicit VehicleState(QObject *parent = nullptr);

    [[nodiscard]] bool connected() const;
    [[nodiscard]] QString mode() const;
    [[nodiscard]] double x() const;
    [[nodiscard]] double y() const;
    [[nodiscard]] double yaw() const;
    [[nodiscard]] double headingDegrees() const;
    [[nodiscard]] double speed() const;
    [[nodiscard]] int batteryPercentage() const;
    [[nodiscard]] QString gpsStatus() const;
    [[nodiscard]] bool emergencyStopActive() const;
    [[nodiscard]] bool rcLink() const;
    [[nodiscard]] bool communicationTimeout() const;
    [[nodiscard]] int errorCode() const;
    [[nodiscard]] qint64 lastUpdateTimestamp() const;

public slots:
    void setConnected(bool connected);
    void setMode(const QString &mode);
    void setX(double x);
    void setY(double y);
    void setYaw(double yaw);
    void setSpeed(double speed);
    void setBatteryPercentage(int percentage);
    void setGpsStatus(const QString &status);
    void setEmergencyStopActive(bool active);
    void setRcLink(bool available);
    void setCommunicationTimeout(bool timedOut);
    void setErrorCode(int code);
    void setLastUpdateTimestamp(qint64 timestamp);

signals:
    void connectedChanged();
    void modeChanged();
    void positionChanged();
    void yawChanged();
    void speedChanged();
    void batteryPercentageChanged();
    void gpsStatusChanged();
    void emergencyStopActiveChanged();
    void rcLinkChanged();
    void communicationTimeoutChanged();
    void errorCodeChanged();
    void lastUpdateTimestampChanged();

private:
    bool m_connected = false;
    QString m_mode = QStringLiteral("unknown");
    double m_x = 0.0;
    double m_y = 0.0;
    double m_yaw = 0.0;
    double m_speed = 0.0;
    int m_batteryPercentage = 0;
    QString m_gpsStatus = QStringLiteral("未知");
    bool m_emergencyStopActive = false;
    bool m_rcLink = false;
    bool m_communicationTimeout = false;
    int m_errorCode = 0;
    qint64 m_lastUpdateTimestamp = 0;
};
