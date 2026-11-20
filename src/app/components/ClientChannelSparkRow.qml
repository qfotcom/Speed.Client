import QtQuick
import QtQuick.Layouts
import SenseDesign

/**
 * 单通道 5s 窗：固定行高 + 普通 Row，避免 RowLayout 子项 parent 挂到外层 ColumnLayout 引发 recursive rearrange。
 */
Item {
    id: root

    property string channelLabel: ""
    property bool channelOn: false
    property string channelTone: "neutral"
    property bool channelActivity: false
    property color seriesColor: ShadcnTheme.c("chart-1")
    property var values: [0]
    property var categories: ["s0", "s1", "s2", "s3", "s4"]
    property var categoryAxisLabels: []
    property var categoryHoverLabels: []
    property bool showTimeAxis: false
    property bool showLaneRule: true

    readonly property int labelColWidth: 112
    readonly property int rowSpacing: 12
    /** 泳道灰框高度（不含与行间分隔线的空隙） */
    readonly property int chartHeight: 80
    /** 灰框下缘与行间分隔线之间的呼吸间距 */
    readonly property int beforeRuleGap: 8
    readonly property int ruleHeight: showLaneRule ? 1 : 0
    readonly property int rowGapBelow: showLaneRule ? 8 : 0
    readonly property int laneBlockHeight: chartHeight + beforeRuleGap + ruleHeight + rowGapBelow

    implicitHeight: laneBlockHeight
    Layout.fillWidth: true
    Layout.preferredHeight: laneBlockHeight
    Layout.maximumHeight: laneBlockHeight
    Layout.minimumHeight: laneBlockHeight

    Row {
        id: bodyRow
        x: 0
        y: 0
        width: root.width
        height: chartHeight
        spacing: rowSpacing

        Item {
            width: labelColWidth
            height: chartHeight

            Column {
                id: labelCol
                width: parent.width
                anchors.verticalCenter: parent.verticalCenter
                spacing: 6

                ShadcnLabel {
                    width: parent.width
                    text: root.channelLabel
                    font.pixelSize: ShadcnTypography.fontSizeXs
                    font.weight: ShadcnTypography.fontWeightMedium
                    color: ShadcnTheme.c("foreground")
                    elide: Text.ElideRight
                }

                ClientStatusBadge {
                    text: root.channelOn ? qsTr("开") : qsTr("关")
                    tone: root.channelTone
                    activity: root.channelActivity
                    /** 与快照区 Legacy 一致：点灭也占位，避免开/关胶囊宽度来回缩 */
                    reserveActivitySlot: true
                    showStatusDot: true
                }
            }
        }

        Item {
            id: plotHost
            width: Math.max(0, root.width - labelColWidth - rowSpacing)
            height: chartHeight
            clip: true

            Rectangle {
                anchors.fill: parent
                radius: ShadcnTheme.radiusSm
                color: ShadcnStyles.withAlpha(ShadcnTheme.c("muted"), ShadcnTheme.darkMode ? 0.25 : 0.45)
                border.color: ShadcnStyles.withAlpha(ShadcnTheme.c("border"), ShadcnTheme.darkMode ? 0.58 : 0.65)
                border.width: 1
            }

            ClientChannelStepLane {
                anchors.fill: parent
                values: root.values && root.values.length > 0 ? root.values : [0]
                seriesColor: root.seriesColor
                channelLabel: root.channelLabel
                showTimeAxis: root.showTimeAxis
                categoryAxisLabels: root.showTimeAxis ? root.categoryAxisLabels : []
                categoryHoverLabels: root.categoryHoverLabels
            }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        y: chartHeight + beforeRuleGap
        height: ruleHeight
        visible: ruleHeight > 0
        color: ShadcnTheme.c("border")
        opacity: 0.55
    }
}