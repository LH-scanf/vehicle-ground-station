import QtQuick

Rectangle {
    id: root
    required property string title
    required property string value
    property string unit: ""
    property color accent: "#38bdf8"

    implicitWidth: 210
    implicitHeight: 126
    radius: 12
    color: "#172033"
    border.color: "#26344f"

    Rectangle {
        width: 4
        height: 42
        radius: 2
        color: root.accent
        anchors.left: parent.left
        anchors.leftMargin: 18
        anchors.verticalCenter: parent.verticalCenter
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 36
        anchors.top: parent.top
        anchors.topMargin: 22
        text: root.title
        color: "#8fa3bf"
        font.pixelSize: 13
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 36
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 22
        text: root.value + (root.unit.length > 0 ? " " + root.unit : "")
        color: "#f8fafc"
        font.pixelSize: 25
        font.weight: Font.DemiBold
    }
}
