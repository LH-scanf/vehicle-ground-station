#pragma once

#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QWebSocket>

class ConfigManager;
class LogManager;
class VehicleState;
struct ProtocolMessage;

class WebSocketClient final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString connectionState READ connectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(bool socketConnected READ socketConnected NOTIFY socketConnectedChanged)
    Q_PROPERTY(bool gatewayReady READ gatewayReady NOTIFY gatewayReadyChanged)
    Q_PROPERTY(int roundTripTimeMs READ roundTripTimeMs NOTIFY roundTripTimeMsChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QString endpoint READ endpoint NOTIFY endpointChanged)
    Q_PROPERTY(bool modeCommandAvailable READ modeCommandAvailable NOTIFY modeCommandAvailableChanged)
    Q_PROPERTY(bool modeCommandPending READ modeCommandPending NOTIFY modeCommandPendingChanged)
    Q_PROPERTY(QString requestedMode READ requestedMode NOTIFY modeCommandStateChanged)
    Q_PROPERTY(QString modeCommandStage READ modeCommandStage NOTIFY modeCommandStateChanged)
    Q_PROPERTY(QString modeCommandMessage READ modeCommandMessage NOTIFY modeCommandStateChanged)

public:
    explicit WebSocketClient(ConfigManager *configManager,
                             VehicleState *vehicleState,
                             LogManager *logManager,
                             QObject *parent = nullptr);

    [[nodiscard]] QString connectionState() const;
    [[nodiscard]] bool socketConnected() const;
    [[nodiscard]] bool gatewayReady() const;
    [[nodiscard]] int roundTripTimeMs() const;
    [[nodiscard]] QString lastError() const;
    [[nodiscard]] QString endpoint() const;
    [[nodiscard]] bool modeCommandAvailable() const;
    [[nodiscard]] bool modeCommandPending() const;
    [[nodiscard]] QString requestedMode() const;
    [[nodiscard]] QString modeCommandStage() const;
    [[nodiscard]] QString modeCommandMessage() const;

    Q_INVOKABLE void connectToGateway();
    Q_INVOKABLE void disconnectFromGateway();
    Q_INVOKABLE void requestModeChange(const QString &mode);

signals:
    void connectionStateChanged();
    void socketConnectedChanged();
    void gatewayReadyChanged();
    void roundTripTimeMsChanged();
    void lastErrorChanged();
    void endpointChanged();
    void modeCommandAvailableChanged();
    void modeCommandPendingChanged();
    void modeCommandStateChanged();

private slots:
    void handleConnected();
    void handleDisconnected();
    void handleTextMessage(const QString &message);
    void handleBinaryMessage(const QByteArray &message);
    void handleSocketError();
    void sendHeartbeat();
    void checkLiveness();
    void handleModeCommandTimeout();

private:
    void handleGatewayReady(const ProtocolMessage &message);
    void handlePong(const ProtocolMessage &message);
    void handleVehicleStatus(const ProtocolMessage &message);
    void handleCommandAck(const ProtocolMessage &message);
    void scheduleReconnect();
    void resetSessionState();
    void setConnectionState(const QString &state);
    void setSocketConnected(bool connected);
    void setGatewayReady(bool ready);
    void setRoundTripTimeMs(int milliseconds);
    void setLastError(const QString &message);
    void setEndpoint(const QString &endpoint);
    void setModeCommandAvailable(bool available);
    void setModeCommandState(const QString &stage, const QString &message, bool pending);
    void finishPendingModeCommand(const QString &stage, const QString &message);
    quint32 takeNextSequence();
    void logProtocolFailure(const QString &code, const QString &message);

    ConfigManager *m_configManager = nullptr;
    VehicleState *m_vehicleState = nullptr;
    LogManager *m_logManager = nullptr;
    QWebSocket m_socket;
    QTimer m_heartbeatTimer;
    QTimer m_livenessTimer;
    QTimer m_reconnectTimer;
    QTimer m_modeCommandTimer;
    QElapsedTimer m_lastPongTimer;
    QElapsedTimer m_lastTelemetryTimer;
    QHash<quint32, qint64> m_pendingPings;
    QString m_connectionState = QStringLiteral("disconnected");
    QString m_lastError;
    QString m_endpoint;
    QString m_gatewayInstanceId;
    QString m_pendingModeRequestId;
    QString m_requestedMode;
    QString m_modeCommandStage = QStringLiteral("idle");
    QString m_modeCommandMessage = QStringLiteral("尚未发送模式切换命令");
    quint32 m_nextSequence = 1;
    int m_roundTripTimeMs = -1;
    bool m_socketConnected = false;
    bool m_gatewayReady = false;
    bool m_manualDisconnect = false;
    bool m_telemetryStaleLogged = false;
    bool m_modeCommandAvailable = false;
    bool m_modeCommandPending = false;
};
