#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>

struct ProtocolMessage
{
    QJsonObject root;
    QJsonObject data;
    QString type;
    QString name;
    QString vehicleId;
    QString requestId;
    quint32 seq = 0;
    qint64 timestamp = 0;
};

struct ProtocolValidationResult
{
    bool valid = false;
    ProtocolMessage message;
    QString errorCode;
    QString errorMessage;
};

class ProtocolValidator final
{
public:
    static constexpr qsizetype MaximumTextMessageBytes = 256 * 1024;

    [[nodiscard]] static ProtocolValidationResult parseIncoming(
        const QByteArray &utf8Payload,
        const QString &expectedVehicleId);

    [[nodiscard]] static bool validateGatewayReady(const ProtocolMessage &message,
                                                   QString &errorCode,
                                                   QString &errorMessage);
    [[nodiscard]] static bool validatePong(const ProtocolMessage &message,
                                           QString &errorCode,
                                           QString &errorMessage);
    [[nodiscard]] static bool validateVehicleStatus(const ProtocolMessage &message,
                                                    QString &errorCode,
                                                    QString &errorMessage);
    [[nodiscard]] static bool validateCommandAck(const ProtocolMessage &message,
                                                 QString &stage,
                                                 QString &code,
                                                 QString &ackMessage,
                                                 QString &errorCode,
                                                 QString &errorMessage);

    [[nodiscard]] static QJsonObject makeHeartbeatPing(const QString &vehicleId,
                                                       quint32 seq,
                                                       qint64 timestamp);
    [[nodiscard]] static QJsonObject makeSetModeCommand(const QString &vehicleId,
                                                        quint32 seq,
                                                        qint64 timestamp,
                                                        const QString &requestId,
                                                        const QString &mode);
};
