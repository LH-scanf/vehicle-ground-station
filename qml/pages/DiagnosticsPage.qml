import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    readonly property var levelValues: ["", "debug", "info", "warning", "error", "critical"]
    readonly property var categoryValues: ["", "system", "configuration", "communication", "operation", "command", "state", "alarm", "error"]

    function levelLabel(value) {
        const labels = { "debug": "调试", "info": "信息", "warning": "警告", "error": "错误", "critical": "严重" }
        return labels[value] || value
    }

    function categoryLabel(value) {
        const labels = {
            "system": "系统", "configuration": "配置", "communication": "通信",
            "operation": "操作", "command": "命令", "state": "状态",
            "alarm": "告警", "error": "错误"
        }
        return labels[value] || value
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 28
        spacing: 14

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Text {
                text: "运行记录 / 诊断"
                color: "#f8fafc"
                font.pixelSize: 23
                font.weight: Font.DemiBold
            }
            Item { Layout.fillWidth: true }
            Text {
                text: "当前显示 " + logList.count + " 条"
                color: "#8fa3bf"
                font.pixelSize: 13
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            ComboBox {
                Layout.preferredWidth: 130
                model: ["全部等级", "调试", "信息", "警告", "错误", "严重"]
                onCurrentIndexChanged: {
                    if (currentIndex >= 0)
                        logFilterModel.levelFilter = root.levelValues[currentIndex]
                }
            }
            ComboBox {
                Layout.preferredWidth: 150
                model: ["全部类型", "系统", "配置", "通信", "操作", "命令", "状态", "告警", "错误"]
                onCurrentIndexChanged: {
                    if (currentIndex >= 0)
                        logFilterModel.categoryFilter = root.categoryValues[currentIndex]
                }
            }
            TextField {
                Layout.fillWidth: true
                placeholderText: "搜索消息、事件、模块、车辆或命令编号"
                onTextChanged: logFilterModel.searchText = text
            }
            CheckBox {
                text: "显示技术详情"
                onCheckedChanged: logFilterModel.showTechnical = checked
            }
            Button {
                text: "打开目录"
                enabled: logManager.logDirectory.length > 0
                onClicked: Qt.openUrlExternally(logManager.logDirectoryUrl)
            }
            Button {
                text: "清屏"
                onClicked: logManager.clearDisplay()
            }
        }

        Rectangle {
            visible: logManager.writerError.length > 0
            Layout.fillWidth: true
            implicitHeight: writerErrorText.implicitHeight + 20
            radius: 8
            color: "#3b1720"
            border.color: "#ef4444"

            Text {
                id: writerErrorText
                anchors.fill: parent
                anchors.margins: 10
                text: "日志写入异常：" + logManager.writerError
                color: "#fecaca"
                wrapMode: Text.WordWrap
                font.pixelSize: 13
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 12
            color: "#111a2a"
            border.color: "#2b3b58"
            clip: true

            ListView {
                id: logList
                anchors.fill: parent
                anchors.margins: 1
                clip: true
                model: logFilterModel

                delegate: Rectangle {
                    required property int index
                    required property string timestamp
                    required property string logLevel
                    required property string logCategory
                    required property string eventName
                    required property string component
                    required property string logMessage
                    required property string vehicleId
                    required property string requestId

                    width: logList.width
                    height: 66
                    color: index % 2 === 0 ? "#111a2a" : "#141f31"

                    Rectangle {
                        width: 4
                        height: parent.height
                        color: logLevel === "critical" ? "#ef4444"
                              : logLevel === "error" ? "#fb7185"
                              : logLevel === "warning" ? "#facc15"
                              : "#38bdf8"
                    }

                    Column {
                        anchors.left: parent.left
                        anchors.leftMargin: 16
                        anchors.right: parent.right
                        anchors.rightMargin: 14
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 5

                        Row {
                            spacing: 12
                            Text {
                                width: 92
                                text: timestamp.length >= 23 ? timestamp.substring(11, 23) : timestamp
                                color: "#7890ad"
                                font.family: "Consolas"
                                font.pixelSize: 12
                            }
                            Text {
                                width: 72
                                text: root.levelLabel(logLevel)
                                color: logLevel === "critical" || logLevel === "error" ? "#fb7185"
                                       : logLevel === "warning" ? "#facc15" : "#7dd3fc"
                                font.pixelSize: 12
                                font.weight: Font.Bold
                            }
                            Text {
                                width: 110
                                text: root.categoryLabel(logCategory)
                                color: "#a78bfa"
                                font.pixelSize: 12
                            }
                            Text {
                                width: Math.max(0, parent.parent.width - 310)
                                text: logMessage
                                color: "#e2e8f0"
                                elide: Text.ElideRight
                                font.pixelSize: 14
                            }
                        }
                        Text {
                            width: parent.width
                            text: (logFilterModel.showTechnical ? component + " · " + eventName : "")
                                  + (vehicleId.length > 0 ? (logFilterModel.showTechnical ? " · " : "") + "车辆 " + vehicleId : "")
                                  + (requestId.length > 0 ? " · 命令 " + requestId : "")
                            color: "#64748b"
                            elide: Text.ElideRight
                            font.pixelSize: 11
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar {}
            }

            Text {
                anchors.centerIn: parent
                visible: logList.count === 0
                text: "当前筛选条件下没有运行记录"
                color: "#64748b"
                font.pixelSize: 15
            }
        }
    }
}
