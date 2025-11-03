
# TrafficMonitor GeoIP 插件（独立开源仓库草案）

本仓库用于开源发布「TrafficMonitor GeoIP 插件」。插件可在任务栏显示通过梯子（VPN/代理）上网时的当前出口地区归属（国家代码·城市），并在鼠标提示中显示 IP / 国家 / 城市 / ISP / ASN / 可疑性（是否可能为云厂商/代理）等信息。

当前状态：本 README 为开源仓库的第一步文档草案。随后会把插件源码、构建脚本、示例配置、CI 等整理迁移到本仓库内（在“进度与路线图”中说明）。

- 插件主实现参考现有工程：
  - 源码入口：[GeoIP.cpp](TrafficMonitorPlugins-IpAddress_V1.00/Plugins/GeoIP/GeoIP.cpp)
  - 项目文件：[GeoIP.vcxproj](TrafficMonitorPlugins-IpAddress_V1.00/Plugins/GeoIP/GeoIP.vcxproj)
  - 插件接口定义：[PluginInterface.h](TrafficMonitorPlugins-IpAddress_V1.00/include/PluginInterface.h)

---

## 功能特性

- 任务栏项目显示“国家代码·城市”，支持显示模板控制
  - 默认模板为 `<%1%>·<%2%>`（`%1 = countryCode`, `%2 = city`）
  - 模板组合逻辑：[CGeoIP::ComposeTexts()](TrafficMonitorPlugins-IpAddress_V1.00/Plugins/GeoIP/GeoIP.cpp:83)
- 数据源级联回退（自动探测，尽力显示）
  - 1st：ip-api.com（JSON）→ [FetchViaIpApi()](TrafficMonitorPlugins-IpAddress_V1.00/Plugins/GeoIP/GeoIP.cpp:137)
  - 2nd：ipinfo.io（JSON）→ [FetchViaIpInfo()](TrafficMonitorPlugins-IpAddress_V1.00/Plugins/GeoIP/GeoIP.cpp:176)
  - 3rd：whois.pconline.com.cn（纯文本，中文地区）→ [FetchViaPconline()](TrafficMonitorPlugins-IpAddress_V1.00/Plugins/GeoIP/GeoIP.cpp:206)
- 启发式“可疑性”标记（是否可能为云厂商/代理/VPN 出口）
  - 基于 ISP/ASN 字符串关键字匹配（cloudflare/akamai/aws/azure/gcp/ovh/hetzner/linode/leaseweb…）
  - 逻辑见上述两个数据源函数
- 右键菜单切换显示模式（立即生效并持久化）
  - 菜单项定义与实现：[GetCommandName()](TrafficMonitorPlugins-IpAddress_V1.00/Plugins/GeoIP/GeoIP.cpp:309)、[OnPluginCommand()](TrafficMonitorPlugins-IpAddress_V1.00/Plugins/GeoIP/GeoIP.cpp:324)、[IsCommandChecked()](TrafficMonitorPlugins-IpAddress_V1.00/Plugins/GeoIP/GeoIP.cpp:336)
  - 模式应用：[ApplyCommandMode()](TrafficMonitorPlugins-IpAddress_V1.00/Plugins/GeoIP/GeoIP.cpp:340)
- 鼠标提示（Tooltip）显示详细信息
  - 获取接口：[GetTooltipInfo()](TrafficMonitorPlugins-IpAddress_V1.00/Plugins/GeoIP/GeoIP.cpp:59)
- 周期刷新（默认 30 秒，可配置 15–600 秒）
  - 刷新入口：[DataRequired()](TrafficMonitorPlugins-IpAddress_V1.00/Plugins/GeoIP/GeoIP.cpp:226)
  - 间隔设置与判定：[SetIntervalSeconds()](TrafficMonitorPlugins-IpAddress_V1.00/Plugins/GeoIP/GeoIP.cpp:63)、[ShouldUpdate()](TrafficMonitorPlugins-IpAddress_V1.00/Plugins/GeoIP/GeoIP.cpp:77)
- 任务栏项目对接
  - 显示文本返回：[CGeoIPItem::GetItemValueText()](TrafficMonitorPlugins-IpAddress_V1.00/Plugins/GeoIP/GeoIPItem.cpp:18)
  - 示例文本（用于分配宽度）：[CGeoIPItem::GetItemValueSampleText()](TrafficMonitorPlugins-IpAddress_V1.00/Plugins/GeoIP/GeoIPItem.cpp:22)
- 插件导出入口（接口对象单例）
  - [TMPluginGetInstance()](TrafficMonitorPlugins-IpAddress_V1.00/Plugins/GeoIP/GeoIP.cpp:360)

---

## 兼容性与要求

- 平台：Windows 10/11 x64（Win32/ARM64EC 亦可构建）
- 编译器/IDE：Visual Studio 2022（v143 工具集）
- 框架/库：
  - MFC（使用 WinInet 进行 HTTP 访问）
  - 预编译头（PCH）已启用
  - 源文件编译选项启用了 `/utf-8`，避免中文导致的编码告警/错误
- 插件接口版本：API v6（参见 [ITMPlugin::GetAPIVersion()](TrafficMonitorPlugins-IpAddress_V1.00/include/PluginInterface.h:164)）

---

## 依赖与上游

- 插件接口与示例工程来自 TrafficMonitor 插件工程（MIT License）
  - 接口头文件：[PluginInterface.h](TrafficMonitorPlugins-IpAddress_V1.00/include/PluginInterface.h)
- 内部工具库（utilities）
  - INI 读写：[IniHelper.h](TrafficMonitorPlugins-IpAddress_V1.00/utilities/IniHelper.h)
  - JSON 解析：[JsonHelper.h](TrafficMonitorPlugins-IpAddress_V1.00/utilities/JsonHelper.h)、[yyjson](TrafficMonitorPlugins-IpAddress_V1.00/utilities/yyjson/yyjson.h)
  - 字符串工具与格式化：[Common.h](TrafficMonitorPlugins-IpAddress_V1.00/utilities/Common.h)
  - 变体包装：[Variant.h](TrafficMonitorPlugins-IpAddress_V1.00/utilities/Variant.h)

注意：为便于独立开源/构建，本仓库后续会内置最小必要的 utilities 子集或以 Git 子模块方式引用。当前阶段可先通过原有解决方案进行编译