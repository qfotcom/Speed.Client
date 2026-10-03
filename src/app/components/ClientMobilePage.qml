import QtQuick
import QtQuick.Layouts
import SenseAppShell 1.0
import SenseDesign

/** 移动端优先的测试页骨架（标题 + 正文卡片区） */
Item {
    id: root

    property string pageId: ""
    property string title: ""
    property string subtitle: ""

    default property alias content: bodyColumn.data

    width: parent && parent.width > 0 ? parent.width : 360
    implicitWidth: width
    implicitHeight: column.implicitHeight + 8

    ColumnLayout {
        id: column
        width: parent.width
        spacing: ShadcnAppPlatform.isMobile ? 14 : 18

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6
            visible: root.title.length > 0

            ShadcnLabel {
                Layout.fillWidth: true
                text: root.title
                font.pixelSize: ShadcnTypography.fontSize2Xl
                font.weight: ShadcnTypography.fontWeightSemibold
                wrapMode: Text.WordWrap
            }

            ShadcnLabel {
                Layout.fillWidth: true
                visible: root.subtitle.length > 0
                wrapMode: Text.WordWrap
                text: root.subtitle
                color: ShadcnTheme.c("muted-foreground")
                font.pixelSize: ShadcnTypography.fontSizeSm
                lineHeight: 1.45
            }
        }

        ColumnLayout {
            id: bodyColumn
            Layout.fillWidth: true
            spacing: ShadcnAppPlatform.isMobile ? 12 : 16
        }
    }
}
