# Geoip-a-TrafficMonitorPlugins
一个用于 TrafficMonitor 的 IP 显示插件

---

# TrafficMonitor GeoIP 插件（独立开源仓库）

本仓库用于开源发布「TrafficMonitor GeoIP 插件」。插件可在任务栏显示通过梯子（VPN/代理）上网时的当前出口地区归属（国家代码·城市），并在鼠标提示中显示 IP / 国家 / 城市 / ISP / ASN / 可疑性（是否可能为云厂商/代理）等信息。

当前状态：仓库已包含插件源码、构建脚本、示例配置与 CI 工作流。可直接按构建指南进行编译与使用。

- 插件主实现参考并已迁移到本仓库：
  - 源码入口：[GeoIP.cpp](src/GeoIP/GeoIP.cpp:1)
  - 项目文件：[GeoIP.vcxproj](src/GeoIP/GeoIP.vcxproj:1)
  - 插件接口定义（MIT）：[PluginInterface.h](external/tm-plugin-sdk/include/PluginInterface.h:1)

---

## 功能特性

- 任务栏项目显示“国家代码·城市”，支持显示模板控制
  - 默认模板为 `<%1%>·<%2%>`（`%1 = countryCode`, `%2 = city`）
  - 模板组合逻辑：[CGeoIP::ComposeTexts()](src/GeoIP/GeoIP.cpp:83)
- 数据源级联回退（自动探测，尽力显示）
  - 1st：ip-api.com（JSON）→ [FetchViaIpApi()](src/GeoIP/GeoIP.cpp:137)
  - 2nd：ipinfo.io（JSON）→ [FetchViaIpInfo()](src/GeoIP/GeoIP.cpp:176)
  - 3rd：whois.pconline.com.cn（纯文本，中文地区）→ [FetchViaPconline()](src/GeoIP/GeoIP.cpp:206)
- 启发式“可疑性”标记（是否可能为云厂商/代理/VPN 出口）
  - 基于 ISP/ASN 字符串关键字匹配（cloudflare/akamai/aws/azure/gcp/ovh/hetzner/linode/leaseweb…）
  - 逻辑见上述两个数据源函数
- 右键菜单切换显示模式（立即生效并持久化）
  - 菜单项定义与实现：[GetCommandName()](src/GeoIP/GeoIP.cpp:309)、[OnPluginCommand()](src/GeoIP/GeoIP.cpp:324)、[IsCommandChecked()](src/GeoIP/GeoIP.cpp:336)
  - 模式应用：[ApplyCommandMode()](src/GeoIP/GeoIP.cpp:340)
- 鼠标提示（Tooltip）显示详细信息
  - 获取接口：[GetTooltipInfo()](src/GeoIP/GeoIP.cpp:59)
- 周期刷新（默认 30 秒，可配置 15–600 秒）
  - 刷新入口：[DataRequired()](src/GeoIP/GeoIP.cpp:226)
  - 间隔设置与判定：[SetIntervalSeconds()](src/GeoIP/GeoIP.cpp:63)、[ShouldUpdate()](src/GeoIP/GeoIP.cpp:77)
- 任务栏项目对接
  - 显示文本返回：[CGeoIPItem::GetItemValueText()](src/GeoIP/GeoIPItem.cpp:18)
  - 示例文本（用于分配宽度）：[CGeoIPItem::GetItemValueSampleText()](src/GeoIP/GeoIPItem.cpp:22)
- 插件导出入口（接口对象单例）
  - [TMPluginGetInstance()](src/GeoIP/GeoIP.cpp:360)

---

## 兼容性与要求

- 平台：Windows 10/11 x64（Win32/ARM64EC 亦可构建）
- 编译器/IDE：Visual Studio 2022（v143 工具集）
- 框架/库：
  - MFC（使用 WinInet 进行 HTTP 访问）
  - 预编译头（PCH）已启用
  - 源文件编译选项启用了 `/utf-8`，避免中文导致的编码告警/错误
- 插件接口版本：API v6（参见 [ITMPlugin::GetAPIVersion()](external/tm-plugin-sdk/include/PluginInterface.h:164)）

---

## 仓库结构与依赖

- 源码与项目：
  - [src/GeoIP/GeoIP.vcxproj](src/GeoIP/GeoIP.vcxproj:1)
  - [src/GeoIP/GeoIP.cpp](src/GeoIP/GeoIP.cpp:1)、[src/GeoIP/GeoIP.h](src/GeoIP/GeoIP.h:1)
  - [src/GeoIP/GeoIPItem.cpp](src/GeoIP/GeoIPItem.cpp:1)、[src/GeoIP/GeoIPItem.h](src/GeoIP/GeoIPItem.h:1)
  - [src/GeoIP/Common.cpp](src/GeoIP/Common.cpp:1)、[src/GeoIP/Common.h](src/GeoIP/Common.h:1)
  - [src/GeoIP/pch.h](src/GeoIP/pch.h:1)、[src/GeoIP/pch.cpp](src/GeoIP/pch.cpp:1)、[src/GeoIP/framework.h](src/GeoIP/framework.h:1)
- 外部依赖（内置最小子集）：
  - 插件接口头（MIT）：[external/tm-plugin-sdk/include/PluginInterface.h](external/tm-plugin-sdk/include/PluginInterface.h:1)
  - utilities 子集：
    - INI 读写：[IniHelper.h](external/utilities/IniHelper.h:1)、[IniHelper.cpp](external/utilities/IniHelper.cpp:1)
    - JSON 解析：[JsonHelper.h](external/utilities/JsonHelper.h:1)、[JsonHelper.cpp](external/utilities/JsonHelper.cpp:1)、[yyjson.h](external/utilities/yyjson/yyjson.h:1)、[yyjson.c](external/utilities/yyjson/yyjson.c:1)
    - 字符串工具与格式化：[Common.h](external/utilities/Common.h:1)、[Common.cpp](external/utilities/Common.cpp:1)
    - 变体包装：[Variant.h](external/utilities/Variant.h:1)、[Variant.cpp](external/utilities/Variant.cpp:1)

---

## 构建指南

完整说明见文档：[BUILD.md](docs/BUILD.md:1)

简要步骤：
1. 打开 Visual Studio 2022（v143 工具集），安装 MFC 与 Windows SDK。
2. 打开项目文件：[src/GeoIP/GeoIP.vcxproj](src/GeoIP/GeoIP.vcxproj:1)。
3. 选择“Release | x64”构建。
4. 构建后输出 DLL 位于：`$(SolutionDir)bin\x64\Release\GeoIP.dll`。

命令行（用于 CI 或本地批量构建）：
- `msbuild .\src\GeoIP\GeoIP.vcxproj /p:Configuration=Release /p:Platform=x64 /m`

---

## 部署与使用

1. 关闭 TrafficMonitor。
2. 将 `GeoIP.dll` 复制到 TrafficMonitor 安装目录的 `plugins` 文件夹，例如：
   - `TrafficMonitor\plugins\GeoIP.dll`
3. 启动 TrafficMonitor：
   - 在“插件管理”启用“GeoIP”。
   - 在“显示设置”勾选“GeoIP”并调整显示位置。
4. 开启梯子后，任务栏应显示形如 `US·Los Angeles`，鼠标提示包含 IP/ISP/ASN 等信息。

---

## 配置（GeoIP.ini）

示例文件位于：[samples/GeoIP.ini.sample](samples/GeoIP.ini.sample:1)

关键项：
- `interval_sec`（刷新秒数，15–600）
- `display_template`（显示模板，默认 `<%1%>·<%2%>`；`%1=countryCode`、`%2=city`、`%3=ip`）

插件在扩展信息 EI_CONFIG_DIR 设置时读取配置：
- 主程序扩展接口 [ITMPlugin::OnExtenedInfo()](external/tm-plugin-sdk/include/PluginInterface.h:274)
- 插件处理逻辑：[CGeoIP::OnExtenedInfo()](src/GeoIP/GeoIP.cpp:272)

---

## CI 自动化

已添加 GitHub Actions 工作流（Windows x64 Release 构建并上传工件）：
- [build.yml](.github/workflows/build.yml:1)

触发条件：任意分支 push/pull_request 均会构建并上传 GeoIP.dll 工件。

---

## 常见问题与排障

- 编译报错：MFC/Windows SDK 未安装
  - 解决：VS 安装管理器添加组件；确保使用 v143 工具集
- 预编译头/PCH 问题：
  - 说明：在 [pch.cpp](src/GeoIP/pch.cpp:1) 生效，并在项目设置标记为 Create（见 [GeoIP.vcxproj](src/GeoIP/GeoIP.vcxproj:265)）
- 中文编码告警（C4819/C4828）：
  - 说明：各配置启用了 `/utf-8`（参考 [GeoIP.vcxproj](src/GeoIP/GeoIP.vcxproj:205)）
- 网络不可达导致显示为空：
  - 检查出站策略与梯子路由；手动访问数据源 URL 验证连通性
- 提示可疑性为 Yes：
  - 启发式匹配 ISP/ASN 关键字（cloudflare/akamai/aws/azure/gcp/ovh/hetzner/linode/leaseweb...），逻辑见：
    - [FetchViaIpApi()](src/GeoIP/GeoIP.cpp:137)
    - [FetchViaIpInfo()](src/GeoIP/GeoIP.cpp:176)

---

## 许可与致谢

- 本仓库使用 MIT 许可：[LICENSE](LICENSE:1)
- 插件接口头（上游 MIT）：[PluginInterface.h](external/tm-plugin-sdk/include/PluginInterface.h:1)
- 数据源：遵守各服务的使用条款（ToS）
- 致谢：TrafficMonitor 项目与文档
