import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
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
            text: "当前为界面占位，保存和连接功能将在配置任务中实现。"
            color: "#8fa3bf"
            font.pixelSize: 14
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        TextField {
            Layout.fillWidth: true
            placeholderText: "车辆 IP，例如 192.168.1.10"
            enabled: false
        }
        TextField {
            Layout.fillWidth: true
            placeholderText: "WebSocket 端口，例如 8765"
            enabled: false
        }
        Button {
            text: "保存设置"
            enabled: false
        }
    }
}
