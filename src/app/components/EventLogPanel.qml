import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SenseAppShell 1.0
import SenseDesign

ShadcnCard {
    id: root

    property var model: []
    property bool startExpanded: true
    property string searchQuery: ""
    property int currentPage: 1
    property int pageSize: ShadcnAppPlatform.isMobile ? 6 : 10

    readonly property var pageSizeOptions: [6, 10, 20, 50]
    readonly property var pageSizeLabels: [qsTr("6 条"), qsTr("10 条"), qsTr("20 条"), qsTr("50 条")]

    width: parent ? parent.width : implicitWidth
    clip: true
    contentMargin: ShadcnAppPlatform.isMobile ? 14 : 20

    readonly property int _logViewportHeight: ShadcnAppPlatform.isMobile ? 280 : 240

    property var filteredModel: []
    property var pageModel: []
    readonly property int pageCount: Math.max(1, Math.ceil(filteredModel.length / Math.max(1, pageSize)))
    readonly property int filteredCount: filteredModel.length

    function logKind(line) {
        if (!line)
            return "default"
        if (line.indexOf("[PUSH]") >= 0)
            return "push"
        if (line.indexOf("ERR]") >= 0 || line.indexOf("[REST ERR]") >= 0 || line.indexOf("[Legacy ERR]") >= 0)
            return "error"
        if (line.indexOf("[REST") >= 0)
            return "rest"
        if (line.indexOf("[Legacy]") >= 0)
            return "legacy"
        return "default"
    }

    function logBadge(kind) {
        switch (kind) {
        case "push": return "PUSH"
        case "error": return "ERR"
        case "rest": return "REST"
        case "legacy": return "Legacy"
        default: return "LOG"
        }
    }

    function logBadgeTone(kind) {
        switch (kind) {
        case "push": return "success"
        case "error": return "danger"
        default: return "neutral"
        }
    }

    function logAccent(kind) {
        switch (kind) {
        case "push":
            return ShadcnTheme.darkMode ? "#4ade80" : "#15803d"
        case "error": return ShadcnTheme.c("destructive")
        case "rest": return ShadcnTheme.c("chart-2")
        case "legacy": return ShadcnTheme.c("chart-4")
        default: return ShadcnTheme.c("border")
        }
    }

    function pageSizeIndex() {
        for (var i = 0; i < pageSizeOptions.length; ++i) {
            if (pageSizeOptions[i] === pageSize)
                return i
        }
        return 0
    }

    function buildFiltered() {
        var src = root.model || []
        var q = (root.searchQuery || "").trim().toLowerCase()
        var out = []
        for (var i = 0; i < src.length; ++i) {
            var line = String(src[i])
            if (!q || line.toLowerCase().indexOf(q) >= 0)
                out.push(line)
        }
        return out
    }

    function refreshViews() {
        filteredModel = buildFiltered()
        var pages = Math.max(1, Math.ceil(filteredModel.length / Math.max(1, pageSize)))
        if (currentPage > pages)
            currentPage = pages
        if (currentPage < 1)
            currentPage = 1
        var start = (currentPage - 1) * pageSize
        pageModel = filteredModel.slice(start, start + pageSize)
    }

    function applySearch(text) {
        var next = text !== undefined ? text : searchField.text
        if (searchQuery === next) {
            refreshViews()
            return
        }
        searchQuery = next
        currentPage = 1
        refreshViews()
    }

    function setPageSizeByIndex(index) {
        if (index < 0 || index >= pageSizeOptions.length)
            return
        var next = pageSizeOptions[index]
        if (pageSize === next)
            return
        pageSize = next
        currentPage = 1
        refreshViews()
    }

    onModelChanged: {
        currentPage = 1
        refreshViews()
    }

    onPageSizeChanged: refreshViews()

    Component.onCompleted: {
        pageSizeSelect.currentIndex = pageSizeIndex()
        refreshViews()
    }

    ShadcnCollapsible {
        width: parent.width
        title: qsTr("事件日志")
        summary: {
            if (root.model.length === 0)
                return qsTr("REST / Legacy / PUSH 操作记录")
            if ((root.searchQuery || "").trim().length > 0)
                return qsTr("匹配 %1 / 共 %2 · 第 %3/%4 页")
                        .arg(root.filteredCount)
                        .arg(root.model.length)
                        .arg(root.currentPage)
                        .arg(root.pageCount)
            return qsTr("%1 条 · 第 %2/%3 页 · 最新在上")
                    .arg(root.model.length)
                    .arg(root.currentPage)
                    .arg(root.pageCount)
        }
        expanded: root.startExpanded

        ColumnLayout {
            width: parent.width
            spacing: 10

            ShadcnInputGroup {
                id: searchField
                Layout.fillWidth: true
                prefixIcon: "search"
                buttonLabel: qsTr("搜索")
                placeholderText: qsTr("搜索 PUSH / REST / ERR…")
                enabled: root.model.length > 0
                onSearchTriggered: root.applySearch(text)
                onTextChanged: root.applySearch(text)
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                ShadcnBadge {
                    text: root.filteredCount > 0 ? String(root.filteredCount) : "0"
                    variant: root.filteredCount > 0 ? "default" : "secondary"
                }

                ShadcnLabel {
                    visible: root.model.length === 0
                             || ((root.searchQuery || "").trim().length > 0 && root.filteredCount === 0)
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    font.pixelSize: ShadcnTypography.fontSizeXs
                    color: ShadcnTheme.c("muted-foreground")
                    text: root.model.length === 0
                          ? qsTr("执行请求或连接 Legacy 后会出现日志")
                          : qsTr("无匹配结果，试试其它关键词")
                }

                Item {
                    Layout.fillWidth: true
                    visible: root.model.length > 0
                             && !((root.searchQuery || "").trim().length > 0 && root.filteredCount === 0)
                }

                ShadcnSelect {
                    id: pageSizeSelect
                    Layout.preferredWidth: ShadcnAppPlatform.isMobile ? 96 : 110
                    size: "sm"
                    enabled: root.model.length > 0
                    model: root.pageSizeLabels
                    placeholderText: qsTr("每页")
                    onCurrentIndexChanged: root.setPageSizeByIndex(currentIndex)
                }

                ShadcnButton {
                    text: qsTr("清空")
                    variant: "outline"
                    size: "sm"
                    enabled: root.model.length > 0
                    onClicked: {
                        searchField.text = ""
                        root.searchQuery = ""
                        root.currentPage = 1
                        ClientBackend.clearLog()
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: root._logViewportHeight
                radius: ShadcnTheme.radiusMd
                color: ShadcnStyles.withAlpha(ShadcnTheme.c("muted"), 0.45)
                border.color: ShadcnTheme.c("border")
                border.width: 1
                clip: true

                Item {
                    anchors.fill: parent
                    anchors.margins: 8

                    ListView {
                        id: logView
                        anchors.fill: parent
                        model: root.pageModel
                        spacing: 8
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                        onCountChanged: Qt.callLater(function () { positionViewAtBeginning() })

                        delegate: Rectangle {
                            width: logView.width
                            radius: ShadcnTheme.radiusSm
                            color: ShadcnTheme.c("card")
                            border.width: 1
                            border.color: ShadcnTheme.c("border")
                            implicitHeight: rowLayout.implicitHeight + 16

                            readonly property string line: modelData
                            readonly property string kind: root.logKind(line)

                            Rectangle {
                                width: 3
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                anchors.margins: 8
                                radius: 2
                                color: root.logAccent(kind)
                            }

                            RowLayout {
                                id: rowLayout
                                anchors.fill: parent
                                anchors.leftMargin: 14
                                anchors.rightMargin: 10
                                anchors.topMargin: 8
                                anchors.bottomMargin: 8
                                spacing: 8

                                ClientStatusBadge {
                                    text: root.logBadge(kind)
                                    tone: root.logBadgeTone(kind)
                                    Layout.alignment: Qt.AlignTop
                                }

                                ShadcnLabel {
                                    Layout.fillWidth: true
                                    wrapMode: Text.WordWrap
                                    font.pixelSize: ShadcnTypography.fontSizeXs
                                    font.family: "Geist Mono, Consolas, monospace"
                                    color: kind === "error"
                                           ? ShadcnTheme.c("destructive")
                                           : ShadcnTheme.c("foreground")
                                    text: line
                                }
                            }
                        }
                    }

                    ShadcnLabel {
                        anchors.centerIn: parent
                        visible: root.pageModel.length === 0
                        width: parent.width - 24
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        color: ShadcnTheme.c("muted-foreground")
                        font.pixelSize: ShadcnTypography.fontSizeSm
                        text: root.model.length === 0
                              ? qsTr("暂无日志")
                              : qsTr("无匹配结果")
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: pager.implicitHeight
                visible: root.filteredCount > 0 && root.pageCount > 1
                clip: true

                ShadcnPagination {
                    id: pager
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: Math.min(implicitWidth, parent.width)
                    page: root.currentPage
                    pageCount: root.pageCount
                    compact: ShadcnAppPlatform.isMobile
                    siblingCount: ShadcnAppPlatform.isMobile ? 0 : 1
                    onPageSelected: function (p) {
                        root.currentPage = p
                        root.refreshViews()
                        logView.positionViewAtBeginning()
                    }
                }
            }
        }
    }
}
