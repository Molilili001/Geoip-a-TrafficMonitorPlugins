# GeoIP 插件使用与部署指南

本插件用于自动检测在使用梯子（VPN/代理）上网时的当前出口地区归属，并在任务栏显示“国家代码·城市”，鼠标提示显示 IP/国家/城市/ISP/ASN 等信息。构建、联调与部署步骤如下。

## 构建

1. 在 Visual Studio 选择“解决方案配置”为 Release，“解决方案平台”为 x64。
2. 生成 GeoIP 项目，输出将位于：
   - OutDir 配置参考 [GeoIP.vcxproj](../GeoIP/GeoIP.vcxproj:132)；通常是 `$(SolutionDir)bin\x64\Release\`。
3. 已设置预编译头与 UTF-8 编译参数，避免中文编码导致的 PCH 生成失败：
   - pch 源项设置参考 [GeoIP.vcxproj](../GeoIP/GeoIP.vcxproj:265)
   - UTF-8 编译参数参考 [GeoIP.vcxproj](../GeoIP/GeoIP.vcxproj:205)

## 联调（PluginTester）

如果你使用解决方案中的 PluginTester：
- 将生成的 `GeoIP.dll` 拷贝到 PluginTester 可执行文件所在输出目录（通常也是 `$(SolutionDir)bin\x64\Release\`）。
- 运行 PluginTester，验证以下行为：
  - 周期刷新触发于 [ITMPlugin::DataRequired()](../../include/PluginInterface.h:179)；本插件实现参考 [GeoIP.cpp](./GeoIP.cpp:196)。
  - 显示文本模板组合逻辑参考 [CGeoIP::ComposeTexts()](./GeoIP.cpp:56)，模板替换调用 [StringHelper::StringFormat()](../../utilities/Common.h:35)。
  - 任务栏项目文本返回参考 [CGeoIPItem::GetItemValueText()](./GeoIPItem.cpp:18)。
  - 鼠标提示返回参考 [CGeoIP::GetTooltipInfo()](./GeoIP.cpp:32)。

## 部署到 TrafficMonitor

1. 关闭 TrafficMonitor。
2. 将 `GeoIP.dll` 复制到 TrafficMonitor 安装目录下的 `plugins` 文件夹，例如：
   - `TrafficMonitor\plugins\GeoIP.dll`
3. 启动 TrafficMonitor：
   - 在“插件管理”中启用“GeoIP”。
   - 在“显示设置”中勾选“GeoIP”项目，配置显示位置。
4. 开启梯子后，任务栏应显示形如 `US·Los Angeles`，鼠标提示包含 IP/ISP/ASN 等信息。

## 配置（GeoIP.ini）

插件会在主程序启动时通过扩展信息接口传入配置路径并读取 `GeoIP.ini`：
- 配置目录传入点参考 [ITMPlugin::OnExtenedInfo()](../../include/PluginInterface.h:274)，本插件处理逻辑参考 [CGeoIP::OnExtenedInfo()](./GeoIP.cpp:242)。

在该目录下创建或编辑 `GeoIP.ini`，示例如下：

```
[config]
; 刷新间隔，单位秒。范围 15–600，默认 30
interval_sec=30

; 显示模板，<%1%>=countryCode，<%2%>=city
display_template=<%1%>·<%2%>

; 预留项：数据源优先级（尚未实现完全可配）
; source_order=ip-api,ipinfo,pconline

; 预留项：适配器绑定（实验性）
; bind_adapter=false
```

说明：
- 读取写入参考 [CGeoIP::DataRequired()](./GeoIP.cpp:196) 中使用的 [utilities::CIniHelper](../../utilities/IniHelper.h:11)。
- 模板格式化调用 [utilities::StringHelper::StringFormat()](../../utilities/Common.h:35)，参数使用 [utilities::CVariant](../../utilities/Variant.h:6)。

## 数据源与回退策略

- 主数据源：`ip-api.com/json`
  - 获取逻辑参考 [CGeoIP::FetchViaIpApi()](./GeoIP.cpp:110)
- 回退数据源：`ipinfo.io/json`
  - 获取逻辑参考 [CGeoIP::FetchViaIpInfo()](./GeoIP.cpp:148)
- 最终回退（仅国家/城市，中文）：`whois.pconline.com.cn/ip.jsp`
  - 获取逻辑参考 [CGeoIP::FetchViaPconline()](./GeoIP.cpp:177)

周期刷新与判定逻辑：
- 刷新间隔控制参考 [CGeoIP::SetIntervalSeconds()](./GeoIP.cpp:36) 与 [CGeoIP::ShouldUpdate()](./GeoIP.cpp:50)。

## VPN/代理可疑性启发式

- 简易启发式基于 ISP/ASN 字符串关键字匹配，参考 [CGeoIP::FetchViaIpApi()](./GeoIP.cpp:139) 与 [CGeoIP::FetchViaIpInfo()](./GeoIP.cpp:169)。
- 提示文本整合参考 [CGeoIP::ComposeTexts()](./GeoIP.cpp:86)。

后续计划：
- 扩充 ASN/ISP 词典、自治域变更检测、频繁位置变化提示。

## 常见问题与排障

- PCH 生成失败或中文编码警告（C4819/C4828）：
  - 现已在项目中启用 UTF-8 编译选项，参考 [GeoIP.vcxproj](../GeoIP/GeoIP.vcxproj:205)。
- `.NET Framework 4.7.2` 目标框架缺失：
  - 这是解决方案其他项目的要求，与 GeoIP 原生 C++ 插件无关。若生成解决方案整体失败，请按 VS 提示安装对应 Developer Pack 或重定向至 4.8。
- 无法访问数据源：
  - 检查本机或梯子出站策略是否允许访问 `ip-api.com`/`ipinfo.io`/`pconline.com.cn`。
  - 插件使用 WinInet 会走系统默认路由；实验性“适配器绑定”尚未启用，未来将允许绑定到当前监控的网络连接。

## 文件与接口参考索引（点击跳转）

- 插件接口定义：[PluginInterface.h](../../include/PluginInterface.h:1)
- 插件主类：
  - 获取实例导出：[TMPluginGetInstance()](./GeoIP.cpp:260)
  - 选项对话框占位返回：[ITMPlugin::ShowOptionsDialog()](../../include/PluginInterface.h:196) 与实现 [GeoIP.cpp](./GeoIP.cpp:216)
- 显示项类：
  - 名称与唯一 ID：[CGeoIPItem::GetItemName()](./GeoIPItem.cpp:5)、[CGeoIPItem::GetItemId()](./GeoIPItem.cpp:9)
  - 显示文本返回：[CGeoIPItem::GetItemValueText()](./GeoIPItem.cpp:18)
- 工具库：
  - JSON 帮助：[JsonHelper.h](../../utilities/JsonHelper.h:1)
  - INI 帮助：[IniHelper.h](../../utilities/IniHelper.h:1)
  - 字符串工具与格式化：[Common.h](../../utilities/Common.h:24)、[StringHelper::StringFormat()](../../utilities/Common.h:35)
- 预编译头：
  - [pch.h](./pch.h:1)、[pch.cpp](./pch.cpp:1)、项目项设置参考 [GeoIP.vcxproj](../GeoIP/GeoIP.vcxproj:265)

## 许可与致谢

- 使用的第三方数据源请遵守各自服务的使用条款（ToS）。
- 插件接口与示例参考 TrafficMonitor 作者文档与工程。