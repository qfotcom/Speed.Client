import QtQuick
import QtQuick.Shapes
import SenseDesign

/**
 * 泳道阶梯线：自绘坐标 0=底 / 100=顶，绕开 Qt Graphs 在「全 0」时把 Y 轴收成一点的问题。
 */
Item {
    id: root

    property var values: [0]
    property color seriesColor: ShadcnTheme.c("chart-1")
    property string channelLabel: ""
    property bool showTimeAxis: false
    property var categoryAxisLabels: []
    property var categoryHoverLabels: []

    readonly property int padLeft: 10
    readonly property int padRight: 10
    readonly property int padTop: 12
    readonly property int padBottom: showTimeAxis ? 22 : 14
    readonly property int pointCount: Math.max(1, values ? values.length : 1)

    property int hoverIndex: -1
    property real hoverMouseX: 0

    readonly property color strokeColor: ShadcnChartTheme.cartesianSeriesStrokeColor(seriesColor)
    readonly property color laneGridMajor: ShadcnTheme.darkModeVisual
            ? ShadcnStyles.withAlpha(ShadcnTheme.c("muted-foreground"), 0.22)
            : ShadcnStyles.withAlpha(ShadcnTheme.c("border"), 0.58)
    readonly property color laneGridMinor: ShadcnTheme.darkModeVisual
            ? ShadcnStyles.withAlpha(ShadcnTheme.c("muted-foreground"), 0.14)
            : ShadcnStyles.withAlpha(ShadcnTheme.c("border"), 0.42)
    readonly property real laneGridMajorOpacity: ShadcnTheme.darkModeVisual ? 0.5 : 0.85
    readonly property real laneGridMinorOpacity: ShadcnTheme.darkModeVisual ? 0.45 : 0.8

    function valueAt(i) {
        if (!values || i < 0 || i >= values.length)
            return 0
        var v = Number(values[i])
        return isNaN(v) ? 0 : Math.max(0, Math.min(100, v))
    }

    /** 0 → 底缘，100 → 顶缘 */
    function laneY(val) {
        var innerH = Math.max(1, height - padTop - padBottom)
        var t = Math.max(0, Math.min(100, val)) / 100
        return padTop + (1 - t) * innerH
    }

    function slotWidth() {
        return Math.max(1, (width - padLeft - padRight) / pointCount)
    }

    function xCenter(i) {
        return padLeft + (i + 0.5) * slotWidth()
    }

    function xRightEdge() {
        return width - padRight
    }

    function hoverIndexAt(mouseX) {
        var innerW = width - padLeft - padRight
        if (innerW <= 0)
            return -1
        var t = (mouseX - padLeft) / innerW
        if (t < 0 || t > 1)
            return -1
        var idx = Math.floor(t * pointCount)
        return Math.max(0, Math.min(pointCount - 1, idx))
    }

    function hoverTitle() {
        if (hoverIndex < 0)
            return ""
        if (categoryHoverLabels && hoverIndex < categoryHoverLabels.length
                && String(categoryHoverLabels[hoverIndex]).length > 0)
            return String(categoryHoverLabels[hoverIndex])
        if (categoryAxisLabels && hoverIndex < categoryAxisLabels.length)
            return String(categoryAxisLabels[hoverIndex])
        return String(hoverIndex)
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: padLeft
        anchors.rightMargin: padRight
        y: laneY(100)
        height: 1
        color: laneGridMajor
        opacity: laneGridMajorOpacity
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: padLeft
        anchors.rightMargin: padRight
        y: laneY(0)
        height: 1
        color: laneGridMajor
        opacity: laneGridMajorOpacity
    }

    Repeater {
        model: Math.max(0, pointCount - 1)
        Rectangle {
            required property int index
            x: padLeft + (index + 1) * root.slotWidth()
            y: padTop
            width: 1
            height: root.height - padTop - padBottom
            color: laneGridMinor
            opacity: laneGridMinorOpacity
        }
    }

    Rectangle {
        visible: showTimeAxis
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: padLeft
        anchors.rightMargin: padRight
        y: root.height - padBottom
        height: 1
        color: laneGridMajor
        opacity: laneGridMajorOpacity
    }

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer
        antialiasing: true

        ShapePath {
            strokeColor: root.strokeColor
            strokeWidth: ShadcnChartTheme.cartesianLineWidth()
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin

            PathSvg {
                path: root._stepSvg
            }
        }
    }

    readonly property string _stepSvg: {
        var n = pointCount
        if (n < 1 || width <= 0 || height <= 0)
            return ""
        var d = ""
        var y0 = laneY(valueAt(0))
        d += "M " + padLeft + " " + y0
        d += " L " + xCenter(0) + " " + y0
        for (var i = 0; i < n - 1; ++i) {
            var yi = laneY(valueAt(i))
            var yi1 = laneY(valueAt(i + 1))
            var xc = xCenter(i + 1)
            d += " L " + xc + " " + yi
            d += " L " + xc + " " + yi1
        }
        var yLast = laneY(valueAt(n - 1))
        d += " L " + xRightEdge() + " " + yLast
        return d
    }

    Row {
        visible: showTimeAxis
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 4
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: padLeft
        anchors.rightMargin: padRight
        height: 16
        spacing: 0

        Repeater {
            model: pointCount
            ShadcnLabel {
                required property int index
                width: slotWidth()
                horizontalAlignment: Text.AlignHCenter
                text: (categoryAxisLabels && index < categoryAxisLabels.length)
                        ? String(categoryAxisLabels[index]) : ""
                font.pixelSize: ShadcnTypography.fontSizeXs
                color: ShadcnTheme.c("muted-foreground")
                elide: Text.ElideRight
            }
        }
    }

    Rectangle {
        visible: hoverIndex >= 0
        x: xCenter(hoverIndex) - 0.5
        y: padTop
        width: 1
        height: root.height - padTop - padBottom
        color: ShadcnTheme.c("chart-1")
        opacity: 0.85
    }

    Rectangle {
        id: hoverDot
        visible: hoverIndex >= 0
        width: 10
        height: 10
        radius: 5
        color: strokeColor
        border.color: ShadcnTheme.c("card")
        border.width: 1.5
        x: xCenter(hoverIndex) - width / 2
        y: laneY(valueAt(hoverIndex)) - height / 2
    }

    Rectangle {
        id: tip
        visible: hoverIndex >= 0
        z: 10
        radius: ShadcnTheme.radiusSm
        color: ShadcnTheme.c("popover")
        border.color: ShadcnTheme.c("border")
        border.width: 1
        implicitWidth: tipCol.implicitWidth + 16
        implicitHeight: tipCol.implicitHeight + 12
        x: Math.min(Math.max(4, hoverMouseX - width / 2), root.width - width - 4)
        y: Math.max(4, hoverDot.y - implicitHeight - 8)

        Column {
            id: tipCol
            anchors.centerIn: parent
            spacing: 2

            ShadcnLabel {
                text: hoverTitle()
                font.pixelSize: ShadcnTypography.fontSizeXs
                font.weight: ShadcnTypography.fontWeightMedium
                color: ShadcnTheme.c("foreground")
            }
            ShadcnLabel {
                text: channelLabel.length > 0
                        ? channelLabel + ": " + Math.round(valueAt(hoverIndex))
                        : String(Math.round(valueAt(hoverIndex)))
                font.pixelSize: ShadcnTypography.fontSizeXs
                color: ShadcnTheme.c("muted-foreground")
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onPositionChanged: function (mouse) {
            hoverMouseX = mouse.x
            hoverIndex = hoverIndexAt(mouse.x)
        }
        onExited: hoverIndex = -1
    }
}