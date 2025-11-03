# GeoIP 插件联调测试（Release|x64 + PluginTester）

目标：在 Release|x64 构建后，将 GeoIP.dll 放入 PluginTester 同目录，验证任务栏文本组合 [CGeoIP::ComposeTexts()](../GeoIP/GeoIP.cpp:56) 与数据源回退 [FetchViaIpApi()](../GeoIP/GeoIP.cpp:110) / [FetchViaIpInfo()](../GeoIP/GeoIP.cpp:148) / [FetchViaPconline()](../GeoIP/GeoIP.cpp:177)。

## 一、构建 Release|x64

1. 打开解决方案：
   - [TrafficMonitorPlugins.sln](../../TrafficMonitorPlugins.sln:1)

2. 在 Visual Studio 选择：
   - 解决方案配置：Release
   - 解决方案平台：x64

3. 生成 GeoIP 项目（或生成整个解决方案）：
   - 输出位置（OutDir）参考 [GeoIP.vcxproj](../GeoIP/GeoIP.vcxproj:132)：
     - `$(SolutionDir)bin\x64\Release\`
   - 重要设置（已配置好）：
     - 预编译头：在 [pch.cpp 项](../GeoIP/GeoIP.vcxproj:265) 设置为 Create
     - UTF-8 编译：各配置 [AdditionalOptions=/utf-8](../GeoIP/GeoIP.vcxproj:205)

4. 构建成功后，确认生成：
   - `$(SolutionDir)bin\x64\Release\GeoIP.dll`

## 二、放入 PluginTester 并运行

1. 找到 PluginTester 可执行文件输出目录（通常同一 OutDir）：
   - `$(SolutionDir)bin\x64\Release\PluginTester.exe`（若构建了 PluginTester 工程）

2. 将 `GeoIP.dll` 复制到 PluginTester.exe 所在目录（同级）：
   - `$(SolutionDir)bin\x64\Release\GeoIP.dll`

3. 运行 PluginTester：
   - 双击 `PluginTester.exe` 或在 VS 以 Release|x64 启动

4. 在 PluginTester 中启用“GeoIP”插件项，观察 UI：
   - 任务栏（或主窗口）显示项目文本来自 [CGeoIPItem::GetItemValueText()](../GeoIP/GeoIPItem.cpp:18)
   - 文本格式由 [CGeoIP::ComposeTexts()](../GeoIP/GeoIP.cpp:56) 生成，默认模板 [m_display_template](../GeoIP/GeoIP.h:57) 为 `<%1%>·<%2%>`（`%1=countryCode`, `%2=city`）
   - 示例文本为 [CGeoIPItem::GetItemValueSampleText()](../GeoIP/GeoIPItem.cpp:22) ：`US·Los Angeles`

## 三、功能验证用例

### 用例 1：基本数据获取（ip-api 成功）
- 条件：可访问 `http://ip-api.com/json`
- 期望：
  - [FetchViaIpApi()](../GeoIP/GeoIP.cpp:110) 返回 success
  - Display 文本为 `CC·City`（例如 `US·Los Angeles`）
  - 鼠标提示 [CGeoIP::GetTooltipInfo()](../GeoIP/GeoIP.cpp:32) 展示：
    - IP/Country(CountryCode)/City/ISP/ASN/Suspicious
- 步骤：
  1. 启动 PluginTester
  2. 启用 GeoIP 项目并等待最多 30s（默认刷新间隔 [m_interval_sec](../GeoIP/GeoIP.h:50) = 30）
  3. 验证文本与 Tooltip

### 用例 2：ip-api 失败回退 ipinfo
- 条件：临时阻断或不可访问 `ip-api.com`，但可访问 `http://ipinfo.io/json`
- 期望：
  - [FetchViaIpApi()](../GeoIP/GeoIP.cpp:110) 返回 false
  - [FetchViaIpInfo()](../GeoIP/GeoIP.cpp:148) 成功；Country 使用 `countryCode`；ISP/ASN 取 `org` 字段
  - Display 文本改为 `CC·City`（CC 可能是 `US/CN/…`）
- 步骤：
  1.（可选）在系统 Hosts/防火墙临时阻断 ip-api
  2. 重启 PluginTester 或等待刷新间隔
  3. 验证文本与 Tooltip 来源变化（ISP/ASN 取 `org`）

### 用例 3：最终回退 pconline（仅中文地区）
- 条件：ip-api 和 ipinfo 均不可访问，但可访问 `http://whois.pconline.com.cn/ip.jsp`
- 期望：
  - [FetchViaPconline()](../GeoIP/GeoIP.cpp:177) 成功；`countryCode=CN`，`country=中国`，`city` 为中文地区文本（如“广东省深圳市”）
  - Display 文本为 `CN·{中文地区}`
- 步骤：
  1. 阻断 ip-api/ipinfo，仅允许 pconline
  2. 等待刷新或重启 PluginTester
  3. 验证文本

### 用例 4：模板与规范化
- 条件：设置自定义模板（通过配置文件）
- 期望：
  - 模板替换调用 [StringHelper::StringFormat()](../../utilities/Common.h:35)（参数类型为 [CVariant](../../utilities/Variant.h:6)）
  - 文本规范化 [StringHelper::StringNormalize()](../../utilities/Common.h:37) 生效（去除前后空白）
- 步骤：
  1. 在配置目录（见下一节）创建 `GeoIP.ini`，设置 `display_template=<%2%>(<%1%>)`
  2. 重启 PluginTester 或等待刷新
  3. 验证显示为 `City(CC)`（如 `Los Angeles(US)`）

### 用例 5：刷新间隔
- 条件：设置 `interval_sec=15`
- 期望：
  - [CGeoIP::SetIntervalSeconds()](../GeoIP/GeoIP.cpp:36) 应用并受限于 15–600 范围
  - [CGeoIP::ShouldUpdate()](../GeoIP/GeoIP.cpp:50) 判断间隔生效
- 步骤：
  1. `GeoIP.ini` 写入 `interval_sec=15`
  2. 观察 15s 内触发更新

### 用例 6：可疑性启发式
- 条件：ISP/ASN 命中关键字（cloudflare/akamai/aws/azure/gcp/ovh/hetzner/linode/leaseweb 等）
- 期望：
  - [m_geo.suspicious](../GeoIP/GeoIP.h:15) 置为 true
  - Tooltip 中 `Suspicious(VPN/Proxy): Yes`
- 步骤：
  1. 使用命中关键字的出口（或手动构造测试数据）
  2. 验证 Tooltip 文本

## 四、配置文件（GeoIP.ini）位置与示例

1. 插件在获得扩展信息 `EI_CONFIG_DIR` 时会记录配置目录并读取：
   - 主程序扩展接口 [ITMPlugin::OnExtenedInfo()](../../include/PluginInterface.h:274)
   - 插件处理逻辑 [CGeoIP::OnExtenedInfo()](../GeoIP/GeoIP.cpp:242)

2. 在该目录创建 `GeoIP.ini`，示例：

```
[config]
interval_sec=30
display_template=<%1%>·<%2%>
```

3. 插件会在 [CGeoIP::DataRequired()](../GeoIP/GeoIP.cpp:196) 写回 `interval_sec`（以确保持久化），模板读取在 [OnExtenedInfo()](../GeoIP/GeoIP.cpp:248)。

## 五、诊断与排障

- 若显示为空或 Tooltip 不更新：
  - 检查网络访问权限与梯子路由策略
  - 手动访问数据源 URL 验证连通性
  - 等待刷新间隔或重启 PluginTester

- 编码与 PCH 问题（已预防）：
  - 项目启用 `/utf-8` 编译选项（各配置 [GeoIP.vcxproj](../GeoIP/GeoIP.vcxproj:205)）
  - 预编译头生成项：[GeoIP.vcxproj](../GeoIP/GeoIP.vcxproj:265)

## 六、测试完成后的部署

1. 关闭 TrafficMonitor
2. 将 `GeoIP.dll` 复制到 TrafficMonitor 安装目录的 `plugins` 文件夹：
   - 例如：`TrafficMonitor\plugins\GeoIP.dll`
3. 启动 TrafficMonitor：
   - 在“插件管理”启用“GeoIP”
   - 在“显示设置”勾选“GeoIP”显示项目
4. 开启梯子后验证任务栏文本与 Tooltip，如：
   - 文本：`US·Los Angeles`
   - Tooltip：包含 IP/Country/City/ISP/ASN/Suspicious

## 七、参考索引（点击跳转）

- 插件接口定义：[PluginInterface.h](../../include/PluginInterface.h:1)
- 插件类与方法：
  - 实例导出 [TMPluginGetInstance()](../GeoIP/GeoIP.cpp:260)
  - 数据获取入口 [ITMPlugin::DataRequired()](../../include/PluginInterface.h:179) 与实现 [GeoIP.cpp](../GeoIP/GeoIP.cpp:196)
  - 模板组合 [ComposeTexts()](../GeoIP/GeoIP.cpp:56)
  - 数据源 [FetchViaIpApi()](../GeoIP/GeoIP.cpp:110) / [FetchViaIpInfo()](../GeoIP/GeoIP.cpp:148) / [FetchViaPconline()](../GeoIP/GeoIP.cpp:177)
- 工具库：
  - 字符串工具与格式化 [Common.h](../../utilities/Common.h:24) / [StringHelper::StringFormat()](../../utilities/Common.h:35)
  - INI 读写 [IniHelper.h](../../utilities/IniHelper.h:11)
  - JSON 解析 [JsonHelper.h](../../utilities/JsonHelper.h:7)
  - 变体包装 [Variant.h](../../utilities/Variant.h:6)