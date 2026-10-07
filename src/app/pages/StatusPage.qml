import QtQuick
import QtQuick.Layouts
import SenseAppShell 1.0
import SenseDesign
import "../components"

Item {
    id: root
    property string pageId: "status"
    property int chartRevision: 0

    width: parent && parent.width > 0 ? parent.width : 360
    implicitWidth: width
    implicitHeight: page.implicitHeight

    readonly property bool showPerChannelCharts: !ShadcnAppPlatform.isMobile && width >= 520

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

    function refreshCharts() {
        chartRevision++
        statusChart.series = statusSeriesForChart()
        statusChart.categories = ClientBackend.statusChartCategories
    }

    ClientMobilePage {
        id: page
        width: parent.width
        title: qsTr("通道状态")
        subtitle: qsTr("Step 阶梯线展示各通道开关量，点按曲线可看时间与状态。")

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
            Layout.fillWidth: true
            Layout.preferredHeight: implicitHeight > 0 ? implicitHeight : 280
            title: qsTr("多通道时序")
            description: qsTr("图例为通道名称；低/高为关/开")
            categories: ClientBackend.statusChartCategories
            series: statusSeriesForChart()
            footerText: qsTr("Legacy 断线重连为阶跃；传输线为短时脉冲。")
            yAxisLabelsVisible: false
            showLegend: true

            Connections {
                target: ClientBackend
                function onStatusChartChanged() {
                    root.refreshCharts()
                }
            }
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
                            model: root.chartRevision >= 0 ? statusSeriesForChart() : []

                            ClientStatusStepChart {
                                required property var modelData
                                width: channelColumn.width
                                title: modelData.label
                                seriesLabel: modelData.label
                                seriesColor: modelData.color
                                values: modelData.values
                                categories: ClientBackend.statusChartCategories
                                description: ""
                                showLegend: false
                                yAxisLabelsVisible: false
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

    Component.onCompleted: refreshCharts()
}
