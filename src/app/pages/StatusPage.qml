import QtQuick
import QtQuick.Layouts
import SenseAppShell 1.0
import SenseDesign
import "../components"

Item {
    id: root
    property string pageId: "status"

    readonly property var statusSlotCategories: ["s0", "s1", "s2", "s3", "s4"]

    /** 与 ClientBackend::rebuildStatusChartSeries 顺序一致；配色拉开，避免五条蓝线糊在一起 */
    readonly property var channelPalette: [
        ShadcnTheme.c("chart-1"),
        ShadcnTheme.c("chart-2"),
        "#c084fc",
        "#fbbf24",
        ShadcnStyles.mixWithForeground(ShadcnTheme.c("muted-foreground"), 0.35)
    ]

    property var channelRows: []

    width: parent && parent.width > 0 ? parent.width : 360
    implicitWidth: width
    implicitHeight: page.implicitHeight

    function lastSampleOn(values) {
        if (!values || values.length === 0) {
            return false
        }
        var v = Number(values[values.length - 1])
        return !isNaN(v) && v >= 50
    }

    /** 泳道开/关：与波形末格一致（每秒历史），不用即时 busy/开关叠加 */
    function channelSampleOn(values) {
        return lastSampleOn(values)
    }

    /** 后端空槽为 NaN；Graphs 阶梯线在 NaN 下常不画线，泳道统一成 0（关） */
    function sanitizePlotValues(values) {
        if (!values || values.length === 0) {
            return [0]
        }
        var out = []
        for (var i = 0; i < values.length; ++i) {
            var v = Number(values[i])
            out.push(isNaN(v) ? 0 : v)
        }
        return out
    }

    function rebuildChannelRows() {
        var raw = ClientBackend.statusChartSeries
        var axis = ClientBackend.statusChartAxisLabels
        var hover = ClientBackend.statusChartHoverLabels
        var defs = [
            { label: qsTr("Legacy TCP") },
            { label: qsTr("Legacy 传输") },
            { label: qsTr("REST /health") },
            { label: qsTr("订阅意图") },
            { label: qsTr("轮询 PING") }
        ]

        var rows = []
        for (var i = 0; i < defs.length; i++) {
            var def = defs[i]
            var entry = raw && i < raw.length ? raw[i] : null
            var vals = sanitizePlotValues(entry && entry.values ? entry.values : [0])
            var on = channelSampleOn(vals)
            var tone
            if (i === 0 && !ClientBackend.legacyConnected)
                tone = "danger"
            else
                tone = on ? "success" : "neutral"
            rows.push({
                        label: def.label,
                        values: vals,
                        color: channelPalette[i % channelPalette.length],
                        on: on,
                        tone: tone,
                        /** 泳道区不用即时脉冲；「此刻快照」区单独展示 legacyBusy */
                        activity: false
                    })
        }
        channelRows = rows
        timeAxisLabels = axis
        timeHoverLabels = hover
    }

    property var timeAxisLabels: []
    property var timeHoverLabels: []

    ClientMobilePage {
        id: page
        width: parent.width
        title: qsTr("客户端通道")
        subtitle: qsTr("最近 %1 秒滑动窗，每秒采样；点按波形查看该秒各通道。")
            .arg(ClientBackend.statusHistorySeconds)

        ShadcnCard {
            Layout.fillWidth: true
            contentMargin: ShadcnAppPlatform.isMobile ? 14 : 20

            ColumnLayout {
                width: parent.width
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    ShadcnIcon { name: "activity"; size: 18 }
                    ShadcnLabel {
                        Layout.fillWidth: true
                        text: qsTr("此刻快照")
                        font.weight: ShadcnTypography.fontWeightSemibold
                    }
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 8
                    ClientStatusBadge {
                        text: ClientBackend.legacyConnected ? qsTr("Legacy 在线") : qsTr("Legacy 离线")
                        tone: ClientBackend.legacyConnected ? "success" : "danger"
                        reserveActivitySlot: ClientBackend.legacyConnected
                        activity: ClientBackend.legacyConnected && ClientBackend.legacyBusy
                    }
                    ClientStatusBadge {
                        text: ClientBackend.legacySubscriptionActive ? qsTr("已订阅") : qsTr("未订阅")
                        tone: ClientBackend.legacySubscriptionActive ? "success" : "neutral"
                    }
                    ClientStatusBadge {
                        text: ClientBackend.pushPollEnabled ? qsTr("PING 轮询开") : qsTr("PING 轮询关")
                        tone: ClientBackend.pushPollEnabled ? "pending" : "neutral"
                    }
                }

            }
        }

        ShadcnCard {
            Layout.fillWidth: true
            contentMargin: ShadcnAppPlatform.isMobile ? 14 : 20

            ColumnLayout {
                width: parent.width
                spacing: 10

                ShadcnLabel {
                    Layout.fillWidth: true
                    text: qsTr("通道泳道")
                    font.weight: ShadcnTypography.fontWeightMedium
                    font.pixelSize: ShadcnTypography.fontSizeSm
                }

                Repeater {
                    id: laneRepeater
                    model: root.channelRows.length

                    ClientChannelSparkRow {
                        required property int index
                        readonly property var row: root.channelRows[index]
                        Layout.fillWidth: true

                        channelLabel: row ? row.label : ""
                        channelOn: row ? row.on : false
                        channelTone: row ? row.tone : "neutral"
                        channelActivity: row ? row.activity : false
                        seriesColor: row ? row.color : ShadcnTheme.c("chart-1")
                        values: row ? row.values : [0]
                        categories: root.statusSlotCategories
                        categoryHoverLabels: root.timeHoverLabels
                        categoryAxisLabels: root.timeAxisLabels
                        showTimeAxis: index === laneRepeater.count - 1
                        showLaneRule: index < laneRepeater.count - 1
                    }
                }

                ShadcnLabel {
                    Layout.fillWidth: true
                    font.pixelSize: ShadcnTypography.fontSizeXs
                    color: ShadcnTheme.c("muted-foreground")
                    wrapMode: Text.WordWrap
                    text: qsTr("固定 %1 格 · 新秒自右进入；「清空」后五行归零，随后按秒重采样。")
                        .arg(ClientBackend.statusHistorySeconds)
                }
            }
        }

        ShadcnButton {
            Layout.fillWidth: true
            variant: "outline"
            iconName: "eraser"
            text: qsTr("清空波形历史")
            onClicked: ClientBackend.clearStatusHistory()
        }
    }

    Connections {
        target: ClientBackend
        function onStatusChartChanged() { root.rebuildChannelRows() }
        function onLegacyConnectedChanged() { root.rebuildChannelRows() }
        function onLegacySubscriptionActiveChanged() { root.rebuildChannelRows() }
        function onPushPollEnabledChanged() { root.rebuildChannelRows() }
    }

    Component.onCompleted: rebuildChannelRows()
}