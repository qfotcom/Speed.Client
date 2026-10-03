import QtQuick
import QtQuick.Layouts
import SenseDesign

/** 与 ClientStatusBadge 同套语义：success 绿、danger 红 */
Rectangle {
    id: root

    property string tone: "neutral"
    property string title: ""
    property string description: ""

    implicitWidth: 400
    implicitHeight: row.implicitHeight + 24
    radius: ShadcnTheme.radiusMd
    border.width: 1

    readonly property color _successFg: ShadcnTheme.darkMode ? "#4ade80" : "#15803d"

    readonly property var _style: {
        switch (root.tone) {
        case "success":
            return {
                bg: ShadcnStyles.withAlpha(_successFg, ShadcnTheme.darkMode ? 0.12 : 0.08),
                border: ShadcnStyles.withAlpha(_successFg, 0.45),
                fg: _successFg,
                icon: "circle-check"
            }
        case "danger":
            return {
                bg: ShadcnTheme.c("card"),
                border: ShadcnTheme.c("destructive"),
                fg: ShadcnTheme.c("destructive"),
                icon: "triangle-alert"
            }
        default:
            return {
                bg: ShadcnTheme.c("card"),
                border: ShadcnTheme.c("border"),
                fg: ShadcnTheme.c("foreground"),
                icon: "circle-alert"
            }
        }
    }

    color: _style.bg
    border.color: _style.border

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        ShadcnIcon {
            name: root._style.icon
            size: 16
            color: root._style.fg
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: 2
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            ShadcnLabel {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: root.title
                font.weight: ShadcnTypography.fontWeightMedium
                color: root.tone === "danger" ? root._style.fg : ShadcnTheme.c("foreground")
            }

            ShadcnLabel {
                Layout.fillWidth: true
                visible: root.description.length > 0
                wrapMode: Text.WordWrap
                text: root.description
                font.pixelSize: ShadcnTypography.fontSizeSm
                color: ShadcnTheme.c("muted-foreground")
            }
        }
    }
}
