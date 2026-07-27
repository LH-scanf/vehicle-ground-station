import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
    Dialog {
        id: modeConfirmDialog
        property string targetMode: ""
        anchors.centerIn: parent
        modal: true
        title: "确认切换车辆模式"
        standardButtons: Dialog.Ok | Dialog.Cancel
        background: Rectangle {
            color: "#172136"
            border.color: "#3b4d6b"
            radius: 10
        }

        contentItem: Text {
            text: "请求车辆切换到 " + modeConfirmDialog.targetMode.toUpperCase()
                  + " 模式？\n车辆实际模式只会在网关执行成功并上报后改变。"
            color: "#dbeafe"
            wrapMode: Text.WordWrap
            padding: 18
        }
        onAccepted: webSocketClient.requestModeChange(targetMode)
    }

    ScrollView {
        id: scrollView
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth

        Item {
            width: scrollView.availableWidth
            implicitHeight: statusGrid.implicitHeight + 56

            GridLayout {
                id: statusGrid
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 28
                columns: width > 850 ? 3 : 2
                rowSpacing: 18
                columnSpacing: 18

                StatusCard {
                    Layout.fillWidth: true
                    title: "连接状态"
                    value: vehicleState.connected ? "在线" : (webSocketClient.socketConnected ? "等待遥测" : "离线")
                    accent: vehicleState.connected ? "#2dd4a8" : "#fb7185"
                }
                StatusCard {
                    Layout.fillWidth: true
                    title: "车辆模式"
                    value: vehicleState.mode.toUpperCase()
                    accent: "#a78bfa"
                }
                StatusCard {
                    Layout.fillWidth: true
                    title: "当前速度"
                    value: vehicleState.speed.toFixed(2)
                    unit: "m/s"
                }
                StatusCard {
                    Layout.fillWidth: true
                    title: "电池电量"
                    value: vehicleState.batteryPercentage.toString()
                    unit: "%"
                    accent: vehicleState.batteryPercentage > 25 ? "#facc15" : "#fb7185"
                }
                StatusCard {
                    Layout.fillWidth: true
                    title: "定位状态"
                    value: vehicleState.gpsStatus
                    accent: "#22d3ee"
                }
                StatusCard {
                    Layout.fillWidth: true
                    title: "急停状态"
                    value: vehicleState.emergencyStopActive ? "已激活" : "正常"
                    accent: vehicleState.emergencyStopActive ? "#ef4444" : "#2dd4a8"
                }
                StatusCard {
                    Layout.fillWidth: true
                    title: "位置 X"
                    value: vehicleState.x.toFixed(2)
                    unit: "m"
                }
                StatusCard {
                    Layout.fillWidth: true
                    title: "位置 Y"
                    value: vehicleState.y.toFixed(2)
                    unit: "m"
                }
                StatusCard {
                    Layout.fillWidth: true
                    title: "车辆航向"
                    value: vehicleState.headingDegrees.toFixed(1)
                    unit: "°"
                    accent: "#38bdf8"
                }
                StatusCard {
                    Layout.fillWidth: true
                    title: "遥控链路"
                    value: vehicleState.rcLink ? "正常" : "未连接"
                    accent: vehicleState.rcLink ? "#2dd4a8" : "#fb7185"
                }
                StatusCard {
                    Layout.fillWidth: true
                    title: "通信状态"
                    value: vehicleState.communicationTimeout ? "车辆端超时" : "正常"
                    accent: vehicleState.communicationTimeout ? "#fb7185" : "#2dd4a8"
                }
                StatusCard {
                    Layout.fillWidth: true
                    title: "错误码"
                    value: vehicleState.errorCode.toString()
                    accent: vehicleState.errorCode === 0 ? "#2dd4a8" : "#fb7185"
                }
                StatusCard {
                    Layout.fillWidth: true
                    title: "最近遥测"
                    value: vehicleState.lastUpdateTimestamp > 0 ? new Date(vehicleState.lastUpdateTimestamp).toLocaleTimeString(Qt.locale(), "HH:mm:ss") : "尚未收到"
                    accent: vehicleState.connected ? "#22d3ee" : "#8fa3bf"
                }

                Rectangle {
                    Layout.columnSpan: statusGrid.columns
                    Layout.fillWidth: true
                    implicitHeight: 170
                    radius: 12
                    color: "#172136"
                    border.color: "#2b3a55"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 20
                        spacing: 12

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "车辆模式切换"
                                color: "#f8fafc"
                                font.pixelSize: 18
                                font.weight: Font.DemiBold
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: "实际模式：" + vehicleState.mode.toUpperCase()
                                color: "#c4b5fd"
                                font.pixelSize: 14
                            }
                        }

                        RowLayout {
                            spacing: 10
                            Repeater {
                                model: [
                                    { "label": "遥控 RC", "value": "rc" },
                                    { "label": "自动 AUTO", "value": "auto" },
                                    { "label": "地面站 GROUND", "value": "ground" }
                                ]
                                delegate: Button {
                                    required property var modelData
                                    text: modelData.label
                                    enabled: vehicleState.connected
                                             && webSocketClient.modeCommandAvailable
                                             && !webSocketClient.modeCommandPending
                                             && vehicleState.mode !== modelData.value
                                    onClicked: {
                                        modeConfirmDialog.targetMode = modelData.value
                                        modeConfirmDialog.open()
                                    }
                                }
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: webSocketClient.modeCommandAvailable
                                  ? webSocketClient.modeCommandMessage
                                  : "当前网关未声明 set_mode 能力，模式按钮已禁用"
                            color: webSocketClient.modeCommandStage === "failed"
                                   || webSocketClient.modeCommandStage === "rejected"
                                   || webSocketClient.modeCommandStage === "timed_out"
                                   || webSocketClient.modeCommandStage === "unknown"
                                   ? "#fb7185" : "#8fa3bf"
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }
    }
}
