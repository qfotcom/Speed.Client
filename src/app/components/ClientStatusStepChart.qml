import QtQuick
import SenseDesign

/**
 * 状态页 Step 图：与 SenseDesign DemoChart 同一套 ShadcnChart（含 Y 刻度），
 * 仅序列值为开关量 0/100。
 */
Item {
    id: root

    property alias title: chart.title
    property alias description: chart.description
    property alias footerText: chart.footerText
    property alias categories: chart.categories
    property alias series: chart.series
    property alias values: chart.values
    property alias seriesLabel: chart.seriesLabel
    property alias seriesColor: chart.seriesColor
    property alias showHeader: chart.showHeader
    property alias showLegend: chart.showLegend
    property alias yAxisLabelsVisible: chart.yAxisLabelsVisible

    implicitWidth: chart.implicitWidth
    implicitHeight: chart.implicitHeight
    width: parent && parent.width > 0 ? parent.width : implicitWidth
    height: implicitHeight > 0 ? implicitHeight : chart.implicitHeight

    ShadcnChart {
        id: chart
        anchors.fill: parent
        chartType: "line"
        lineStyle: "stepLeft"
        yAxisFixedRange: true
        yAxisFixedMin: 0
        yAxisFixedMax: 100
        yAxisFixedTickInterval: 50
        yAxisLabelsVisible: true
        showLegend: true
    }
}
