#include "ProtocolValidator.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonValue>
#include <QSet>

#include <cmath>
#include <limits>

namespace {

ProtocolValidationResult failure(const QString &code, const QString &message)
{
    ProtocolValidationResult result;
    result.errorCode = code;
    result.errorMessage = message;
    return result;
}

bool requireString(const QJsonObject &object,
                   const QString &field,
                   QString &value,
                   QString &errorCode,
                   QString &errorMessage)
{
    if (!object.contains(field)) {
        errorCode = QStringLiteral("missing_field");
        errorMessage = QStringLiteral("缺少字段 data.%1").arg(field);
        return false;
    }
    if (!object.value(field).isString() || object.value(field).toString().isEmpty()) {
        errorCode = QStringLiteral("invalid_field_type");
        errorMessage = QStringLiteral("字段 data.%1 必须是非空字符串").arg(field);
        return false;
    }
    value = object.value(field).toString();
    return true;
}

bool requireFiniteNumber(const QJsonObject &object,
                         const QString &field,
                         double &value,
                         QString &errorCode,
                         QString &errorMessage)
{
    if (!object.contains(field)) {
        errorCode = QStringLiteral("missing_field");
        errorMessage = QStringLiteral("缺少字段 data.%1").arg(field);
        return false;
    }
    if (!object.value(field).isDouble()) {
        errorCode = QStringLiteral("invalid_field_type");
        errorMessage = QStringLiteral("字段 data.%1 必须是数字").arg(field);
        return false;
    }
    value = object.value(field).toDouble();
    if (!std::isfinite(value)) {
        errorCode = QStringLiteral("invalid_field_value");
        errorMessage = QStringLiteral("字段 data.%1 必须是有限数字").arg(field);
        return false;
    }
    return true;
}

bool requireInteger(const QJsonObject &object,
                    const QString &field,
                    qint64 minimum,
                    qint64 maximum,
                    qint64 &value,
                    QString &errorCode,
                    QString &errorMessage)
{
    double number = 0.0;
    if (!requireFiniteNumber(object, field, number, errorCode, errorMessage))
        return false;
    if (std::floor(number) != number || number < static_cast<double>(minimum)
        || number > static_cast<double>(maximum)) {
        errorCode = QStringLiteral("invalid_field_value");
        errorMessage = QStringLiteral("字段 data.%1 必须是范围内的整数").arg(field);
        return false;
    }
    value = static_cast<qint64>(number);
    return true;
}

bool requireBoolean(const QJsonObject &object,
                    const QString &field,
                    QString &errorCode,
                    QString &errorMessage)
{
    if (!object.contains(field)) {
        errorCode = QStringLiteral("missing_field");
        errorMessage = QStringLiteral("缺少字段 data.%1").arg(field);
        return false;
    }
    if (!object.value(field).isBool()) {
        errorCode = QStringLiteral("invalid_field_type");
        errorMessage = QStringLiteral("字段 data.%1 必须是布尔值").arg(field);
        return false;
    }
    return true;
}

bool validateOptionalNumber(const QJsonObject &object,
                            const QString &field,
                            QString &errorCode,
                            QString &errorMessage)
{
    if (!object.contains(field))
        return true;
    double value = 0.0;
    return requireFiniteNumber(object, field, value, errorCode, errorMessage);
}

bool validateOptionalString(const QJsonObject &object,
                            const QString &field,
                            QString &errorCode,
                            QString &errorMessage)
{
    if (!object.contains(field))
        return true;
    QString value;
    return requireString(object, field, value, errorCode, errorMessage);
}

bool validateOptionalInteger(const QJsonObject &object,
                             const QString &field,
                             qint64 minimum,
                             qint64 maximum,
                             QString &errorCode,
                             QString &errorMessage)
{
    if (!object.contains(field))
        return true;
    qint64 value = 0;
    return requireInteger(object, field, minimum, maximum, value, errorCode, errorMessage);
}

} // namespace

ProtocolValidationResult ProtocolValidator::parseIncoming(const QByteArray &utf8Payload,
                                                          const QString &expectedVehicleId)
{
    if (utf8Payload.size() > MaximumTextMessageBytes)
        return failure(QStringLiteral("message_too_large"), QStringLiteral("消息超过256 KiB限制"));

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(utf8Payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject())
        return failure(QStringLiteral("invalid_json"), QStringLiteral("消息不是合法JSON对象"));

    const QJsonObject root = document.object();
    const auto missing = [&root](const QString &field) { return !root.contains(field); };
    for (const QString &field : {QStringLiteral("version"),
                                 QStringLiteral("type"),
                                 QStringLiteral("name"),
                                 QStringLiteral("vehicle_id"),
                                 QStringLiteral("seq"),
                                 QStringLiteral("timestamp"),
                                 QStringLiteral("data")}) {
        if (missing(field))
            return failure(QStringLiteral("missing_field"), QStringLiteral("缺少公共字段 %1").arg(field));
    }

    const QJsonValue version = root.value(QStringLiteral("version"));
    if (!version.isDouble() || version.toDouble() != 1.0)
        return failure(QStringLiteral("unsupported_version"), QStringLiteral("仅支持协议版本1"));

    for (const QString &field : {QStringLiteral("type"),
                                 QStringLiteral("name"),
                                 QStringLiteral("vehicle_id")}) {
        if (!root.value(field).isString() || root.value(field).toString().isEmpty())
            return failure(QStringLiteral("invalid_field_type"), QStringLiteral("字段 %1 必须是非空字符串").arg(field));
    }

    const QString vehicleId = root.value(QStringLiteral("vehicle_id")).toString();
    if (vehicleId != expectedVehicleId)
        return failure(QStringLiteral("vehicle_id_mismatch"), QStringLiteral("车辆编号与当前配置不一致"));

    const QJsonValue seqValue = root.value(QStringLiteral("seq"));
    const double seqNumber = seqValue.toDouble(-1.0);
    if (!seqValue.isDouble() || std::floor(seqNumber) != seqNumber || seqNumber < 1.0
        || seqNumber > static_cast<double>(std::numeric_limits<quint32>::max())) {
        return failure(QStringLiteral("invalid_field_value"), QStringLiteral("字段 seq 必须是有效的无符号32位序号"));
    }

    const QJsonValue timestampValue = root.value(QStringLiteral("timestamp"));
    const double timestampNumber = timestampValue.toDouble(-1.0);
    if (!timestampValue.isDouble() || std::floor(timestampNumber) != timestampNumber || timestampNumber < 0.0
        || timestampNumber > static_cast<double>(std::numeric_limits<qint64>::max())) {
        return failure(QStringLiteral("invalid_field_value"), QStringLiteral("字段 timestamp 必须是非负整数毫秒时间戳"));
    }

    if (!root.value(QStringLiteral("data")).isObject())
        return failure(QStringLiteral("invalid_field_type"), QStringLiteral("字段 data 必须是对象"));

    static const QSet<QString> allowedTypes = {
        QStringLiteral("heartbeat"), QStringLiteral("telemetry"), QStringLiteral("ack"),
        QStringLiteral("event"),     QStringLiteral("alarm"),     QStringLiteral("error")};
    const QString type = root.value(QStringLiteral("type")).toString();
    if (!allowedTypes.contains(type))
        return failure(QStringLiteral("unknown_message_type"), QStringLiteral("未知消息类型 %1").arg(type));

    if (type == QStringLiteral("ack")) {
        if (!root.value(QStringLiteral("request_id")).isString()
            || root.value(QStringLiteral("request_id")).toString().isEmpty()) {
            return failure(QStringLiteral("missing_field"), QStringLiteral("ack缺少有效request_id"));
        }
        if (root.value(QStringLiteral("request_id")).toString().size() > 64)
            return failure(QStringLiteral("invalid_field_value"), QStringLiteral("ack的request_id超过64字符"));
    }

    ProtocolValidationResult result;
    result.valid = true;
    result.message.root = root;
    result.message.data = root.value(QStringLiteral("data")).toObject();
    result.message.type = type;
    result.message.name = root.value(QStringLiteral("name")).toString();
    result.message.vehicleId = vehicleId;
    result.message.requestId = root.value(QStringLiteral("request_id")).toString();
    result.message.seq = static_cast<quint32>(seqNumber);
    result.message.timestamp = static_cast<qint64>(timestampNumber);
    return result;
}

bool ProtocolValidator::validateGatewayReady(const ProtocolMessage &message,
                                             QString &errorCode,
                                             QString &errorMessage)
{
    if (message.type != QStringLiteral("event") || message.name != QStringLiteral("gateway_ready")) {
        errorCode = QStringLiteral("unknown_message_name");
        errorMessage = QStringLiteral("消息不是gateway_ready事件");
        return false;
    }

    QString ignored;
    if (!requireString(message.data, QStringLiteral("gateway_version"), ignored, errorCode, errorMessage)
        || !requireString(message.data, QStringLiteral("gateway_instance_id"), ignored, errorCode, errorMessage)) {
        return false;
    }

    qint64 protocolVersion = 0;
    if (!requireInteger(message.data,
                        QStringLiteral("protocol_version"),
                        1,
                        1,
                        protocolVersion,
                        errorCode,
                        errorMessage)) {
        return false;
    }

    QString policy;
    if (!requireString(message.data,
                       QStringLiteral("auto_disconnect_policy"),
                       policy,
                       errorCode,
                       errorMessage)) {
        return false;
    }
    if (policy != QStringLiteral("cancel_task_and_stop")) {
        errorCode = QStringLiteral("invalid_field_value");
        errorMessage = QStringLiteral("不支持网关声明的auto断联策略");
        return false;
    }

    const QJsonValue capabilities = message.data.value(QStringLiteral("capabilities"));
    if (!capabilities.isArray()) {
        errorCode = message.data.contains(QStringLiteral("capabilities"))
                        ? QStringLiteral("invalid_field_type")
                        : QStringLiteral("missing_field");
        errorMessage = QStringLiteral("gateway_ready.data.capabilities必须是数组");
        return false;
    }
    for (const QJsonValue &capability : capabilities.toArray()) {
        if (!capability.isString() || capability.toString().isEmpty()) {
            errorCode = QStringLiteral("invalid_field_type");
            errorMessage = QStringLiteral("capabilities成员必须是非空字符串");
            return false;
        }
    }
    return true;
}

bool ProtocolValidator::validatePong(const ProtocolMessage &message,
                                     QString &errorCode,
                                     QString &errorMessage)
{
    if (message.type != QStringLiteral("heartbeat") || message.name != QStringLiteral("pong")) {
        errorCode = QStringLiteral("unknown_message_name");
        errorMessage = QStringLiteral("消息不是pong心跳");
        return false;
    }
    qint64 ignored = 0;
    return requireInteger(message.data,
                          QStringLiteral("ping_seq"),
                          1,
                          std::numeric_limits<quint32>::max(),
                          ignored,
                          errorCode,
                          errorMessage)
           && requireInteger(message.data,
                             QStringLiteral("ping_timestamp"),
                             0,
                             std::numeric_limits<qint64>::max(),
                             ignored,
                             errorCode,
                             errorMessage);
}

bool ProtocolValidator::validateVehicleStatus(const ProtocolMessage &message,
                                              QString &errorCode,
                                              QString &errorMessage)
{
    if (message.type != QStringLiteral("telemetry") || message.name != QStringLiteral("vehicle_status")) {
        errorCode = QStringLiteral("unknown_message_name");
        errorMessage = QStringLiteral("消息不是vehicle_status遥测");
        return false;
    }

    double ignoredNumber = 0.0;
    for (const QString &field : {QStringLiteral("x"),
                                 QStringLiteral("y"),
                                 QStringLiteral("yaw"),
                                 QStringLiteral("speed")}) {
        if (!requireFiniteNumber(message.data, field, ignoredNumber, errorCode, errorMessage))
            return false;
    }

    QString mode;
    if (!requireString(message.data, QStringLiteral("mode"), mode, errorCode, errorMessage))
        return false;
    if (mode != QStringLiteral("rc") && mode != QStringLiteral("auto") && mode != QStringLiteral("ground")) {
        errorCode = QStringLiteral("invalid_field_value");
        errorMessage = QStringLiteral("data.mode不是受支持的模式");
        return false;
    }

    qint64 ignoredInteger = 0;
    if (!requireInteger(message.data,
                        QStringLiteral("battery_pct"),
                        0,
                        100,
                        ignoredInteger,
                        errorCode,
                        errorMessage)
        || !requireInteger(message.data,
                           QStringLiteral("gps_fix"),
                           0,
                           std::numeric_limits<int>::max(),
                           ignoredInteger,
                           errorCode,
                           errorMessage)
        || !requireInteger(message.data,
                           QStringLiteral("error_code"),
                           0,
                           std::numeric_limits<int>::max(),
                           ignoredInteger,
                           errorCode,
                           errorMessage)) {
        return false;
    }

    for (const QString &field : {QStringLiteral("rc_link"),
                                 QStringLiteral("estop_active"),
                                 QStringLiteral("comm_timeout")}) {
        if (!requireBoolean(message.data, field, errorCode, errorMessage))
            return false;
    }

    for (const QString &field : {QStringLiteral("latitude"),
                                 QStringLiteral("longitude"),
                                 QStringLiteral("battery_v"),
                                 QStringLiteral("wp_dist")}) {
        if (!validateOptionalNumber(message.data, field, errorCode, errorMessage))
            return false;
    }

    for (const QString &field : {QStringLiteral("steer_norm"),
                                 QStringLiteral("throttle_norm")}) {
        if (!message.data.contains(field))
            continue;
        double value = 0.0;
        if (!requireFiniteNumber(message.data, field, value, errorCode, errorMessage))
            return false;
        if (value < -1.0 || value > 1.0) {
            errorCode = QStringLiteral("invalid_field_value");
            errorMessage = QStringLiteral("字段 data.%1 必须位于 [-1, 1]").arg(field);
            return false;
        }
    }

    if (!validateOptionalInteger(message.data,
                                 QStringLiteral("signal_rssi"),
                                 std::numeric_limits<int>::min(),
                                 std::numeric_limits<int>::max(),
                                 errorCode,
                                 errorMessage)
        || !validateOptionalInteger(message.data,
                                    QStringLiteral("wp_idx"),
                                    0,
                                    std::numeric_limits<int>::max(),
                                    errorCode,
                                    errorMessage)
        || !validateOptionalInteger(message.data,
                                    QStringLiteral("wp_total"),
                                    0,
                                    std::numeric_limits<int>::max(),
                                    errorCode,
                                    errorMessage)
        || !validateOptionalString(message.data,
                                   QStringLiteral("mission_id"),
                                   errorCode,
                                   errorMessage)
        || !validateOptionalString(message.data,
                                   QStringLiteral("mission_status"),
                                   errorCode,
                                   errorMessage)) {
        return false;
    }
    return true;
}

bool ProtocolValidator::validateCommandAck(const ProtocolMessage &message,
                                           QString &stage,
                                           QString &code,
                                           QString &ackMessage,
                                           QString &errorCode,
                                           QString &errorMessage)
{
    if (message.type != QStringLiteral("ack") || message.name != QStringLiteral("set_mode")) {
        errorCode = QStringLiteral("unknown_message_name");
        errorMessage = QStringLiteral("消息不是set_mode命令应答");
        return false;
    }
    if (!requireString(message.data, QStringLiteral("stage"), stage, errorCode, errorMessage)
        || !requireString(message.data, QStringLiteral("code"), code, errorCode, errorMessage)
        || !requireString(message.data,
                          QStringLiteral("message"),
                          ackMessage,
                          errorCode,
                          errorMessage)) {
        return false;
    }

    static const QSet<QString> stages = {QStringLiteral("accepted"),
                                         QStringLiteral("rejected"),
                                         QStringLiteral("completed"),
                                         QStringLiteral("failed")};
    if (!stages.contains(stage)) {
        errorCode = QStringLiteral("invalid_field_value");
        errorMessage = QStringLiteral("data.stage不是受支持的命令阶段");
        return false;
    }
    return true;
}

QJsonObject ProtocolValidator::makeHeartbeatPing(const QString &vehicleId,
                                                 quint32 seq,
                                                 qint64 timestamp)
{
    return {{QStringLiteral("version"), 1},
            {QStringLiteral("type"), QStringLiteral("heartbeat")},
            {QStringLiteral("name"), QStringLiteral("ping")},
            {QStringLiteral("vehicle_id"), vehicleId},
            {QStringLiteral("seq"), static_cast<qint64>(seq)},
            {QStringLiteral("timestamp"), timestamp},
            {QStringLiteral("data"), QJsonObject{}}};
}

QJsonObject ProtocolValidator::makeSetModeCommand(const QString &vehicleId,
                                                  quint32 seq,
                                                  qint64 timestamp,
                                                  const QString &requestId,
                                                  const QString &mode)
{
    return {{QStringLiteral("version"), 1},
            {QStringLiteral("type"), QStringLiteral("command")},
            {QStringLiteral("name"), QStringLiteral("set_mode")},
            {QStringLiteral("vehicle_id"), vehicleId},
            {QStringLiteral("seq"), static_cast<qint64>(seq)},
            {QStringLiteral("request_id"), requestId},
            {QStringLiteral("timestamp"), timestamp},
            {QStringLiteral("data"), QJsonObject{{QStringLiteral("mode"), mode}}}};
}
