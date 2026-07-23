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
            text: "诊断与日志\n通信模块完成后接入实时数据"
            color: "#8fa3bf"
            font.pixelSize: 20
            lineHeight: 1.5
        }
    }
}
