import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    property bool saveAttempted: false
    property bool saveSucceeded: false

    function connectionStateText(state) {
        const labels = {
            "disconnected": "未连接",
            "connecting": "正在连接",
            "waiting_gateway": "已连接，等待网关就绪",
            "waiting_telemetry": "网关已就绪，等待车辆遥测",
            "connected": "在线",
            "reconnecting": "等待自动重连",
            "heartbeat_timeout": "心跳超时",
            "telemetry_stale": "遥测已过期",
            "error": "连接错误"
        }
        return labels[state] || state
    }

    ColumnLayout {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: 28
        width: Math.min(parent.width - 56, 560)
        spacing: 14

        Text {
            text: "连接设置"
            color: "#f8fafc"
            font.pixelSize: 24
            font.weight: Font.DemiBold
        }
        Text {
            text: "修改后保存到本机配置。共享默认值不会被改写。"
            color: "#8fa3bf"
            font.pixelSize: 14
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        TextField {
            id: vehicleIpField
            Layout.fillWidth: true
            text: configManager.vehicleIp
            placeholderText: "车辆 IP，例如 192.168.1.10"
        }
        TextField {
            id: vehiclePortField
            Layout.fillWidth: true
            text: configManager.vehiclePort.toString()
            placeholderText: "WebSocket 端口，例如 8765"
            inputMethodHints: Qt.ImhDigitsOnly
            validator: IntValidator { bottom: 1; top: 65535 }
        }
        TextField {
            id: vehicleIdField
            Layout.fillWidth: true
            text: configManager.vehicleId
            placeholderText: "车辆编号，例如 car_01"
        }
        CheckBox {
            id: autoConnectCheckBox
            text: "启动时自动连接"
            checked: configManager.autoConnect
        }
        CheckBox {
            id: autoReconnectCheckBox
            text: "断线后自动重连"
            checked: configManager.autoReconnect
        }
        Button {
            text: "保存设置"
            onClicked: {
                configManager.vehicleIp = vehicleIpField.text
                configManager.vehiclePort = Number(vehiclePortField.text)
                configManager.vehicleId = vehicleIdField.text
                configManager.autoConnect = autoConnectCheckBox.checked
                configManager.autoReconnect = autoReconnectCheckBox.checked
                saveAttempted = true
                saveSucceeded = configManager.save()
            }
        }
        Text {
            visible: saveAttempted || configManager.errorMessage.length > 0
            text: configManager.errorMessage.length > 0
                  ? configManager.errorMessage
                  : (saveSucceeded ? "设置已保存" : "")
            color: configManager.errorMessage.length > 0 ? "#fb7185" : "#2dd4a8"
            font.pixelSize: 13
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: connectionColumn.implicitHeight + 28
            radius: 8
            color: "#111a2a"
            border.color: "#25324a"

            ColumnLayout {
                id: connectionColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins: 14
                spacing: 8

                Text {
                    text: "网关状态：" + connectionStateText(webSocketClient.connectionState)
                    color: vehicleState.connected ? "#2dd4a8" : "#cbd5e1"
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                }
                Text {
                    text: webSocketClient.endpoint.length > 0
                          ? webSocketClient.endpoint
                          : "连接地址将在发起连接时生成"
                    color: "#8fa3bf"
                    font.pixelSize: 13
                }
                Text {
                    visible: webSocketClient.roundTripTimeMs >= 0
                    text: "心跳往返：" + webSocketClient.roundTripTimeMs + " ms"
                    color: "#8fa3bf"
                    font.pixelSize: 13
                }
                Text {
                    visible: webSocketClient.lastError.length > 0
                    text: webSocketClient.lastError
                    color: "#fb7185"
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                RowLayout {
                    Button {
                        text: "连接网关"
                        enabled: !webSocketClient.socketConnected
                                 && webSocketClient.connectionState !== "connecting"
                        onClicked: webSocketClient.connectToGateway()
                    }
                    Button {
                        text: "断开连接"
                        enabled: webSocketClient.socketConnected
                                 || webSocketClient.connectionState === "connecting"
                        onClicked: webSocketClient.disconnectFromGateway()
                    }
                }
            }
        }
    }
}
