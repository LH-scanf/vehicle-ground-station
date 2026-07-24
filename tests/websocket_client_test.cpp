#include "communication/WebSocketClient.h"
#include "config/ConfigManager.h"
#include "log/LogManager.h"
#include "vehicle/VehicleState.h"

#include <QDateTime>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScopedPointer>
#include <QTemporaryDir>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QtTest>

namespace {

QJsonObject serverEnvelope(const QString &type,
                           const QString &name,
                           quint32 seq,
                           const QJsonObject &data)
{
    return {{QStringLiteral("version"), 1},
            {QStringLiteral("type"), type},
            {QStringLiteral("name"), name},
            {QStringLiteral("vehicle_id"), QStringLiteral("car_01")},
            {QStringLiteral("seq"), static_cast<qint64>(seq)},
            {QStringLiteral("timestamp"), QDateTime::currentMSecsSinceEpoch()},
            {QStringLiteral("data"), data}};
}

void sendObject(QWebSocket *socket, const QJsonObject &object)
{
    socket->sendTextMessage(QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact)));
}

} // namespace

class WebSocketClientTest final : public QObject
{
    Q_OBJECT

private slots:
    void receivesReadyHeartbeatAndTelemetry();
};

void WebSocketClientTest::receivesReadyHeartbeatAndTelemetry()
{
    QWebSocketServer server(QStringLiteral("gateway-test"), QWebSocketServer::NonSecureMode);
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    QScopedPointer<QWebSocket> peer;
    quint32 serverSequence = 1;
    connect(&server, &QWebSocketServer::newConnection, this, [&] {
        peer.reset(server.nextPendingConnection());
        QVERIFY(peer);

        sendObject(peer.data(),
                   serverEnvelope(
                       QStringLiteral("event"),
                       QStringLiteral("gateway_ready"),
                       serverSequence++,
                       {{QStringLiteral("gateway_version"), QStringLiteral("0.1.0")},
                        {QStringLiteral("gateway_instance_id"), QStringLiteral("gw_test")},
                        {QStringLiteral("protocol_version"), 1},
                        {QStringLiteral("auto_disconnect_policy"), QStringLiteral("cancel_task_and_stop")},
                        {QStringLiteral("capabilities"), QJsonArray{}}}));

        connect(peer.data(), &QWebSocket::textMessageReceived, this, [&](const QString &text) {
            const QJsonObject ping = QJsonDocument::fromJson(text.toUtf8()).object();
            if (ping.value(QStringLiteral("type")).toString() != QStringLiteral("heartbeat")
                || ping.value(QStringLiteral("name")).toString() != QStringLiteral("ping")) {
                return;
            }
            sendObject(peer.data(),
                       serverEnvelope(
                           QStringLiteral("heartbeat"),
                           QStringLiteral("pong"),
                           serverSequence++,
                           {{QStringLiteral("ping_seq"), ping.value(QStringLiteral("seq"))},
                            {QStringLiteral("ping_timestamp"), ping.value(QStringLiteral("timestamp"))}}));
        });

        sendObject(peer.data(),
                   serverEnvelope(
                       QStringLiteral("telemetry"),
                       QStringLiteral("vehicle_status"),
                       serverSequence++,
                       {{QStringLiteral("x"), 2.35},
                        {QStringLiteral("y"), 1.48},
                        {QStringLiteral("yaw"), 0.52},
                        {QStringLiteral("speed"), 0.8},
                        {QStringLiteral("mode"), QStringLiteral("auto")},
                        {QStringLiteral("battery_pct"), 82},
                        {QStringLiteral("gps_fix"), 4},
                        {QStringLiteral("rc_link"), true},
                        {QStringLiteral("estop_active"), false},
                        {QStringLiteral("comm_timeout"), false},
                        {QStringLiteral("error_code"), 0}}));
    });

    ConfigManager configManager;
    configManager.setVehicleIp(QStringLiteral("127.0.0.1"));
    configManager.setVehiclePort(server.serverPort());
    configManager.setVehicleId(QStringLiteral("car_01"));
    configManager.setAutoReconnect(false);

    QTemporaryDir logDirectory;
    QVERIFY(logDirectory.isValid());
    LogManager logManager;
    QVERIFY(logManager.initialize(logDirectory.path(), 1, QStringLiteral("debug"), 100));
    VehicleState vehicleState;
    WebSocketClient client(&configManager, &vehicleState, &logManager);

    client.connectToGateway();
    QTRY_VERIFY_WITH_TIMEOUT(client.gatewayReady(), 2000);
    QTRY_VERIFY_WITH_TIMEOUT(vehicleState.connected(), 2000);
    QTRY_VERIFY_WITH_TIMEOUT(client.roundTripTimeMs() >= 0, 2000);
    QCOMPARE(client.connectionState(), QStringLiteral("connected"));
    QCOMPARE(vehicleState.mode(), QStringLiteral("auto"));
    QCOMPARE(vehicleState.batteryPercentage(), 82);
    QCOMPARE(vehicleState.gpsStatus(), QStringLiteral("RTK Fixed"));
    QVERIFY(qAbs(vehicleState.x() - 2.35) < 0.0001);
    QVERIFY(qAbs(vehicleState.yaw() - 0.52) < 0.0001);

    client.disconnectFromGateway();
    QTRY_VERIFY_WITH_TIMEOUT(!client.socketConnected(), 2000);
}

QTEST_GUILESS_MAIN(WebSocketClientTest)

#include "websocket_client_test.moc"
