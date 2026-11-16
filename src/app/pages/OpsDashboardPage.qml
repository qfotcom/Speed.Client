import QtQuick
import QtQuick.Layouts
import SenseAppShell 1.0
import SenseDesign
import "../components"

Item {
    id: root
    property string pageId: "dashboard"

    /** all | requests | events — 与 ShadcnActivityHeatmap.sourceFilter 同步 */
    property string metricFilter: "all"

    readonly property var trendChartBindKey: ({
                                                 days: ClientBackend.opsHeatmapDays,
                                                 filter: metricFilter
                                             })

    readonly property var trendCategories: {
        var _k = trendChartBindKey
        return trendCategoriesForChart()
    }
    readonly property var trendSeries: {
        var _k = trendChartBindKey
        return trendSeriesForChart()
    }

    readonly property var heatmapFilterSegments: [
        { value: "all", label: qsTr("全部") },
        { value: "requests", label: qsTr("累计请求") },
        { value: "events", label: qsTr("事件发布") }
    ]

    readonly property string heatmapCaption:
        metricFilter === "requests" ? qsTr("请求活跃度 · 全年日增量")
        : metricFilter === "events" ? qsTr("事件活跃度 · 全年日增量")
        : qsTr("请求 + 事件活跃度 · 全年日增量")

    readonly property bool mobileHeatmap: ShadcnAppPlatform.isMobile

    /** 近 7 日趋势：仅用 chart-* + 描边/混色，与 SenseDesign 折线图一致 */
    readonly property color trendRequestsColor: ShadcnChartTheme.cartesianSeriesStrokeColor(
                ShadcnTheme.c("chart-1"))
    readonly property color trendEventsColor: ShadcnChartTheme.cartesianSeriesStrokeColor(
                ShadcnStyles.mixWithForeground(ShadcnTheme.c("chart-2"),
                                               ShadcnTheme.darkModeVisual ? 0.55 : 0.68))
    readonly property color trendTotalColor: ShadcnChartTheme.cartesianSeriesStrokeColor(
                ShadcnTheme.c("chart-3"))

    function opsHeatmapTooltip(dayData, formatLongDate) {
        var c = dayData && dayData.count !== undefined ? Number(dayData.count) : 0
        var line
        if (metricFilter === "requests")
            line = c === 0 ? qsTr("无请求日增量") : qsTr("请求日增量 %1").arg(c)
        else if (metricFilter === "events")
            line = c === 0 ? qsTr("无事件日增量") : qsTr("事件日增量 %1").arg(c)
        else
            line = c === 0 ? qsTr("无日增量") : qsTr("日增量 %1").arg(c)
        return line + " · " + formatLongDate(dayData.date)
    }

    width: parent && parent.width > 0 ? parent.width : 360
    implicitWidth: width
    implicitHeight: page.implicitHeight

    function heatmapEntriesForFilter() {
        var raw = ClientBackend.opsHeatmapDays
        var out = []
        if (!raw || raw.length === 0) {
            return out
        }
        for (var i = 0; i < raw.length; i++) {
            var row = raw[i]
            var req = row.requests !== undefined ? Number(row.requests) : 0
            var ev = row.events_published !== undefined ? Number(row.events_published) : 0
            var count = metricFilter === "requests" ? req
                        : metricFilter === "events" ? ev
                        : req + ev
            out.push({
                         date: row.date,
                         count: count
                     })
        }
        return out
    }

    function shortDateLabel(isoDate) {
        if (!isoDate || isoDate.length < 10) {
            return isoDate || ""
        }
        return isoDate.substring(5, 10)
    }

    function lastSevenDaysRows() {
        var raw = ClientBackend.opsHeatmapDays
        if (!raw || raw.length === 0) {
            return []
        }
        var sorted = []
        for (var i = 0; i < raw.length; i++) {
            sorted.push(raw[i])
        }
        sorted.sort(function (a, b) {
            return String(a.date) < String(b.date) ? -1 : 1
        })
        var start = Math.max(0, sorted.length - 7)
        return sorted.slice(start)
    }

    function trendSeriesForChart() {
        var rows = lastSevenDaysRows()
        var reqVals = []
        var evVals = []
        var sumVals = []
        for (var i = 0; i < rows.length; i++) {
            var req = rows[i].requests !== undefined ? Number(rows[i].requests) : 0
            var ev = rows[i].events_published !== undefined ? Number(rows[i].events_published) : 0
            reqVals.push(req)
            evVals.push(ev)
            sumVals.push(req + ev)
        }
        if (reqVals.length === 0) {
            reqVals = [0]
            evVals = [0]
            sumVals = [0]
        }

        if (metricFilter === "requests") {
            return [{
                        label: qsTr("日请求量"),
                        values: reqVals,
                        color: root.trendRequestsColor
                    }]
        }
        if (metricFilter === "events") {
            return [{
                        label: qsTr("日事件量"),
                        values: evVals,
                        color: root.trendEventsColor
                    }]
        }
        return [
            {
                label: qsTr("日请求量"),
                values: reqVals,
                color: root.trendRequestsColor
            },
            {
                label: qsTr("日事件量"),
                values: evVals,
                color: root.trendEventsColor
            },
            {
                label: qsTr("合计"),
                values: sumVals,
                color: root.trendTotalColor
            }
        ]
    }

    function trendCategoriesForChart() {
        var rows = lastSevenDaysRows()
        var cats = []
        for (var i = 0; i < rows.length; i++) {
            cats.push(shortDateLabel(rows[i].date))
        }
        return cats.length > 0 ? cats : [qsTr("—")]
    }

    ClientMobilePage {
        id: page
        width: parent.width
        title: qsTr("运维看板")
        subtitle: qsTr("累计请求与事件发布；热力图与趋势来自 REST /api/v1/ops/history/*")

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Item { Layout.fillWidth: true }
            ShadcnButton {
                iconName: "refresh-cw"
                text: qsTr("刷新")
                variant: "outline"
                size: "sm"
                enabled: !ClientBackend.opsDashboardBusy
                onClicked: ClientBackend.refreshOpsDashboard()
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: ShadcnAppPlatform.isMobile ? 1 : 2
            columnSpacing: 12
            rowSpacing: 12

            ServerMetricTile {
                Layout.fillWidth: true
                iconName: "list-checks"
                title: qsTr("累计请求")
                value: ClientBackend.dashboardRequestsTotal
                hint: qsTr("framework.requests.total · 当前 snapshot")
            }

            ServerMetricTile {
                Layout.fillWidth: true
                iconName: "radio-tower"
                title: qsTr("事件发布")
                value: ClientBackend.dashboardEventsTotal
                hint: qsTr("framework.events.published_total · 含运维 topic 推送")
            }
        }

        ClientStatusAlert {
            Layout.fillWidth: true
            visible: ClientBackend.opsDashboardError.length > 0
            tone: "danger"
            title: qsTr("历史数据不可用")
            description: ClientBackend.opsDashboardError
        }

        ShadcnActivityHeatmap {
            Layout.fillWidth: true
            height: implicitHeight
            showSourceFilter: true
            filterEntriesBySource: false
            /** 手机放不下 53 周：固定格宽 + 横向滑；默认从年初列开始，避免只看到少量月份 */
            fitToWidth: !root.mobileHeatmap
            passWheelToPage: !root.mobileHeatmap
            cellSize: root.mobileHeatmap ? 8 : 12
            pinRecent: false
            maxCellSize: root.mobileHeatmap ? 8 : 0
            caption: root.heatmapCaption
            filterSegments: root.heatmapFilterSegments
            sourceFilter: root.metricFilter
            onSourceFilterChanged: {
                if (root.metricFilter !== sourceFilter) {
                    root.metricFilter = sourceFilter
                }
            }
            entries: {
                var _d = ClientBackend.opsHeatmapDays
                var _f = root.metricFilter
                return root.heatmapEntriesForFilter()
            }
            anchorDate: new Date()
            tooltipFormatter: root.opsHeatmapTooltip
        }

        ShadcnChartDeferred {
            Layout.fillWidth: true
            plotMountDelayMs: 0
            title: qsTr("近 7 日趋势")
            description: qsTr("按日请求 / 事件增量；「全部」显示两条线 + 合计。")
            showHeader: true
            chartType: "line"
            lineStyle: "straight"
            showLineDots: metricFilter === "all"
            yAxisLabelsVisible: true
            showLegend: metricFilter === "all"
            categories: root.trendCategories
            series: root.trendSeries
        }
    }

    Component.onCompleted: ClientBackend.refreshOpsDashboard()
}