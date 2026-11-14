import QtQuick
import QtQuick.Layouts
import SenseAppShell 1.0
import SenseDesign
import "../components"

Item {
    id: root
    property string pageId: "server"
    width: parent && parent.width > 0 ? parent.width : 360
    implicitWidth: width
    implicitHeight: page.implicitHeight

    readonly property bool compactGrid: width < 400
    readonly property double cpuPct: ClientBackend.serverCpuPercent
    readonly property double memPct: ClientBackend.serverMemUsedPercent
    readonly property bool hasCpu: cpuPct >= 0
    readonly property bool hasMem: memPct >= 0

    ClientMobilePage {
        id: page
        width: parent.width
        title: qsTr("服务器状态")
        subtitle: qsTr("Legacy 流式：SUB ops.host.metrics 后服务端有变才 PUSH；REST 为当场采样校准。")

        ShadcnCard {
            Layout.fillWidth: true
            contentMargin: ShadcnAppPlatform.isMobile ? 14 : 20

            ColumnLayout {
                width: parent.width
                spacing: 14

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    ShadcnIcon { name: "server"; size: 20 }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        ShadcnLabel {
                            Layout.fillWidth: true
                            text: ClientBackend.serverAppName.length > 0
                                  ? ClientBackend.serverAppName
                                  : qsTr("Speed.Server")
                            font.weight: ShadcnTypography.fontWeightSemibold
                            font.pixelSize: ShadcnTypography.fontSizeLg
                        }
                        ShadcnLabel {
                            Layout.fillWidth: true
                            text: ClientBackend.serverPlatformLabel.length > 0
                                  ? ClientBackend.serverPlatformLabel
                                  : qsTr("等待 snapshot…")
                            font.pixelSize: ShadcnTypography.fontSizeXs
                            color: ShadcnTheme.c("muted-foreground")
                        }
                    }
                    ClientStatusBadge {
                        text: ClientBackend.serverSnapshotBusy
                              ? qsTr("REST 刷新中")
                              : (ClientBackend.serverSnapshotReady ? qsTr("有数据") : qsTr("等待首屏"))
                        tone: ClientBackend.serverSnapshotBusy
                              ? "pending"
                              : (ClientBackend.serverSnapshotReady ? "success" : "neutral")
                    }
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 8
                    ClientStatusBadge {
                        text: ClientBackend.serverOpsStreamActive
                              ? qsTr("流式推送中")
                              : qsTr("推送未就绪")
                        tone: ClientBackend.serverOpsStreamActive ? "success" : "pending"
                        reserveActivitySlot: ClientBackend.serverOpsStreamActive
                        activity: ClientBackend.serverOpsStreamActive
                    }
                    ClientValueTierBadge {
                        id: legacyPingBadge
                        value: ClientBackend.legacyPingRttMs
                        pending: !ClientBackend.legacyConnected || ClientBackend.legacyPingRttMs < 0
                        text: ClientBackend.legacyPingRttMs >= 0
                              ? qsTr("Ping %1 ms").arg(ClientBackend.legacyPingRttMs)
                              : qsTr("Ping …")
                    }
                    ClientStatusBadge {
                        visible: ClientBackend.serverProcessRssDetail.length > 0
                        text: qsTr("进程 %1").arg(ClientBackend.serverProcessRssDetail)
                        tone: "neutral"
                    }
                    ClientStatusBadge {
                        text: qsTr("更新于 %1").arg(
                            ClientBackend.serverSnapshotUpdatedLabel.length > 0
                                ? ClientBackend.serverSnapshotUpdatedLabel
                                : "—")
                        tone: ClientBackend.serverSnapshotReady ? "neutral" : "pending"
                    }
                }

                ClientStatusAlert {
                    Layout.fillWidth: true
                    visible: ClientBackend.serverSnapshotError.length > 0
                    tone: "danger"
                    title: qsTr("无法读取 snapshot")
                    description: ClientBackend.serverSnapshotError
                }

                ShadcnButton {
                    Layout.fillWidth: true
                    iconName: "refresh-cw"
                    text: qsTr("REST 手动拉快照")
                    variant: "outline"
                    enabled: !ClientBackend.serverSnapshotBusy
                    onClicked: ClientBackend.fetchServerSnapshot()
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: compactGrid ? 1 : 2
            columnSpacing: 12
            rowSpacing: 12

            ShadcnCard {
                Layout.fillWidth: true
                contentMargin: 16
                ColumnLayout {
                    width: parent.width
                    spacing: 8
                    RowLayout {
                        Layout.fillWidth: true
                        ShadcnIcon { name: "cpu"; size: 16; color: ShadcnTheme.c("chart-1") }
                        ShadcnLabel {
                            Layout.fillWidth: true
                            text: qsTr("主机 CPU")
                            font.weight: ShadcnTypography.fontWeightMedium
                        }
                        ShadcnLabel {
                            text: hasCpu ? ("%1%").arg(cpuPct.toFixed(1)) : "—"
                            font.weight: ShadcnTypography.fontWeightSemibold
                            color: hasCpu && cpuPct > 85
                                   ? ShadcnTheme.c("destructive")
                                   : ShadcnTheme.c("foreground")
                        }
                    }
                    ShadcnProgress {
                        Layout.fillWidth: true
                        value: hasCpu ? cpuPct : 0
                        indeterminateMode: !hasCpu && ClientBackend.serverSnapshotBusy
                    }
                }
            }

            ShadcnCard {
                Layout.fillWidth: true
                contentMargin: 16
                ColumnLayout {
                    width: parent.width
                    spacing: 8
                    RowLayout {
                        Layout.fillWidth: true
                        ShadcnIcon { name: "memory-stick"; size: 16; color: ShadcnTheme.c("chart-2") }
                        ShadcnLabel {
                            Layout.fillWidth: true
                            text: qsTr("物理内存")
                            font.weight: ShadcnTypography.fontWeightMedium
                        }
                        ShadcnLabel {
                            text: hasMem ? ("%1%").arg(memPct.toFixed(1)) : "—"
                            font.weight: ShadcnTypography.fontWeightSemibold
                        }
                    }
                    ShadcnProgress {
                        Layout.fillWidth: true
                        value: hasMem ? memPct : 0
                        indeterminateMode: !hasMem && ClientBackend.serverSnapshotBusy
                    }
                    ShadcnLabel {
                        Layout.fillWidth: true
                        visible: ClientBackend.serverHostMemDetail.length > 0
                        wrapMode: Text.WordWrap
                        font.pixelSize: ShadcnTypography.fontSizeXs
                        color: ShadcnTheme.c("muted-foreground")
                        text: ClientBackend.serverHostMemDetail
                    }
                }
            }

            ShadcnCard {
                Layout.fillWidth: true
                visible: ClientBackend.serverHasRootDisk
                contentMargin: 16
                ColumnLayout {
                    width: parent.width
                    spacing: 8
                    RowLayout {
                        Layout.fillWidth: true
                        ShadcnIcon { name: "hard-drive"; size: 16; color: ShadcnTheme.c("chart-3") }
                        ShadcnLabel {
                            Layout.fillWidth: true
                            text: qsTr("系统盘")
                            font.weight: ShadcnTypography.fontWeightMedium
                        }
                        ShadcnLabel {
                            text: ClientBackend.serverRootDiskUsedPercent >= 0
                                  ? ("%1%").arg(ClientBackend.serverRootDiskUsedPercent.toFixed(1))
                                  : "—"
                            font.weight: ShadcnTypography.fontWeightSemibold
                        }
                    }
                    ShadcnLabel {
                        Layout.fillWidth: true
                        visible: ClientBackend.serverRootDiskMount.length > 0
                        text: ClientBackend.serverRootDiskMount
                        font.pixelSize: ShadcnTypography.fontSizeXs
                        color: ShadcnTheme.c("muted-foreground")
                    }
                    ShadcnProgress {
                        Layout.fillWidth: true
                        value: ClientBackend.serverRootDiskUsedPercent >= 0
                               ? ClientBackend.serverRootDiskUsedPercent
                               : 0
                    }
                    ShadcnLabel {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        font.pixelSize: ShadcnTypography.fontSizeXs
                        color: ShadcnTheme.c("muted-foreground")
                        text: ClientBackend.serverRootDiskDetail
                    }
                }
            }

            ShadcnCard {
                Layout.fillWidth: true
                visible: ClientBackend.serverHasDataDisk
                contentMargin: 16
                ColumnLayout {
                    width: parent.width
                    spacing: 8
                    RowLayout {
                        Layout.fillWidth: true
                        ShadcnIcon { name: "database"; size: 16; color: ShadcnTheme.c("chart-4") }
                        ShadcnLabel {
                            Layout.fillWidth: true
                            text: qsTr("数据盘")
                            font.weight: ShadcnTypography.fontWeightMedium
                        }
                        ShadcnLabel {
                            text: ClientBackend.serverDataDiskUsedPercent >= 0
                                  ? ("%1%").arg(ClientBackend.serverDataDiskUsedPercent.toFixed(1))
                                  : "—"
                            font.weight: ShadcnTypography.fontWeightSemibold
                        }
                    }
                    ShadcnLabel {
                        Layout.fillWidth: true
                        visible: ClientBackend.serverDataDiskMount.length > 0
                        text: ClientBackend.serverDataDiskMount
                        font.pixelSize: ShadcnTypography.fontSizeXs
                        color: ShadcnTheme.c("muted-foreground")
                    }
                    ShadcnProgress {
                        Layout.fillWidth: true
                        value: ClientBackend.serverDataDiskUsedPercent >= 0
                               ? ClientBackend.serverDataDiskUsedPercent
                               : 0
                    }
                    ShadcnLabel {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        font.pixelSize: ShadcnTypography.fontSizeXs
                        color: ShadcnTheme.c("muted-foreground")
                        text: ClientBackend.serverDataDiskDetail
                    }
                }
            }

            ShadcnCard {
                Layout.fillWidth: true
                Layout.columnSpan: compactGrid ? 1 : 2
                contentMargin: 16
                ColumnLayout {
                    width: parent.width
                    spacing: 8
                    RowLayout {
                        Layout.fillWidth: true
                        ShadcnIcon { name: "radio"; size: 16; color: ShadcnTheme.c("chart-5") }
                        ShadcnLabel {
                            Layout.fillWidth: true
                            text: qsTr("本机 → Legacy 延迟（TCP 9001 往返）")
                            font.weight: ShadcnTypography.fontWeightMedium
                        }
                        ShadcnLabel {
                            text: ClientBackend.legacyPingRttMs >= 0
                                  ? (ClientBackend.legacyPingRttMs + " ms")
                                  : "—"
                            font.weight: ShadcnTypography.fontWeightSemibold
                            color: legacyPingBadge.valueForeground
                        }
                    }
                    ShadcnLabel {
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        font.pixelSize: ShadcnTypography.fontSizeXs
                        color: ShadcnTheme.c("muted-foreground")
                        text: ClientBackend.legacyPingLabel + " · "
                              + qsTr("非 CMD/ICMP，与业务 Legacy 通道一致")
                    }
                }
            }
        }

        ShadcnCard {
            Layout.fillWidth: true
            visible: !ClientBackend.serverHasDataDisk && ClientBackend.serverSnapshotReady
            contentMargin: 14
            ShadcnLabel {
                width: parent.width
                wrapMode: Text.WordWrap
                font.pixelSize: ShadcnTypography.fontSizeXs
                color: ShadcnTheme.c("muted-foreground")
                text: qsTr("未上报数据盘：请在服务端 settings 配置 ops.metrics.disk_mounts（如 system=/,data=/data）。")
            }
        }

        ShadcnCard {
            Layout.fillWidth: true
            contentMargin: ShadcnAppPlatform.isMobile ? 14 : 18

            ColumnLayout {
                width: parent.width
                spacing: 12

                ShadcnLabel {
                    text: qsTr("框架指标")
                    font.weight: ShadcnTypography.fontWeightSemibold
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: root.compactGrid ? 1 : (root.width > 720 ? 3 : 2)
                    columnSpacing: 10
                    rowSpacing: 10

                    Repeater {
                        model: ClientBackend.serverOpsTiles

                        ServerMetricTile {
                            Layout.fillWidth: true
                            iconName: modelData.icon
                            title: modelData.title
                            value: modelData.value
                            hint: modelData.hint
                        }
                    }
                }
            }
        }
    }
}
