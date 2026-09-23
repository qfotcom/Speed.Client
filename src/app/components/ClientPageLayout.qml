import QtQuick
import QtQuick.Layouts
import SenseAppShell 1.0
import SenseDesign 1.0

/** @brief SpeedClient 页面骨架：SenseAppPageLayout，默认 pretitle 为应用名 */
SenseAppPageLayout {
    pretitle: "SpeedClient"

    /** @brief 滚动区底部留白，避免最后一项（事件日志）贴边或被系统手势区挡住 */
    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: SensePlatform.isMobile ? SenseSpacing.spacer8 : SenseSpacing.spacer4
    }
}
