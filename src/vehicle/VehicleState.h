#pragma once

#include <QObject>
#include <QString>

class VehicleState final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected WRITE setConnected NOTIFY connectedChanged)
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(int batteryPercentage READ batteryPercentage WRITE setBatteryPercentage NOTIFY batteryPercentageChanged)
    Q_PROPERTY(QString gpsStatus READ gpsStatus WRITE setGpsStatus NOTIFY gpsStatusChanged)
    Q_PROPERTY(bool emergencyStopActive READ emergencyStopActive WRITE setEmergencyStopActive NOTIFY emergencyStopActiveChanged)

public:
    explicit VehicleState(QObject *parent = nullptr);

    [[nodiscard]] bool connected() const;
    [[nodiscard]] QString mode() const;
    [[nodiscard]] double speed() const;
    [[nodiscard]] int batteryPercentage() const;
    [[nodiscard]] QString gpsStatus() const;
    [[nodiscard]] bool emergencyStopActive() const;

public slots:
    void setConnected(bool connected);
    void setMode(const QString &mode);
    void setSpeed(double speed);
    void setBatteryPercentage(int percentage);
    void setGpsStatus(const QString &status);
    void setEmergencyStopActive(bool active);

signals:
    void connectedChanged();
    void modeChanged();
    void speedChanged();
    void batteryPercentageChanged();
    void gpsStatusChanged();
    void emergencyStopActiveChanged();

private:
    bool m_connected = true;
    QString m_mode = QStringLiteral("auto");
    double m_speed = 0.8;
    int m_batteryPercentage = 82;
    QString m_gpsStatus = QStringLiteral("RTK Fixed");
    bool m_emergencyStopActive = false;
};
