import QtQuick
import SenseAppShell 1.0
import SenseDesign
import "page-config.js" as PageConfig

ShadcnAppWindow {
    id: shell

    docSiteChrome: false
    appTitle: qsTr("SpeedClient")
    pagePathPrefix: Qt.resolvedUrl("pages/")
    pageConfig: PageConfig.pages
    navGroups: PageConfig.navGroups
    navGroupOrder: PageConfig.navGroupOrder
    currentPage: "connection"

    ShadcnSonner {
        id: sonner
        parent: shell.contentItem
        anchors.fill: parent
        showTrigger: false
        showCloseButton: true
        stackTop: 12
        z: 1000
        defaultDuration: 3500
    }

    Connections {
        target: ClientBackend
        function onToastRequested(message, level) {
            sonner.toast(message)
        }
    }
}
