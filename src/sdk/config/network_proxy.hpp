#pragma once

#include "config/client_settings.hpp"

namespace speed::client {

/** 将 ClientSettings 中的代理选项应用到 Qt 全局（REST / 其它 QNetworkAccessManager）。 */
void applyNetworkProxy(const ClientSettings &cfg);

QString proxySummaryLabel(const ClientSettings &cfg);

} // namespace speed::client