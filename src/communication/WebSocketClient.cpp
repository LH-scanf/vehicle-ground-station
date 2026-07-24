#include "WebSocketClient.h"

#include "ProtocolValidator.h"
#include "config/ConfigManager.h"
#include "log/LogManager.h"
#include "vehicle/VehicleState.h"

#include <QAbstractSocket>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkProxy>
#include <QUrl>
#include <QWebSocketProtocol>

#include <limits>

namespace {

QString gpsStatusName(int fix)
{
    switch (fix) {
    case 0:
        return QStringLiteral("无有效定位");
    case 1:
        return QStringLiteral("单点定位");
    case 2:
        return QStringLiteral("差分定位");
    case 4:
        return QStringLiteral("RTK Fixed");
    case 5:
        return QStringLiteral("RTK Float");
    default:
        return QStringLiteral("未知(%1)").arg(fix);
    }
}

} // namespace

WebSocketClient::WebSocketClient(ConfigManager *configManager,
                                 VehicleState *vehicleState,
                                 LogManager *logManager,
                                 QObject *parent)
    : QObject(parent)
    , m_configManager(configManager)
    , m_vehicleState(vehicleState)
    , m_logManager(logManager)
{
    Q_ASSERT(m_configManager);
    Q_ASSERT(m_vehicleState);
    Q_ASSERT(m_logManager);

    // The vehicle gateway is reached directly on the local network. Explicitly bypass
    // system/application proxies so PAC, VPN, or stale proxy settings cannot intercept
    // or reject the ws:// connection before the WebSocket handshake.
    m_socket.setProxy(QNetworkProxy::NoProxy);
    m_socket.setMaxAllowedIncomingFrameSize(ProtocolValidator::MaximumTextMessageBytes);
    m_socket.setMaxAllowedIncomingMessageSize(ProtocolValidator::MaximumTextMessageBytes);

    m_heartbeatTimer.setInterval(1000);
    m_livenessTimer.setInterval(250);
    m_reconnectTimer.setInterval(3000);
    m_reconnectTimer.setSingleShot(true);

    connect(&m_socket, &QWebSocket::connected, this, &WebSocketClient::handleConnected);
    connect(&m_socket, &QWebSocket::disconnected, this, &WebSocketClient::handleDisconnected);
    connect(&m_socket, &QWebSocket::textMessageReceived, this, &WebSocketClient::handleTextMessage);
    connect(&m_socket, &QWebSocket::binaryMessageReceived, this, &WebSocketClient::handleBinaryMessage);
    connect(&m_socket, &QWebSocket::errorOccurred, this, &WebSocketClient::handleSocketError);
    connect(&m_heartbeatTimer, &QTimer::timeout, this, &WebSocketClient::sendHeartbeat);
    connect(&m_livenessTimer, &QTimer::timeout, this, &WebSocketClient::checkLiveness);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &WebSocketClient::connectToGateway);
}

QString WebSocketClient::connectionState() const { return m_connectionState; }
bool WebSocketClient::socketConnected() const { return m_socketConnected; }
bool WebSocketClient::gatewayReady() const { return m_gatewayReady; }
int WebSocketClient::roundTripTimeMs() const { return m_roundTripTimeMs; }
QString WebSocketClient::lastError() const { return m_lastError; }
QString WebSocketClient::endpoint() const { return m_endpoint; }

void WebSocketClient::connectToGateway()
{
    if (m_socket.state() == QAbstractSocket::ConnectedState
        || m_socket.state() == QAbstractSocket::ConnectingState) {
        return;
    }

    m_manualDisconnect = false;
    m_reconnectTimer.stop();
    resetSessionState();
    const QString url = QStringLiteral("ws://%1:%2")
                            .arg(m_configManager->vehicleIp())
                            .arg(m_configManager->vehiclePort());
    setEndpoint(url);
    setConnectionState(QStringLiteral("connecting"));
    setLastError({});

    m_logManager->addEntry(LogManager::Level::Info,
                           LogManager::Category::Communication,
                           LogManager::Display::Primary,
                           QStringLiteral("websocket_connecting"),
                           QStringLiteral("WebSocketClient"),
                           QStringLiteral("正在连接车辆网关 %1").arg(url),
                           m_configManager->vehicleId(),
                           {},
                           {{QStringLiteral("endpoint"), url}});
    m_socket.open(QUrl(url));
}

void WebSocketClient::disconnectFromGateway()
{
    m_manualDisconnect = true;
    m_reconnectTimer.stop();
    if (m_socket.state() == QAbstractSocket::UnconnectedState) {
        resetSessionState();
        setConnectionState(QStringLiteral("disconnected"));
        return;
    }
    m_socket.close(QWebSocketProtocol::CloseCodeNormal, QStringLiteral("operator_disconnect"));
}

void WebSocketClient::handleConnected()
{
    setSocketConnected(true);
    setConnectionState(QStringLiteral("waiting_gateway"));
    m_lastPongTimer.start();
    m_lastTelemetryTimer.invalidate();
    m_heartbeatTimer.start();
    m_livenessTimer.start();
    sendHeartbeat();

    m_logManager->addEntry(LogManager::Level::Info,
                           LogManager::Category::Communication,
                           LogManager::Display::Primary,
                           QStringLiteral("websocket_connected"),
                           QStringLiteral("WebSocketClient"),
                           QStringLiteral("WebSocket连接已建立，等待网关就绪"),
                           m_configManager->vehicleId());
}

void WebSocketClient::handleDisconnected()
{
    const bool wasManual = m_manualDisconnect;
    resetSessionState();
    setConnectionState(QStringLiteral("disconnected"));
    m_logManager->addEntry(wasManual ? LogManager::Level::Info : LogManager::Level::Warning,
                           LogManager::Category::Communication,
                           LogManager::Display::Primary,
                           QStringLiteral("websocket_disconnected"),
                           QStringLiteral("WebSocketClient"),
                           wasManual ? QStringLiteral("已断开车辆网关") : QStringLiteral("车辆网关连接意外断开"),
                           m_configManager->vehicleId());
    if (!wasManual)
        scheduleReconnect();
}

void WebSocketClient::handleTextMessage(const QString &text)
{
    const QByteArray utf8 = text.toUtf8();
    const ProtocolValidationResult result = ProtocolValidator::parseIncoming(
        utf8, m_configManager->vehicleId());
    if (!result.valid) {
        logProtocolFailure(result.errorCode, result.errorMessage);
        if (result.errorCode == QStringLiteral("message_too_large"))
            m_socket.close(QWebSocketProtocol::CloseCodeTooMuchData, result.errorMessage);
        return;
    }

    const ProtocolMessage &message = result.message;
    if (message.type == QStringLiteral("event") && message.name == QStringLiteral("gateway_ready")) {
        handleGatewayReady(message);
    } else if (message.type == QStringLiteral("heartbeat") && message.name == QStringLiteral("pong")) {
        handlePong(message);
    } else if (message.type == QStringLiteral("telemetry")
               && message.name == QStringLiteral("vehicle_status")) {
        handleVehicleStatus(message);
    } else {
        m_logManager->addEntry(LogManager::Level::Debug,
                               LogManager::Category::Communication,
                               LogManager::Display::Diagnostic,
                               QStringLiteral("protocol_message_ignored"),
                               QStringLiteral("WebSocketClient"),
                               QStringLiteral("当前通信切片暂不处理消息 %1/%2")
                                   .arg(message.type, message.name),
                               m_configManager->vehicleId(),
                               message.requestId,
                               {{QStringLiteral("type"), message.type},
                                {QStringLiteral("name"), message.name},
                                {QStringLiteral("seq"), static_cast<qint64>(message.seq)}});
    }
}

void WebSocketClient::handleSocketError()
{
    setLastError(m_socket.errorString());
    setConnectionState(QStringLiteral("error"));
    m_logManager->addEntry(LogManager::Level::Warning,
                           LogManager::Category::Communication,
                           LogManager::Display::Primary,
                           QStringLiteral("websocket_error"),
                           QStringLiteral("WebSocketClient"),
                           QStringLiteral("WebSocket错误：%1").arg(m_socket.errorString()),
                           m_configManager->vehicleId());

    // Some connection failures don't produce a separate disconnected signal on every
    // Qt/platform combination. Defer the state check so auto-reconnect still starts.
    QTimer::singleShot(0, this, [this] {
        if (m_socket.state() == QAbstractSocket::UnconnectedState)
            scheduleReconnect();
    });
}

void WebSocketClient::handleBinaryMessage(const QByteArray &message)
{
    Q_UNUSED(message)
    logProtocolFailure(QStringLiteral("invalid_field_type"),
                       QStringLiteral("V1协议不接受WebSocket二进制消息"));
    m_socket.close(QWebSocketProtocol::CloseCodeDatatypeNotSupported,
                   QStringLiteral("binary_messages_not_supported"));
}

void WebSocketClient::sendHeartbeat()
{
    if (m_socket.state() != QAbstractSocket::ConnectedState)
        return;

    const quint32 seq = takeNextSequence();
    const qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    const QJsonObject ping = ProtocolValidator::makeHeartbeatPing(
        m_configManager->vehicleId(), seq, timestamp);
    m_pendingPings.insert(seq, timestamp);
    while (m_pendingPings.size() > 8)
        m_pendingPings.erase(m_pendingPings.begin());
    m_socket.sendTextMessage(QString::fromUtf8(QJsonDocument(ping).toJson(QJsonDocument::Compact)));
}

void WebSocketClient::checkLiveness()
{
    if (!m_socketConnected)
        return;

    if (m_lastPongTimer.isValid() && m_lastPongTimer.elapsed() > 3000) {
        setConnectionState(QStringLiteral("heartbeat_timeout"));
        setLastError(QStringLiteral("心跳响应超过3秒"));
        m_vehicleState->setCommunicationTimeout(true);
        m_logManager->addEntry(LogManager::Level::Error,
                               LogManager::Category::Communication,
                               LogManager::Display::Primary,
                               QStringLiteral("heartbeat_timeout"),
                               QStringLiteral("WebSocketClient"),
                               QStringLiteral("车辆网关心跳超时，关闭连接并等待重连"),
                               m_configManager->vehicleId());
        m_socket.close(QWebSocketProtocol::CloseCodeGoingAway, QStringLiteral("heartbeat_timeout"));
        return;
    }

    if (m_gatewayReady && m_lastTelemetryTimer.isValid() && m_lastTelemetryTimer.elapsed() > 3000) {
        m_vehicleState->setConnected(false);
        setConnectionState(QStringLiteral("telemetry_stale"));
        if (!m_telemetryStaleLogged) {
            m_telemetryStaleLogged = true;
            m_logManager->addEntry(LogManager::Level::Warning,
                                   LogManager::Category::Communication,
                                   LogManager::Display::Primary,
                                   QStringLiteral("telemetry_stale"),
                                   QStringLiteral("WebSocketClient"),
                                   QStringLiteral("车辆遥测超过3秒未更新"),
                                   m_configManager->vehicleId());
        }
    }
}

void WebSocketClient::handleGatewayReady(const ProtocolMessage &message)
{
    QString errorCode;
    QString errorMessage;
    if (!ProtocolValidator::validateGatewayReady(message, errorCode, errorMessage)) {
        logProtocolFailure(errorCode, errorMessage);
        return;
    }

    const QString newInstanceId = message.data.value(QStringLiteral("gateway_instance_id")).toString();
    if (!m_gatewayInstanceId.isEmpty() && m_gatewayInstanceId != newInstanceId) {
        m_logManager->addEntry(LogManager::Level::Warning,
                               LogManager::Category::Communication,
                               LogManager::Display::Primary,
                               QStringLiteral("gateway_instance_changed"),
                               QStringLiteral("WebSocketClient"),
                               QStringLiteral("检测到车辆网关实例已变化"),
                               m_configManager->vehicleId());
    }
    m_gatewayInstanceId = newInstanceId;
    setGatewayReady(true);
    setConnectionState(QStringLiteral("waiting_telemetry"));

    m_logManager->addEntry(LogManager::Level::Info,
                           LogManager::Category::Communication,
                           LogManager::Display::Primary,
                           QStringLiteral("gateway_ready"),
                           QStringLiteral("WebSocketClient"),
                           QStringLiteral("车辆网关已就绪，等待新鲜遥测"),
                           m_configManager->vehicleId(),
                           {},
                           {{QStringLiteral("gateway_version"),
                             message.data.value(QStringLiteral("gateway_version")).toString()},
                            {QStringLiteral("gateway_instance_id"), newInstanceId}});
}

void WebSocketClient::handlePong(const ProtocolMessage &message)
{
    QString errorCode;
    QString errorMessage;
    if (!ProtocolValidator::validatePong(message, errorCode, errorMessage)) {
        logProtocolFailure(errorCode, errorMessage);
        return;
    }

    const quint32 pingSeq = static_cast<quint32>(
        message.data.value(QStringLiteral("ping_seq")).toDouble());
    if (!m_pendingPings.contains(pingSeq)) {
        logProtocolFailure(QStringLiteral("invalid_field_value"),
                           QStringLiteral("pong无法匹配当前已发送的ping"));
        return;
    }

    const qint64 sentAt = m_pendingPings.take(pingSeq);
    setRoundTripTimeMs(static_cast<int>(qMax<qint64>(0, QDateTime::currentMSecsSinceEpoch() - sentAt)));
    m_lastPongTimer.restart();
    m_vehicleState->setCommunicationTimeout(false);
}

void WebSocketClient::handleVehicleStatus(const ProtocolMessage &message)
{
    if (!m_gatewayReady) {
        logProtocolFailure(QStringLiteral("invalid_field_value"),
                           QStringLiteral("gateway_ready之前收到vehicle_status"));
        return;
    }

    QString errorCode;
    QString errorMessage;
    if (!ProtocolValidator::validateVehicleStatus(message, errorCode, errorMessage)) {
        logProtocolFailure(errorCode, errorMessage);
        return;
    }

    const QJsonObject &data = message.data;
    m_vehicleState->setX(data.value(QStringLiteral("x")).toDouble());
    m_vehicleState->setY(data.value(QStringLiteral("y")).toDouble());
    m_vehicleState->setYaw(data.value(QStringLiteral("yaw")).toDouble());
    m_vehicleState->setSpeed(data.value(QStringLiteral("speed")).toDouble());
    m_vehicleState->setMode(data.value(QStringLiteral("mode")).toString());
    m_vehicleState->setBatteryPercentage(data.value(QStringLiteral("battery_pct")).toInt());
    m_vehicleState->setGpsStatus(gpsStatusName(data.value(QStringLiteral("gps_fix")).toInt()));
    m_vehicleState->setRcLink(data.value(QStringLiteral("rc_link")).toBool());
    m_vehicleState->setEmergencyStopActive(data.value(QStringLiteral("estop_active")).toBool());
    m_vehicleState->setCommunicationTimeout(data.value(QStringLiteral("comm_timeout")).toBool());
    m_vehicleState->setErrorCode(data.value(QStringLiteral("error_code")).toInt());
    // Use local receipt time for freshness display so clock skew between Windows and
    // the vehicle computer cannot make a fresh telemetry sample look old or future-dated.
    m_vehicleState->setLastUpdateTimestamp(QDateTime::currentMSecsSinceEpoch());
    m_vehicleState->setConnected(true);
    m_lastTelemetryTimer.restart();
    m_telemetryStaleLogged = false;
    setConnectionState(QStringLiteral("connected"));
}

void WebSocketClient::scheduleReconnect()
{
    if (!m_configManager->autoReconnect() || m_manualDisconnect || m_reconnectTimer.isActive())
        return;
    setConnectionState(QStringLiteral("reconnecting"));
    m_reconnectTimer.start();
    m_logManager->addEntry(LogManager::Level::Info,
                           LogManager::Category::Communication,
                           LogManager::Display::Diagnostic,
                           QStringLiteral("websocket_reconnect_scheduled"),
                           QStringLiteral("WebSocketClient"),
                           QStringLiteral("将在3秒后尝试重新连接"),
                           m_configManager->vehicleId());
}

void WebSocketClient::resetSessionState()
{
    m_heartbeatTimer.stop();
    m_livenessTimer.stop();
    m_pendingPings.clear();
    m_lastPongTimer.invalidate();
    m_lastTelemetryTimer.invalidate();
    m_nextSequence = 1;
    m_telemetryStaleLogged = false;
    setSocketConnected(false);
    setGatewayReady(false);
    setRoundTripTimeMs(-1);
    m_vehicleState->setConnected(false);
}

void WebSocketClient::setConnectionState(const QString &state)
{
    if (m_connectionState == state)
        return;
    m_connectionState = state;
    emit connectionStateChanged();
}

void WebSocketClient::setSocketConnected(bool connected)
{
    if (m_socketConnected == connected)
        return;
    m_socketConnected = connected;
    emit socketConnectedChanged();
}

void WebSocketClient::setGatewayReady(bool ready)
{
    if (m_gatewayReady == ready)
        return;
    m_gatewayReady = ready;
    emit gatewayReadyChanged();
}

void WebSocketClient::setRoundTripTimeMs(int milliseconds)
{
    if (m_roundTripTimeMs == milliseconds)
        return;
    m_roundTripTimeMs = milliseconds;
    emit roundTripTimeMsChanged();
}

void WebSocketClient::setLastError(const QString &message)
{
    if (m_lastError == message)
        return;
    m_lastError = message;
    emit lastErrorChanged();
}

void WebSocketClient::setEndpoint(const QString &endpoint)
{
    if (m_endpoint == endpoint)
        return;
    m_endpoint = endpoint;
    emit endpointChanged();
}

quint32 WebSocketClient::takeNextSequence()
{
    const quint32 current = m_nextSequence;
    m_nextSequence = current == std::numeric_limits<quint32>::max() ? 1 : current + 1;
    return current;
}

void WebSocketClient::logProtocolFailure(const QString &code, const QString &message)
{
    setLastError(message);
    m_logManager->addEntry(LogManager::Level::Warning,
                           LogManager::Category::Communication,
                           LogManager::Display::Diagnostic,
                           QStringLiteral("protocol_validation_failed"),
                           QStringLiteral("ProtocolValidator"),
                           message,
                           m_configManager->vehicleId(),
                           {},
                           {{QStringLiteral("code"), code}});
}
