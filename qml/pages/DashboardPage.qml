import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"

Item {
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
            }
        }
    }
}
