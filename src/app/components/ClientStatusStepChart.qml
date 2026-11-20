import QtQuick
import SenseDesign

/**
 * 状态页 Step 图：与 SenseDesign DemoChart 同一套 ShadcnChartDeferred（含 Y 刻度）。
 * immediatePlot=true：实时波形不 deferred（避免 Repeater 更新时骨架闪一下）。
 */
ShadcnChartDeferred {
    id: root

    /** 实时数据图设为 true；单图首次入场可保持默认 false */
    property bool immediatePlot: false
    deferPlotMount: !immediatePlot

    width: parent && parent.width > 0 ? parent.width : implicitWidth
    plotMountDelayMs: 0
    chartType: "line"
    /** stepLeft 在 index 0 向左画水平段，易被 plot 左缘 clip；与滑动窗「新点在右」用 stepRight */
    lineStyle: "stepRight"
    yAxisFixedRange: true
    yAxisFixedMin: 0
    yAxisFixedMax: 100
    yAxisFixedTickInterval: 50
    yAxisLabelsVisible: true
    showLegend: true
}
