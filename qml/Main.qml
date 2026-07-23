import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import VehicleGroundStation
import "components"
import "pages"

ApplicationWindow {
    id: window
    width: 1280
    height: 800
    minimumWidth: 960
    minimumHeight: 640
    visible: true
    title: "Vehicle Ground Station"
    color: "#0b1220"

    property int currentPage: 0
    readonly property var navigationItems: [
        { "label": "状态总览", "glyph": "◫" },
        { "label": "地图任务", "glyph": "⌖" },
        { "label": "日志诊断", "glyph": "≡" },
        { "label": "系统设置", "glyph": "⚙" }
    ]

    header: Rectangle {
        height: 68
        color: "#111a2a"
        border.color: "#25324a"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 24
            anchors.rightMargin: 24
            spacing: 18

            Text {
                text: "VEHICLE GROUND STATION"
                color: "#f8fafc"
                font.pixelSize: 17
                font.weight: Font.Bold
                font.letterSpacing: 1.2
            }
            Item { Layout.fillWidth: true }
            ConnectionIndicator { connected: vehicleState.connected }
            Rectangle {
                implicitWidth: modeLabel.implicitWidth + 24
                implicitHeight: 30
                radius: 7
                color: "#292343"
                Text {
                    id: modeLabel
                    anchors.centerIn: parent
                    text: "模式  " + vehicleState.mode.toUpperCase()
                    color: "#c4b5fd"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }
            }
            Rectangle {
                implicitWidth: estopLabel.implicitWidth + 26
                implicitHeight: 36
                radius: 8
                color: vehicleState.emergencyStopActive ? "#991b1b" : "#3b1720"
                border.color: "#ef4444"
                Text {
                    id: estopLabel
                    anchors.centerIn: parent
                    text: vehicleState.emergencyStopActive ? "急停已激活" : "急停"
                    color: "#fecaca"
                    font.pixelSize: 13
                    font.weight: Font.Bold
                }
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.preferredWidth: 210
            Layout.fillHeight: true
            color: "#0f1727"
            border.color: "#25324a"

            Column {
                anchors.fill: parent
                anchors.topMargin: 22
                spacing: 8

                Repeater {
                    model: window.navigationItems
                    delegate: Rectangle {
                        required property var modelData
                        required property int index
                        width: 210
                        height: 52
                        color: window.currentPage === index ? "#1d3049" : "transparent"

                        Rectangle {
                            visible: window.currentPage === index
                            width: 3
                            height: parent.height
                            color: "#38bdf8"
                        }
                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 24
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.glyph + "    " + modelData.label
                            color: window.currentPage === index ? "#f8fafc" : "#8fa3bf"
                            font.pixelSize: 15
                            font.weight: window.currentPage === index ? Font.DemiBold : Font.Normal
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: window.currentPage = index
                        }
                    }
                }
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: window.currentPage

            DashboardPage {}
            MapPage {}
            DiagnosticsPage {}
            SettingsPage {}
        }
    }
}
