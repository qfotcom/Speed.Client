import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import SenseDesign 1.0
import "../components"

ClientPageLayout {
    title: "REST"
    description: "GET /health 与 GET /api/v1/echo?text=...（Drogon 适配器）。"

    SenseCard {
        Layout.fillWidth: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: SenseSpacing.spacer4
            spacing: SenseSpacing.spacer3

            RowLayout {
                Layout.fillWidth: true
                spacing: SenseSpacing.spacer3

                SenseButton {
                    text: "Health"
                    variant: "primary"
                    iconLeft: "heart-pulse"
                    enabled: !ClientBackend.restBusy
                    Layout.fillWidth: true
                    onClicked: ClientBackend.restHealth()
                }
                SenseButton {
                    text: "Echo"
                    variant: "secondary"
                    iconLeft: "message"
                    enabled: !ClientBackend.restBusy
                    Layout.fillWidth: true
                    onClicked: ClientBackend.restEcho()
                }
            }

            SenseMetricCard {
                Layout.fillWidth: true
                metricTitle: "/health"
                value: ClientBackend.lastRestHealth.length > 0
                       ? ClientBackend.lastRestHealth
                       : "—"
                icon: "chart-line"
            }

            SenseMetricCard {
                Layout.fillWidth: true
                metricTitle: "/api/v1/echo"
                value: ClientBackend.lastRestEcho.length > 0
                       ? ClientBackend.lastRestEcho
                       : "—"
                icon: "comment-dots"
            }
        }
    }

    EventLogPanel { model: ClientBackend.eventLog }
}
