import QtQuick

Rectangle {
    id: root
    required property bool connected

    implicitWidth: statusText.implicitWidth + 30
    implicitHeight: 30
    radius: 15
    color: connected ? "#153b32" : "#43242a"
    border.color: connected ? "#2dd4a8" : "#fb7185"

    Text {
        id: statusText
        anchors.centerIn: parent
        text: root.connected ? "●  在线" : "●  离线"
        color: root.connected ? "#76e4c4" : "#fda4af"
        font.pixelSize: 13
        font.weight: Font.DemiBold
    }
}
