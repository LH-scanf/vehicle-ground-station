import QtQuick
import QtQuick.Layouts
import "../components"

Item {
    GridLayout {
        anchors.fill: parent
        anchors.margins: 28
        columns: width > 850 ? 3 : 2
        rowSpacing: 18
        columnSpacing: 18

        StatusCard {
            Layout.fillWidth: true
            title: "连接状态"
            value: vehicleState.connected
                   ? "在线"
                   : (webSocketClient.socketConnected ? "等待遥测" : "离线")
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

        Item { Layout.fillHeight: true }
    }
}
