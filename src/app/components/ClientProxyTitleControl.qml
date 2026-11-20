import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SenseDesign
import SenseAppShell 1.0

/** 标题栏入口 + ShadcnDialog 居中配置本地代理（草稿，仅「应用」提交）。 */
Item {
    id: root

    implicitWidth: 32
    implicitHeight: 32

    readonly property color _fg: ShadcnTheme.c("foreground")
    readonly property bool _enabled: ClientBackend.proxyEnabled

    property bool draftEnabled: false
    property string draftHost: "127.0.0.1"
    property int draftPort: 7890
    property bool draftSocks5: true

    function loadDraftFromBackend() {
        draftEnabled = ClientBackend.proxyEnabled
        draftHost = ClientBackend.proxyHost
        draftPort = ClientBackend.proxyPort
        draftSocks5 = ClientBackend.proxySocks5
    }

    function commitDraft() {
        ClientBackend.proxyEnabled = draftEnabled
        ClientBackend.proxyHost = draftHost
        ClientBackend.proxyPort = draftPort
        ClientBackend.proxySocks5 = draftSocks5
        ClientBackend.applyProxySettings()
    }

    ShadcnDialog {
        id: proxyDialog
        showTrigger: false
        title: qsTr("网络代理")
        description: _enabled
                     ? qsTr("当前生效：%1").arg(ClientBackend.proxySummary)
                     : qsTr("未启用 · REST 与 Legacy 默认直连。")
        cancelText: qsTr("取消")
        confirmText: qsTr("应用")

        onOpenChanged: {
            if (open)
                root.loadDraftFromBackend()
        }
        onCanceled: { /* 关闭即丢弃草稿 */ }
        onConfirmed: root.commitDraft()

        RowLayout {
            width: parent.width
            spacing: 8
            ShadcnLabel {
                Layout.fillWidth: true
                text: qsTr("使用本地代理")
                font.pixelSize: ShadcnTypography.fontSizeSm
            }
            ShadcnSwitch {
                checked: root.draftEnabled
                onToggled: root.draftEnabled = checked
            }
        }

        ColumnLayout {
            width: parent.width
            spacing: 10
            enabled: root.draftEnabled
            opacity: enabled ? 1 : 0.5

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 6
                ShadcnLabel {
                    text: qsTr("协议")
                    font.pixelSize: ShadcnTypography.fontSizeXs
                    color: ShadcnTheme.c("muted-foreground")
                }
                ShadcnSlidingSegmented {
                    Layout.fillWidth: true
                    value: root.draftSocks5 ? "socks5" : "http"
                    segments: [
                        { value: "socks5", label: "SOCKS5" },
                        { value: "http", label: "HTTP" }
                    ]
                    onOptionActivated: function (v) {
                        root.draftSocks5 = (v === "socks5")
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 120
                    spacing: 6
                    ShadcnLabel {
                        text: qsTr("主机")
                        font.pixelSize: ShadcnTypography.fontSizeXs
                        color: ShadcnTheme.c("muted-foreground")
                    }
                    ShadcnInput {
                        Layout.fillWidth: true
                        text: root.draftHost
                        placeholderText: qsTr("127.0.0.1")
                        onEditingFinished: root.draftHost = text.trim()
                    }
                }
                ColumnLayout {
                    Layout.preferredWidth: 88
                    spacing: 6
                    ShadcnLabel {
                        text: qsTr("端口")
                        font.pixelSize: ShadcnTypography.fontSizeXs
                        color: ShadcnTheme.c("muted-foreground")
                    }
                    ShadcnInput {
                        Layout.fillWidth: true
                        text: String(root.draftPort)
                        inputMethodHints: Qt.ImhDigitsOnly
                        placeholderText: "7890"
                        onEditingFinished:
                            root.draftPort = parseInt(text) || 7890
                    }
                }
            }

            ShadcnLabel {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                font.pixelSize: ShadcnTypography.fontSizeXs
                color: ShadcnTheme.c("muted-foreground")
                text: qsTr("与「连接」页端点独立；本机 Clash 等常用 SOCKS5 · 127.0.0.1:7890。")
            }
        }
    }

    Rectangle {
        id: hit
        anchors.fill: parent
        radius: ShadcnTheme.radiusSm
        color: proxyDialog.open || chromeArea.containsMouse
               ? ShadcnTheme.c("accent")
               : "transparent"

        ShadcnIcon {
            anchors.centerIn: parent
            name: "globe"
            size: 16
            color: proxyDialog.open || chromeArea.containsMouse
                   ? ShadcnTheme.c("accent-foreground")
                   : (_enabled ? ShadcnTheme.c("primary") : _fg)
        }

        MouseArea {
            id: chromeArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: proxyDialog.open = !proxyDialog.open
            onContainsMouseChanged: {
                if (containsMouse && !proxyDialog.open) {
                    tipTimer.restart()
                } else {
                    tipTimer.stop()
                    tipPopup.close()
                }
            }
        }

        Timer {
            id: tipTimer
            interval: ShadcnMotion.durationTooltipDelay
            onTriggered: {
                if (chromeArea.containsMouse && !proxyDialog.open)
                    tipPopup.open()
            }
        }

        Popup {
            id: tipPopup
            parent: Overlay.overlay
            modal: false
            focus: false
            padding: 6
            closePolicy: Popup.NoAutoClose
            transformOrigin: Item.Top
            onAboutToShow: {
                ShadcnOverlay.onPopupAboutToShow(tipPopup)
                ShadcnOverlay.repositionAnchoredPopup(tipPopup, hit, Qt.BottomEdge, 6, true)
            }
            background: Rectangle {
                radius: ShadcnTheme.radiusSm
                color: ShadcnTheme.c("primary")
                border.width: 1
                border.color: ShadcnStyles.withAlpha(ShadcnTheme.c("primary-foreground"), 0.12)
            }
            contentItem: Text {
                text: _enabled ? ClientBackend.proxySummary : qsTr("本地代理 · 直连")
                color: ShadcnTheme.c("primary-foreground")
                font.pixelSize: ShadcnTypography.fontSizeXs
                font.weight: ShadcnTypography.fontWeightMedium
            }
        }
    }
}