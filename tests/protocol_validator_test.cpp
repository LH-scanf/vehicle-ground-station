#include "communication/ProtocolValidator.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtTest>

namespace {

QJsonObject envelope(const QString &type, const QString &name, const QJsonObject &data)
{
    return {{QStringLiteral("version"), 1},
            {QStringLiteral("type"), type},
            {QStringLiteral("name"), name},
            {QStringLiteral("vehicle_id"), QStringLiteral("car_01")},
            {QStringLiteral("seq"), 1},
            {QStringLiteral("timestamp"), 1784651000000.0},
            {QStringLiteral("data"), data}};
}

ProtocolValidationResult parse(const QJsonObject &object)
{
    return ProtocolValidator::parseIncoming(
        QJsonDocument(object).toJson(QJsonDocument::Compact), QStringLiteral("car_01"));
}

QJsonObject validTelemetryData()
{
    return {{QStringLiteral("x"), 2.35},
            {QStringLiteral("y"), 1.48},
            {QStringLiteral("yaw"), 0.52},
            {QStringLiteral("speed"), 0.8},
            {QStringLiteral("mode"), QStringLiteral("auto")},
            {QStringLiteral("battery_pct"), 82},
            {QStringLiteral("gps_fix"), 4},
            {QStringLiteral("rc_link"), true},
            {QStringLiteral("estop_active"), false},
            {QStringLiteral("comm_timeout"), false},
            {QStringLiteral("error_code"), 0}};
}

} // namespace

class ProtocolValidatorTest final : public QObject
{
    Q_OBJECT

private slots:
    void parsesGatewayReady();
    void buildsHeartbeatPing();
    void buildsSetModeCommand();
    void validatesMatchingPongShape();
    void validatesVehicleStatus();
    void rejectsInvalidOptionalVehicleStatusFields();
    void rejectsMissingTelemetryField();
    void rejectsInvalidMode();
    void rejectsVehicleMismatch();
    void rejectsUnsupportedVersion();
    void rejectsAckWithoutRequestId();
    void validatesSetModeAckStages();
    void rejectsInvalidSetModeAckStage();
    void rejectsOversizedMessage();
};

void ProtocolValidatorTest::parsesGatewayReady()
{
    const QJsonObject data = {
        {QStringLiteral("gateway_version"), QStringLiteral("0.1.0")},
        {QStringLiteral("gateway_instance_id"), QStringLiteral("gw_test")},
        {QStringLiteral("protocol_version"), 1},
        {QStringLiteral("auto_disconnect_policy"), QStringLiteral("cancel_task_and_stop")},
        {QStringLiteral("capabilities"), QJsonArray{}}};
    const ProtocolValidationResult result = parse(
        envelope(QStringLiteral("event"), QStringLiteral("gateway_ready"), data));
    QVERIFY(result.valid);

    QString code;
    QString message;
    QVERIFY(ProtocolValidator::validateGatewayReady(result.message, code, message));
}

void ProtocolValidatorTest::buildsHeartbeatPing()
{
    const QJsonObject ping = ProtocolValidator::makeHeartbeatPing(
        QStringLiteral("car_01"), 42, 1784651000000LL);
    QCOMPARE(ping.value(QStringLiteral("type")).toString(), QStringLiteral("heartbeat"));
    QCOMPARE(ping.value(QStringLiteral("name")).toString(), QStringLiteral("ping"));
    QCOMPARE(ping.value(QStringLiteral("seq")).toInteger(), 42);
    QVERIFY(ping.value(QStringLiteral("data")).toObject().isEmpty());
}

void ProtocolValidatorTest::buildsSetModeCommand()
{
    const QJsonObject command = ProtocolValidator::makeSetModeCommand(
        QStringLiteral("car_01"),
        43,
        1784651000001LL,
        QStringLiteral("cmd_test"),
        QStringLiteral("ground"));
    QCOMPARE(command.value(QStringLiteral("type")).toString(), QStringLiteral("command"));
    QCOMPARE(command.value(QStringLiteral("name")).toString(), QStringLiteral("set_mode"));
    QCOMPARE(command.value(QStringLiteral("request_id")).toString(), QStringLiteral("cmd_test"));
    QCOMPARE(command.value(QStringLiteral("data")).toObject().value(QStringLiteral("mode")).toString(),
             QStringLiteral("ground"));
}

void ProtocolValidatorTest::validatesMatchingPongShape()
{
    const QJsonObject data = {{QStringLiteral("ping_seq"), 42},
                              {QStringLiteral("ping_timestamp"), 1784651000000.0}};
    const ProtocolValidationResult result = parse(
        envelope(QStringLiteral("heartbeat"), QStringLiteral("pong"), data));
    QVERIFY(result.valid);

    QString code;
    QString message;
    QVERIFY(ProtocolValidator::validatePong(result.message, code, message));
}

void ProtocolValidatorTest::validatesVehicleStatus()
{
    const ProtocolValidationResult result = parse(envelope(
        QStringLiteral("telemetry"), QStringLiteral("vehicle_status"), validTelemetryData()));
    QVERIFY(result.valid);

    QString code;
    QString message;
    QVERIFY(ProtocolValidator::validateVehicleStatus(result.message, code, message));
}

void ProtocolValidatorTest::rejectsInvalidOptionalVehicleStatusFields()
{
    for (const auto &[field, value] : {
             std::pair{QStringLiteral("steer_norm"), QJsonValue(1.1)},
             std::pair{QStringLiteral("signal_rssi"), QJsonValue(-42.5)},
             std::pair{QStringLiteral("wp_idx"), QJsonValue(-1)},
             std::pair{QStringLiteral("mission_id"), QJsonValue(123)}}) {
        QJsonObject data = validTelemetryData();
        data.insert(field, value);
        const ProtocolValidationResult result = parse(
            envelope(QStringLiteral("telemetry"), QStringLiteral("vehicle_status"), data));
        QVERIFY(result.valid);

        QString code;
        QString message;
        QVERIFY2(!ProtocolValidator::validateVehicleStatus(result.message, code, message),
                 qPrintable(field));
    }
}

void ProtocolValidatorTest::rejectsMissingTelemetryField()
{
    QJsonObject data = validTelemetryData();
    data.remove(QStringLiteral("yaw"));
    const ProtocolValidationResult result = parse(
        envelope(QStringLiteral("telemetry"), QStringLiteral("vehicle_status"), data));
    QVERIFY(result.valid);

    QString code;
    QString message;
    QVERIFY(!ProtocolValidator::validateVehicleStatus(result.message, code, message));
    QCOMPARE(code, QStringLiteral("missing_field"));
}

void ProtocolValidatorTest::rejectsInvalidMode()
{
    QJsonObject data = validTelemetryData();
    data.insert(QStringLiteral("mode"), QStringLiteral("invalid"));
    const ProtocolValidationResult result = parse(
        envelope(QStringLiteral("telemetry"), QStringLiteral("vehicle_status"), data));
    QVERIFY(result.valid);

    QString code;
    QString message;
    QVERIFY(!ProtocolValidator::validateVehicleStatus(result.message, code, message));
    QCOMPARE(code, QStringLiteral("invalid_field_value"));
}

void ProtocolValidatorTest::rejectsVehicleMismatch()
{
    QJsonObject object = envelope(
        QStringLiteral("telemetry"), QStringLiteral("vehicle_status"), validTelemetryData());
    object.insert(QStringLiteral("vehicle_id"), QStringLiteral("other_car"));
    const ProtocolValidationResult result = parse(object);
    QVERIFY(!result.valid);
    QCOMPARE(result.errorCode, QStringLiteral("vehicle_id_mismatch"));
}

void ProtocolValidatorTest::rejectsUnsupportedVersion()
{
    QJsonObject object = envelope(
        QStringLiteral("telemetry"), QStringLiteral("vehicle_status"), validTelemetryData());
    object.insert(QStringLiteral("version"), 2);
    const ProtocolValidationResult result = parse(object);
    QVERIFY(!result.valid);
    QCOMPARE(result.errorCode, QStringLiteral("unsupported_version"));
}

void ProtocolValidatorTest::rejectsAckWithoutRequestId()
{
    const ProtocolValidationResult result = parse(envelope(
        QStringLiteral("ack"),
        QStringLiteral("set_mode"),
        {{QStringLiteral("stage"), QStringLiteral("accepted")},
         {QStringLiteral("code"), QStringLiteral("ok")},
         {QStringLiteral("message"), QStringLiteral("accepted")}}));
    QVERIFY(!result.valid);
    QCOMPARE(result.errorCode, QStringLiteral("missing_field"));
}

void ProtocolValidatorTest::validatesSetModeAckStages()
{
    for (const QString &stage : {QStringLiteral("accepted"),
                                 QStringLiteral("rejected"),
                                 QStringLiteral("completed"),
                                 QStringLiteral("failed")}) {
        QJsonObject object = envelope(
            QStringLiteral("ack"),
            QStringLiteral("set_mode"),
            {{QStringLiteral("stage"), stage},
             {QStringLiteral("code"), QStringLiteral("ok")},
             {QStringLiteral("message"), QStringLiteral("test result")}});
        object.insert(QStringLiteral("request_id"), QStringLiteral("cmd_test"));
        const ProtocolValidationResult result = parse(object);
        QVERIFY(result.valid);

        QString parsedStage;
        QString code;
        QString ackMessage;
        QString errorCode;
        QString errorMessage;
        QVERIFY(ProtocolValidator::validateCommandAck(result.message,
                                                      parsedStage,
                                                      code,
                                                      ackMessage,
                                                      errorCode,
                                                      errorMessage));
        QCOMPARE(parsedStage, stage);
    }
}

void ProtocolValidatorTest::rejectsInvalidSetModeAckStage()
{
    QJsonObject object = envelope(
        QStringLiteral("ack"),
        QStringLiteral("set_mode"),
        {{QStringLiteral("stage"), QStringLiteral("done")},
         {QStringLiteral("code"), QStringLiteral("ok")},
         {QStringLiteral("message"), QStringLiteral("test result")}});
    object.insert(QStringLiteral("request_id"), QStringLiteral("cmd_test"));
    const ProtocolValidationResult result = parse(object);
    QVERIFY(result.valid);

    QString stage;
    QString code;
    QString ackMessage;
    QString errorCode;
    QString errorMessage;
    QVERIFY(!ProtocolValidator::validateCommandAck(
        result.message, stage, code, ackMessage, errorCode, errorMessage));
    QCOMPARE(errorCode, QStringLiteral("invalid_field_value"));
}

void ProtocolValidatorTest::rejectsOversizedMessage()
{
    const QByteArray payload(ProtocolValidator::MaximumTextMessageBytes + 1, 'x');
    const ProtocolValidationResult result = ProtocolValidator::parseIncoming(
        payload, QStringLiteral("car_01"));
    QVERIFY(!result.valid);
    QCOMPARE(result.errorCode, QStringLiteral("message_too_large"));
}

QTEST_APPLESS_MAIN(ProtocolValidatorTest)

#include "protocol_validator_test.moc"
