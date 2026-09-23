import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SenseDesign 1.0

SenseCard {
    id: root

    property alias model: logView.model

    Layout.fillWidth: true
    Layout.preferredHeight: Math.min(280, Math.max(160, logView.contentHeight + 48))

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: SenseSpacing.spacer4
        spacing: SenseSpacing.spacer3

        RowLayout {
            Layout.fillWidth: true
            SenseText {
                text: "事件日志"
                size: "lg"
                weight: "semibold"
                Layout.fillWidth: true
            }
            SenseButton {
                text: "清空"
                variant: "ghost"
                size: "sm"
                onClicked: ClientBackend.clearLog()
            }
        }

        ListView {
            id: logView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: SenseSpacing.spacer2
            delegate: SenseText {
                width: logView.width
                text: modelData
                size: "xs"
                autoWrap: true
                textColor: SenseTheme.currentTheme.textSecondary
            }
        }
    }
}
