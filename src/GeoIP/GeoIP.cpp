#include "pch.h"
#include "GeoIP.h"
#include <windows.h>
#include "GeoIPItem.h"
#include "Common.h"
#include "../../utilities/IniHelper.h"
#include "../../utilities/JsonHelper.h"
#include "../../utilities/yyjson/yyjson.h"
#include "../../utilities/Variant.h"
#include "../../utilities/Common.h"
// 计算插件DLL所在目录（TrafficMonitor\\plugins\\）
static std::wstring GetPluginDir() {
    wchar_t path[MAX_PATH] = {0};
    GetModuleFileNameW(AfxGetInstanceHandle(), path, MAX_PATH);
    std::wstring full(path);
    size_t pos = full.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        return full.substr(0, pos + 1);
    }
    return L"";
}

// 若 plugins 目录下不存在默认配置，则创建标准默认配置
static void EnsureDefaultConfigInPluginDir(int default_interval, const std::wstring& default_template) {
    std::wstring cfg_path = GetPluginDir() + L"GeoIP.ini";
    std::string junk;
    if (!utilities::CCommon::GetFileContent(cfg_path.c_str(), junk)) {
        utilities::CIniHelper ini(cfg_path);
        ini.WriteInt(L"config", L"interval_sec", default_interval);
        ini.WriteString(L"config", L"display_template", default_template);
        ini.Save();
    }
}



CGeoIP CGeoIP::m_instance;

CGeoIP::CGeoIP() {
    m_item_ptr = new CGeoIPItem();
    m_interval_sec = 30;
    m_last_update = 0;
}

CGeoIP& CGeoIP::Instance() {
    return m_instance;
}

IPluginItem* CGeoIP::GetItem(int index) {
    switch (index) {
    case 0:
        return m_item_ptr;
    default:
        break;
    }
    return nullptr;
}

const wchar_t* CGeoIP::GetTooltipInfo() {
    return m_tooltip_info.c_str();
}

void CGeoIP::SetIntervalSeconds(int seconds) {
    if (seconds < 15) seconds = 15;
    if (seconds > 600) seconds = 600;
    m_interval_sec = seconds;
}

const GeoInfo& CGeoIP::GetGeoInfo() const {
    return m_geo;
}

const std::wstring& CGeoIP::GetDisplayText() const {
    return m_display_text;
}

bool CGeoIP::ShouldUpdate() const {
    time_t now = time(nullptr);
    if (m_last_update == 0) return true;
    return (now - m_last_update) >= m_interval_sec;
}

void CGeoIP::ComposeTexts() {
    // 优先使用模板，缺省模板为 "<%1%>·<%2%>" => <%1%>=countryCode, <%2%>=city
    std::wstring cc = m_geo.countryCode;
    std::wstring city = m_geo.city;
    utilities::StringHelper::StringNormalize(cc);
    utilities::StringHelper::StringNormalize(city);

    if (!m_display_template.empty()) {
        std::wstring text = utilities::StringHelper::StringFormat(m_display_template.c_str(), { utilities::CVariant(cc), utilities::CVariant(city), utilities::CVariant(m_geo.ip) });
        utilities::StringHelper::StringNormalize(text);
        // 如果模板导致多余的分隔符，进行简单清理
        if (cc.empty() && !city.empty()) {
            // 去掉可能的前缀分隔符
            utilities::StringHelper::StringReplace(text, L"·", L"");
            utilities::StringHelper::StringNormalize(text);
        }
        m_display_text = text;
    } else {
        // 回退到安全组合
        if (!cc.empty() && !city.empty()) {
            m_display_text = cc + L"·" + city;
        } else if (!cc.empty()) {
            m_display_text = cc;
        } else if (!city.empty()) {
            m_display_text = city;
        } else {
            m_display_text = L"";
        }
    }

    // 鼠标提示：详细
    wchar_t suspicious_str[16]{};
    swprintf_s(suspicious_str, L"%s", m_geo.suspicious ? L"Yes" : L"No");
    std::wstring tip;
    tip += L"IP: " + m_geo.ip + L"\n";
    tip += L"Country: " + m_geo.country + L" (" + m_geo.countryCode + L")\n";
    tip += L"City: " + m_geo.city + L"\n";
    tip += L"ISP: " + m_geo.isp + L"\n";
    tip += L"ASN: " + m_geo.asn + L"\n";
    tip += L"Suspicious(VPN/Proxy): " + std::wstring(suspicious_str) + L"\n";
    m_tooltip_info = tip;
}

static bool contains_any_icase(const std::wstring& hay, const std::initializer_list<const wchar_t*>& needles) {
    std::wstring lower = hay;
    for (auto& ch : lower) { ch = towlower(ch); }
    for (auto n : needles) {
        std::wstring nn = n;
        for (auto& ch : nn) { ch = towlower(ch); }
        if (lower.find(nn) != std::wstring::npos) return true;
    }
    return false;
}

bool CGeoIP::FetchViaIpApi(const std::wstring& ip_override) {
    // ip-api.com/json or ip-api.com/json/{ip}
    std::string body;
    std::wstring url;
    if (!ip_override.empty()) {
        // 当提供了明确的 IPv4 地址时，查询该地址的归属，避免因访问协议族（v6/v4）不同导致出口地址不一致
        url = L"http://ip-api.com/json/" + ip_override + L"?fields=status,country,countryCode,regionName,city,isp,org,as,query";
    } else {
        url = L"http://ip-api.com/json?fields=status,country,countryCode,regionName,city,isp,org,as,query";
    }
    url += L"&t=" + std::to_wstring(static_cast<unsigned long long>(time(nullptr)));
    if (!CGeoHttp::GetURL(url, body, L"TrafficMonitor/GeoIP")) return false;

    yyjson_read_err err{};
    yyjson_doc* doc = yyjson_read_opts((char*)body.c_str(), body.size(), 0, NULL, &err);
    if (!doc) return false;
    yyjson_val* root = yyjson_doc_get_root(doc);

    std::string status = utilities::JsonHelper::GetJsonString(root, "status");
    if (status != "success") {
        yyjson_doc_free(doc);
        return false;
    }

    m_geo.ip = utilities::StringHelper::StrToUnicode(utilities::JsonHelper::GetJsonString(root, "query").c_str(), true);
    m_geo.country = utilities::JsonHelper::GetJsonWString(root, "country");
    m_geo.countryCode = utilities::JsonHelper::GetJsonWString(root, "countryCode");
    m_geo.city = utilities::JsonHelper::GetJsonWString(root, "city");
    m_geo.isp = utilities::JsonHelper::GetJsonWString(root, "isp");

    // ASN: "ASXXXX SomeOrg"
    std::wstring as_str = utilities::JsonHelper::GetJsonWString(root, "as");
    m_geo.asn = as_str;

    yyjson_doc_free(doc);

    // 简易可疑判断
    m_geo.suspicious = contains_any_icase(m_geo.isp, {
        L"cloudflare", L"digitalocean", L"akhami", L"akamai", L"amazon", L"aws", L"alibaba",
        L"tencent", L"huawei", L"ovh", L"hetzner", L"leaseweb", L"linode", L"azure", L"gcp", L"google"
    }) || contains_any_icase(m_geo.asn, { L"cdn", L"cloud", L"vpn" });

    return true;
}

bool CGeoIP::FetchViaIpInfo() {
    // ipinfo.io/json
    std::string body;
    std::wstring url = L"http://ipinfo.io/json";
    url += L"?t=" + std::to_wstring(static_cast<unsigned long long>(time(nullptr)));
    if (!CGeoHttp::GetURL(url, body, L"TrafficMonitor/GeoIP")) return false;

    yyjson_doc* doc = yyjson_read((const char*)body.c_str(), body.size(), 0);
    if (!doc) return false;
    yyjson_val* root = yyjson_doc_get_root(doc);

    m_geo.ip = utilities::StringHelper::StrToUnicode(utilities::JsonHelper::GetJsonString(root, "ip").c_str(), true);
    m_geo.countryCode = utilities::JsonHelper::GetJsonWString(root, "country");
    m_geo.city = utilities::JsonHelper::GetJsonWString(root, "city");
    // ipinfo不直接给国家名，这里用国家代码作为国家名展示
    m_geo.country = m_geo.countryCode;
    std::wstring org = utilities::JsonHelper::GetJsonWString(root, "org"); // "ASXXXX OrgName"
    m_geo.asn = org;
    m_geo.isp = org;

    yyjson_doc_free(doc);

    m_geo.suspicious = contains_any_icase(m_geo.isp, {
        L"cloudflare", L"digitalocean", L"akamai", L"amazon", L"aws", L"alibaba",
        L"tencent", L"huawei", L"ovh", L"hetzner", L"leaseweb", L"linode", L"azure", L"gcp", L"google"
    }) || contains_any_icase(m_geo.asn, { L"cdn", L"cloud", L"vpn" });

    return true;
}

bool CGeoIP::FetchIPv4Address(std::wstring& out_ip) {
    // 仅获取 IPv4 出口地址，避免由于协议族不同（IPv6/IPv4）导致地理归属差异
    auto is_ipv4 = [](const std::wstring& s) -> bool {
        if (s.empty() || s.size() < 7 || s.size() > 15) return false;
        int dots = 0;
        for (wchar_t ch : s) {
            if (ch == L'.') dots++;
            else if (ch < L'0' || ch > L'9') return false;
        }
        return dots == 3;
    };
    auto try_url = [&](const std::wstring& url) -> bool {
        std::string body;
        if (!CGeoHttp::GetURL(url, body, L"TrafficMonitor/GeoIP")) return false;
        std::wstring s = CGeoHttp::StrToUnicode(body.c_str(), true);
        utilities::StringHelper::StringNormalize(s);
        // 去除常见的换行与空白
        utilities::StringHelper::StringReplace(s, L"\r", L"");
        utilities::StringHelper::StringReplace(s, L"\n", L"");
        utilities::StringHelper::StringNormalize(s);
        if (!is_ipv4(s)) return false;
        out_ip = s;
        return true;
    };

    // 依次尝试多个 IPv4-only 端点
    if (try_url(L"http://api-ipv4.ip.sb/ip") ||
        try_url(L"http://ipv4.icanhazip.com") ||
        try_url(L"http://v4.ident.me"))
    {
        return true;
    }
    return false;
}

bool CGeoIP::FetchViaPconline() {
    // whois.pconline.com.cn/ip.jsp 返回简体中文地区，如“广东省深圳市”
    std::string body;
    std::wstring url = L"http://whois.pconline.com.cn/ip.jsp";
    url += L"?t=" + std::to_wstring(static_cast<unsigned long long>(time(nullptr)));
    if (!CGeoHttp::GetURL(url, body, L"TrafficMonitor/GeoIP")) return false;

    // 去除可能的换行和空白
    // pconline 返回内容为中文，经过 GetURL 已统一转换为 UTF-8，这里必须按 UTF-8 解码
    std::wstring cn = CGeoHttp::StrToUnicode(body.c_str(), true);
    utilities::StringHelper::StringNormalize(cn);

    m_geo.countryCode = L"CN";
    m_geo.country = L"中国";
    m_geo.city = cn;
    m_geo.isp.clear();
    m_geo.asn.clear();
    m_geo.suspicious = false;
    return true;
}

void CGeoIP::DataRequired() {
    if (!ShouldUpdate()) return;

    bool ok = false;

    // 优先：当配置启用时，按 IPv4 出口地址进行地理查询，避免因浏览器/系统代理在 IPv6 上的不同路由导致结果差异
    if (m_prefer_ipv4) {
        std::wstring v4_ip;
        if (FetchIPv4Address(v4_ip)) {
            ok = FetchViaIpApi(v4_ip);
        }
    }

    // 其次：按默认路径获取（依赖数据源自动判定）
    if (!ok) ok = FetchViaIpApi();

    // 回退路径
    if (!ok) ok = FetchViaIpInfo();
    if (!ok) ok = FetchViaPconline();

    // 成功时才更新时间戳；失败时设置短回退重试窗口，避免长时间显示旧IP
    if (ok) {
        m_last_update = time(nullptr);
    } else {
        // 失败时约5秒后重试（不改变全局配置，仅调整时间判定）
        time_t now = time(nullptr);
        int retry_in = 5;
        if (retry_in >= m_interval_sec) retry_in = m_interval_sec - 1; // 保证小于间隔
        if (retry_in < 1) retry_in = 1;
        m_last_update = now - m_interval_sec + retry_in;
    }

    ComposeTexts();

    // 持久化刷新间隔（固定写到 plugins 目录）
    {
        utilities::CIniHelper ini(GetPluginDir() + L"GeoIP.ini");
        ini.WriteInt(L"config", L"interval_sec", m_interval_sec);
        ini.Save();
    }
}

// 选项对话框：当前占位返回未更改（未来可添加UI）
ITMPlugin::OptionReturn CGeoIP::ShowOptionsDialog(void* /*hParent*/) {
    return ITMPlugin::OR_OPTION_UNCHANGED;
}

const wchar_t* CGeoIP::GetInfo(PluginInfoIndex index) {
    static CString str;
    switch (index) {
    case TMI_NAME:
        return L"GeoIP";
    case TMI_DESCRIPTION:
        return L"显示通过梯子出口检测的国家与城市，并在提示中展示IP/ASN/ISP。";
    case TMI_AUTHOR:
        return L"Kilo Code";
    case TMI_COPYRIGHT:
        return L"Copyright (C) 2025";
    case ITMPlugin::TMI_URL:
        return L"https://ip-api.com/";
    case TMI_VERSION:
        return L"1.00";
    default:
        break;
    }
    return L"";
}

void CGeoIP::OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data) {
    switch (index) {
    case ITMPlugin::EI_CONFIG_DIR:
    {
        // 将配置固定在 plugins 目录，并在首次运行时生成默认标准配置
        m_config_dir = GetPluginDir();
        EnsureDefaultConfigInPluginDir(m_interval_sec, m_display_template);

        utilities::CIniHelper ini(m_config_dir + L"GeoIP.ini");
        int interval = ini.GetInt(L"config", L"interval_sec", m_interval_sec);
        SetIntervalSeconds(interval);

        // 读取模板到配置字段；命令模式为“遵照配置”时应用
        std::wstring tmpl = ini.GetString(L"config", L"display_template");
        if (!tmpl.empty()) {
            m_display_template_config = tmpl;
        } else {
            m_display_template_config = m_display_template; // 使用默认
        }

        // 读取命令模式（0=遵照配置，1=国家代码·城市，2=仅IP，3=国家代码·城市·IP）
        m_command_mode = ini.GetInt(L"config", L"command_mode", 0);
        // IPv4 优先策略（默认启用，以便与 ip.sb 的 v4 结果更一致）
        m_prefer_ipv4 = ini.GetBool(L"config", L"prefer_ipv4", true);
        
        // 应用命令模式到当前显示模板
        ApplyCommandMode();
    }
        break;
    default:
        break;
    }
}

// 插件右键命令实现
int CGeoIP::GetCommandCount() {
    return 4;
}

const wchar_t* CGeoIP::GetCommandName(int command_index) {
    switch (command_index) {
    case 0: return L"遵照配置显示";
    case 1: return L"显示 国家代码·城市";
    case 2: return L"仅显示 IP";
    case 3: return L"显示 国家代码·城市·IP";
    default: return L"";
    }
}

void* CGeoIP::GetCommandIcon(int /*command_index*/) {
    // 不提供自定义图标，使用默认
    return nullptr;
}

void CGeoIP::OnPluginCommand(int command_index, void* /*hWnd*/, void* /*para*/) {
    if (command_index < 0 || command_index >= GetCommandCount()) return;
    m_command_mode = command_index;
    ApplyCommandMode();
    // 立即更新显示文本
    ComposeTexts();
    // 将命令模式持久化到配置
    utilities::CIniHelper ini(m_config_dir + L"GeoIP.ini");
    ini.WriteInt(L"config", L"command_mode", m_command_mode);
    ini.Save();
}

int CGeoIP::IsCommandChecked(int command_index) {
    return (command_index == m_command_mode) ? 1 : 0;
}

void CGeoIP::ApplyCommandMode() {
    switch (m_command_mode) {
    case 0: // 遵照配置
        m_display_template = m_display_template_config;
        break;
    case 1: // 国家代码·城市
        m_display_template = L"<%1%>·<%2%>";
        break;
    case 2: // 仅IP
        m_display_template = L"<%3%>";
        break;
    case 3: // 国家代码·城市·IP
        m_display_template = L"<%1%>·<%2%>·<%3%>";
        break;
    default:
        m_display_template = m_display_template_config;
        break;
    }
}

ITMPlugin* TMPluginGetInstance()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    return &CGeoIP::Instance();
}