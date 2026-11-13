import QtQuick
import QtQuick.Layouts
import SenseDesign

Rectangle {
    id: root

    property string iconName: "activity"
    property string title: ""
    property string value: "—"
    property string hint: ""

    implicitWidth: 160
    implicitHeight: tileCol.implicitHeight + 24
    radius: ShadcnTheme.radiusMd
    color: ShadcnTheme.c("card")
    border.color: ShadcnTheme.c("border")
    border.width: 1

    ColumnLayout {
        id: tileCol
        anchors.fill: parent
        anchors.margins: 14
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Rectangle {
                width: 32
                height: 32
                radius: ShadcnTheme.radiusSm
                color: ShadcnStyles.withAlpha(ShadcnTheme.c("primary"), 0.12)
                ShadcnIcon {
                    anchors.centerIn: parent
                    name: root.iconName
                    size: 16
                    color: ShadcnTheme.c("primary")
                }
            }
            ShadcnLabel {
                Layout.fillWidth: true
                text: root.title
                font.pixelSize: ShadcnTypography.fontSizeXs
                font.weight: ShadcnTypography.fontWeightMedium
                color: ShadcnTheme.c("muted-foreground")
                elide: Text.ElideRight
            }
        }

        ShadcnLabel {
            Layout.fillWidth: true
            text: root.value
            font.pixelSize: ShadcnTypography.fontSize2Xl
            font.weight: ShadcnTypography.fontWeightSemibold
            color: ShadcnTheme.c("foreground")
        }

        ShadcnLabel {
            Layout.fillWidth: true
            visible: root.hint.length > 0
            text: root.hint
            wrapMode: Text.WordWrap
            font.pixelSize: ShadcnTypography.fontSizeXs
            color: ShadcnTheme.c("muted-foreground")
        }
    }
}
