import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SenseDesign 1.0
import "../components"

ClientPageLayout {
    title: "Legacy 与推送"
    description: "行协议 + 4 字节帧。Workflow 模式下 PUSH 随下一次响应下发；开启轮询 PING 可及时收到推送。"

    SenseCard {
        Layout.fillWidth: true
        title: "Legacy"
        iconName: "satellite-dish"

        body: Component {
            ColumnLayout {
                width: parent ? parent.width : 360
                spacing: SenseSpacing.spacer3

                RowLayout {
                    Layout.fillWidth: true
                    spacing: SenseSpacing.spacer3

                    SenseButton {
                        text: "PING"
                        variant: "outline"
                        iconLeft: "chart-line"
                        enabled: ClientBackend.legacyConnected && !ClientBackend.legacyBusy
                        onClicked: ClientBackend.legacyPing()
                    }
                    SenseButton {
                        text: "REQ Echo"
                        variant: "secondary"
                        iconLeft: "terminal"
                        enabled: ClientBackend.legacyConnected && !ClientBackend.legacyBusy
                        onClicked: ClientBackend.legacyEcho()
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: SenseSpacing.spacer3

                    SenseButton {
                        text: "SUB"
                        variant: "primary"
                        iconLeft: "bell"
                        enabled: ClientBackend.legacyConnected && !ClientBackend.legacyBusy
                        Layout.fillWidth: true
                        onClicked: ClientBackend.legacySubscribe()
                    }
                    SenseButton {
                        text: "UNSUB"
                        variant: "ghost"
                        enabled: ClientBackend.legacyConnected && !ClientBackend.legacyBusy
                        Layout.fillWidth: true
                        onClicked: ClientBackend.legacyUnsubscribe()
                    }
                }

                SenseSwitch {
                    text: "轮询 PING（接收 PUSH）"
                    checked: ClientBackend.pushPollEnabled
                    onToggled: ClientBackend.pushPollEnabled = checked
                }

                SenseMetricCard {
                    Layout.fillWidth: true
                    metricTitle: "最近响应"
                    value: ClientBackend.lastLegacyLine.length > 0
                           ? ClientBackend.lastLegacyLine
                           : "—"
                    icon: "code"
                }

                SenseAlert {
                    Layout.fillWidth: true
                    type: "info"
                    title: "联调提示"
                    description: "服务端执行 PUB " + ClientBackend.subscribeTopic
                                 + " <payload>；订阅并轮询 PING 后应看到 [PUSH] 日志。"
                }
            }
        }
    }

    EventLogPanel { model: ClientBackend.eventLog }
}
