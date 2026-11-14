import QtQuick
import QtQuick.Layouts
import SenseAppShell 1.0
import SenseDesign
import "../components"

Item {
    id: root
    property string pageId: "status"
    /** 分通道图 X 轴类目（与多通道图同步，避免 delegate 里每次调函数） */
    property var statusCategoriesCached: []
    property var statusHoverLabelsCached: []
    readonly property var statusSlotCategories: ["s0", "s1", "s2", "s3", "s4"]
    /** ListModel 不支持 color / 数组 role；用固定数量 Repeater + 缓存行数据 */
    property var channelSeriesCache: []

    width: parent && parent.width > 0 ? parent.width : 360
    implicitWidth: width
    implicitHeight: page.implicitHeight

  readonly property bool showPerChannelCharts: ShadcnAppPlatform.isMobile || width >= 520

    function statusSeriesForChart() {
        var raw = ClientBackend.statusChartSeries
        var out = []
        if (!raw || raw.length === 0) {
            return [{
                        label: qsTr("等待采样"),
                        values: [0],
                        color: ShadcnTheme.c("chart-1")
                    }]
        }
        for (var i = 0; i < raw.length; i++) {
            var s = raw[i]
            var vals = s.values && s.values.length > 0 ? s.values : [0]
            var token = s.color || ("chart-" + ((i % 5) + 1))
            out.push({
                         label: s.label || qsTr("通道"),
                         values: vals,
                         color: ShadcnTheme.c(token)
                     })
        }
        return out.length > 0 ? out : [{
                                             label: qsTr("等待采样"),
                                             values: [0],
                                             color: ShadcnTheme.c("chart-1")
                                         }]
    }

    function applyStatusCharts() {
        var series = statusSeriesForChart()
        statusChart.series = series
        statusChart.categoryAxisLabels = ClientBackend.statusChartAxisLabels
        statusChart.categoryHoverLabels = ClientBackend.statusChartHoverLabels
        statusCategoriesCached = ClientBackend.statusChartAxisLabels
        statusHoverLabelsCached = ClientBackend.statusChartHoverLabels
        channelSeriesCache = series
    }

    ClientMobilePage {
        id: page
        width: parent.width
        title: qsTr("客户端通道")
        subtitle: qsTr("最近 %1 秒滑动窗，每秒采样；点按曲线查看该时刻各通道。")
            .arg(ClientBackend.statusHistorySeconds)

        ShadcnCard {
            Layout.fillWidth: true
            contentMargin: ShadcnAppPlatform.isMobile ? 14 : 20

            ColumnLayout {
                width: parent.width
                spacing: 12

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    ShadcnIcon { name: "activity"; size: 18 }
                    ShadcnLabel {
                        Layout.fillWidth: true
                        text: qsTr("当前快照")
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

        ClientStatusStepChart {
            id: statusChart
            categories: root.statusSlotCategories
            categoryHoverLabels: ClientBackend.statusChartHoverLabels
            categoryAxisLabels: ClientBackend.statusChartAxisLabels
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight > 0 ? implicitHeight : 280
            title: qsTr("多通道时序")
            description: qsTr("图例为通道名称；Y 轴 0 / 100 表示关 / 开")
            footerText: qsTr("固定 %1 格：新秒在右侧进入，满窗后左侧滑出；在线多为 100 平线。")
                .arg(ClientBackend.statusHistorySeconds)
        }

        ShadcnCard {
            Layout.fillWidth: true
            visible: showPerChannelCharts
            contentMargin: ShadcnAppPlatform.isMobile ? 14 : 20

            ColumnLayout {
                width: parent.width
                spacing: 10

                ShadcnLabel {
                    Layout.fillWidth: true
                    text: qsTr("分通道 Step")
                    font.weight: ShadcnTypography.fontWeightMedium
                }

                Item {
                    Layout.fillWidth: true
                    implicitHeight: channelColumn.implicitHeight

                    Column {
                        id: channelColumn
                        width: parent.width
                        spacing: 14

                        Repeater {
                            model: root.channelSeriesCache.length

                            ClientStatusStepChart {
                                required property int index
                                readonly property var row: root.channelSeriesCache[index]
                                width: channelColumn.width
                                categories: root.statusSlotCategories
                                categoryHoverLabels: ClientBackend.statusChartHoverLabels
                                categoryAxisLabels: ClientBackend.statusChartAxisLabels
                                immediatePlot: true
                                title: row ? row.label : ""
                                seriesLabel: row ? row.label : ""
                                seriesColor: row ? row.color : ShadcnTheme.c("chart-1")
                                values: row && row.values ? row.values : [0]
                                description: ""
                                showLegend: false
                            }
                        }
                    }
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
        function onStatusChartChanged() {
            root.applyStatusCharts()
        }
    }

    Component.onCompleted: applyStatusCharts()
}