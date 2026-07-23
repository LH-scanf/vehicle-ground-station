import QtQuick

Item {
    Rectangle {
        anchors.fill: parent
        anchors.margins: 28
        radius: 14
        color: "#111a2a"
        border.color: "#2b3b58"

        Text {
            anchors.centerIn: parent
            horizontalAlignment: Text.AlignHCenter
            text: "二维地图\n将在坐标规则与地图协议确定后实现"
            color: "#8fa3bf"
            font.pixelSize: 20
            lineHeight: 1.5
        }
    }
}
