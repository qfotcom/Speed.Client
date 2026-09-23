import QtQuick
import SenseAppShell 1.0
import SenseDesign 1.0

SenseAppWindow {
    id: shell

    appTitle: "SpeedClient"
    appIcon: "bolt"
    currentPage: "connection"
    pagePathPrefix: Qt.resolvedUrl("pages/")

    navGroupOrder: ["client"]
    navGroups: ({ "client": "SpeedClient" })

    pageConfig: [
        {
            id: "connection",
            name: "连接",
            icon: "plug",
            group: "client",
            component: "ConnectionPage"
        },
        {
            id: "rest",
            name: "REST",
            icon: "cloud",
            group: "client",
            component: "RestPage"
        },
        {
            id: "legacy",
            name: "推送",
            icon: "satellite-dish",
            group: "client",
            component: "LegacyPage"
        }
    ]

    Connections {
        target: ClientBackend
        function onToastRequested(message, level) {
            var t = level === "error" ? "danger" : (level === "success" ? "success" : "info")
            toast.showMessage(message, t)
        }
    }

    SenseToast {
        id: toast
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: shell._safe.bottom + SenseSpacing.spacer6
    }
}
