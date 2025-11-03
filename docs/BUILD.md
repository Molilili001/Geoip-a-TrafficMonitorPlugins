# 构建指南（TrafficMonitor GeoIP 插件）

本指南说明如何在 Windows 上使用 Visual Studio 2022 构建本仓库中的 GeoIP 插件，并将其部署到 TrafficMonitor。请确保已安装包含「使用 C++ 的桌面开发」工作负载的 VS2022，启用 MFC、Windows 10/11 SDK、MSBuild 和 v143 工具集。

- 插件工程入口：[GeoIP.vcxproj](GeoIP-Plugin-GitHub/src/GeoIP/GeoIP.vcxproj:1)
- 插件主类与入口导出：
  - [CGeoIP::DataRequired()](GeoIP-Plugin-GitHub/src/GeoIP/GeoIP.cpp:226)
  - [TMPluginGetInstance()](GeoIP-Plugin-GitHub/src/GeoIP/GeoIP.cpp:360)
- 插件接口头（MIT）：[PluginInterface.h](GeoIP-Plugin-GitHub/external/tm-plugin-sdk/include/PluginInterface.h:1)
- 内部工具库（已内置最小子集）：
  - INI：[IniHelper.h](GeoIP-Plugin-GitHub/external/utilities/IniHelper.h:1)
  - JSON：[JsonHelper.h](GeoIP-Plugin-GitHub/external/utilities/JsonHelper.h:1)、[yyjson.h](GeoIP-Plugin-GitHub/external/utilities/yyjson/yyjson.h:1)
  - 字符串与格式化：[Common.h](GeoIP-Plugin-GitHub/external/utilities/Common.h:1)
  - 变体包装：[Variant.h](GeoIP-Plugin-GitHub/external/utilities/Variant.h:1)

— — —

一、环境准备
- 操作系统：Windows 10/11 x64（支持 Win32/ARM64EC 编译）
- 开发工具：Visual Studio 2022（v143），安装组件包括：
  - 使用 C++ 的桌面开发、MSBuild、MFC、Windows 10/11 SDK
- 网络：能访问 `ip-api.com`、`ipinfo.io`、`whois.pconline.com.cn`（用于运行时检测出口地理信息）
- 可选：TrafficMonitor 主程序（用于部署验证）

— — —

二、仓库结构与包含路径
- 源码与项目：
  - [src/GeoIP/GeoIP.vcxproj](GeoIP-Plugin-GitHub/src/GeoIP/GeoIP.vcxproj:1)
  - [src/GeoIP/GeoIP.cpp](GeoIP-Plugin-GitHub/src/GeoIP/GeoIP.cpp:1)、[src/GeoIP/GeoIP.h](GeoIP-Plugin-GitHub/src/GeoIP/GeoIP.h:1)
  - [src/GeoIP/GeoIPItem.cpp](GeoIP-Plugin-GitHub/src/GeoIP/GeoIPItem.cpp:1)、[src/GeoIP/GeoIPItem.h](GeoIP-Plugin-GitHub/src/GeoIP/GeoIPItem.h:1)
  - [src/GeoIP/Common.cpp](GeoIP-Plugin-GitHub/src/GeoIP/Common.cpp:1)、[src/GeoIP/Common.h](GeoIP-Plugin-GitHub/src/GeoIP/Common.h:1)
  - [src/GeoIP/pch.h](GeoIP-Plugin-GitHub/src/GeoIP/pch.h:1)、[src/GeoIP/pch.cpp](GeoIP-Plugin-GitHub/src/GeoIP/pch.cpp:1)、[src/GeoIP/framework.h](GeoIP-Plugin-GitHub/src/GeoIP/framework.h:1)
- 外部依赖（已内置）：
  - [external/tm-plugin-sdk/include/PluginInterface.h](GeoIP-Plugin-GitHub/external/tm-plugin-sdk/include/PluginInterface.h:1)
  - [external/utilities/*](GeoIP-Plugin-GitHub/external/utilities/Common.h:1)
- 项目包含路径已在 [GeoIP.vcxproj](GeoIP-Plugin-GitHub/src/GeoIP/GeoIP.vcxproj:110) 配置为：
  - `$(SolutionDir)external\utilities`
  - `$(SolutionDir)external\tm-plugin-sdk\include`
  - `$(ProjectDir)`
- 编译选项：
  - 预编译头生效项：[pch.cpp](GeoIP-Plugin-GitHub/src/GeoIP/GeoIP.vcxproj:265)
  - UTF-8 编译开关：`/utf-8` 已配置于各构建配置的 [ClCompile.AdditionalOptions](GeoIP-Plugin-GitHub/src/GeoIP/GeoIP.vcxproj:205)

— — —

三、使用 Visual Studio 构建
- 打开：双击 [src/GeoIP/GeoIP.vcxproj](GeoIP-Plugin-GitHub/src/GeoIP/GeoIP.vcxproj:1) 用 VS2022 打开工程
- 选择：
  - 解决方案配置：Release
  - 解决方案平台：x64（可选 Win32/ARM64EC）
- 生成：菜单「生成」→「生成 GeoIP」
- 输出位置（参考 [OutDir](GeoIP-Plugin-GitHub/src/GeoIP/GeoIP.vcxproj:118)）：
  - `$(SolutionDir)bin\x64\Release\GeoIP.dll`
- 若因 MFC/SDK 缺失编译失败，请在 VS 安装管理器勾选 MFC 与 Windows SDK 组件后重试

— — —

四、使用 MSBuild 命令行构建（可用于 CI）
- 打开「开发者命令提示符 for VS 2022」或 PowerShell
- 切换到仓库根目录：`cd GeoIP-Plugin-GitHub`
- 执行命令（示例：Release|x64）：
  - `msbuild .\src\GeoIP\GeoIP.vcxproj /p:Configuration=Release /p:Platform=x64 /m`
- 成功后输出 DLL 在：
  - `.\bin\x64\Release\GeoIP.dll`

— — —

五、部署到 TrafficMonitor
- 关闭 TrafficMonitor
- 拷贝插件 DLL 至主程序 `plugins`：
  - `TrafficMonitor\plugins\GeoIP.dll`
- 启动 TrafficMonitor：
  - 在「插件管理」启用“GeoIP”
  - 在「显示设置」勾选“GeoIP”并调整显示位置
- 运行验证：
  - 任务栏显示（默认模板）：`CC·City`（如 `US·Los Angeles`）
  - 鼠标提示包含：IP / Country(CountryCode) / City / ISP / ASN / Suspicious

— — —

六、配置（samples/GeoIP.ini.sample）
- 配置示例文件：[samples/GeoIP.ini.sample](GeoIP-Plugin-GitHub/samples/GeoIP.ini.sample:1)
- 插件在扩展信息 EI_CONFIG_DIR 设置时读取配置：
  - [ITMPlugin::OnExtenedInfo()](GeoIP-Plugin-GitHub/external/tm-plugin-sdk/include/PluginInterface.h:274)
  - 插件处理逻辑：[CGeoIP::OnExtenedInfo()](GeoIP-Plugin-GitHub/src/GeoIP/GeoIP.cpp:272)
- 关键项：
  - `interval_sec`（刷新秒数，15–600）
  - `display_template`（显示模板，默认 `<%1%>·<%2%>`；`%1=countryCode`、`%2=city`、`%3=ip`）

— — —

七、常见问题与排障
- 编译报错：MFC/Windows SDK 未安装
  - 解决：VS 安装管理器添加组件；确保使用 v143 工具集
- 预编译头/PCH 问题：
  - 说明：已在 [pch.cpp](GeoIP-Plugin-GitHub/src/GeoIP/pch.cpp:1) 生效，并在项目设置标记为 Create
- 中文编码告警（C4819/C4828）：
  - 说明：各配置启用了 `/utf-8`（参考 [GeoIP.vcxproj](GeoIP-Plugin-GitHub/src/GeoIP/GeoIP.vcxproj:205)）
- 网络不可达导致显示为空：
  - 检查出站策略与梯子路由；手动访问数据源 URL 验证连通性
- 提示可疑性为 Yes：
  - 启发式匹配 ISP/ASN 关键字（cloudflare/akamai/aws/azure/gcp/ovh/hetzner/linode/leaseweb...），逻辑见：
    - [FetchViaIpApi()](GeoIP-Plugin-GitHub/src/GeoIP/GeoIP.cpp:137)
    - [FetchViaIpInfo()](GeoIP-Plugin-GitHub/src/GeoIP/GeoIP.cpp:176)

— — —

八、许可与致谢
- 本仓库使用 MIT 许可：[LICENSE](GeoIP-Plugin-GitHub/LICENSE:1)
- 插件接口头（上游 MIT）：[PluginInterface.h](GeoIP-Plugin-GitHub/external/tm-plugin-sdk/include/PluginInterface.h:1)
- 数据源：遵守各服务的使用条款（ToS）
- 致谢：TrafficMonitor 项目与文档

— — —

九、后续计划（开源仓库）
- 添加 CI（GitHub Actions）自动构建 Release|x64 并产出 GeoIP.dll
- 增强启发式与字典，改进 VPN/代理识别精度
- 补充贡献指南与代码规范、PR 模板

— — —

十、验证清单（发布前）
- 构建：Release|x64 成功，输出 DLL 正确
- 部署：拷贝至 TrafficMonitor/plugins 后可启用
- 配置：读取并应用 `interval_sec`、`display_template`
- UI：任务栏文本格式符合预期，Tooltip 信息完整