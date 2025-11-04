#pragma once
#include "PluginInterface.h"
#include <string>
#include <ctime>

class CGeoIPItem;

struct GeoInfo {
    std::wstring ip;
    std::wstring country;
    std::wstring countryCode;
    std::wstring city;
    std::wstring isp;
    std::wstring asn;
    bool suspicious{false};
};

class CGeoIP : public ITMPlugin {
private:
    CGeoIP();

public:
    static CGeoIP& Instance();

    // ITMPlugin overrides
    virtual IPluginItem* GetItem(int index) override;
    virtual const wchar_t* GetTooltipInfo() override;
    virtual void DataRequired() override;
    virtual OptionReturn ShowOptionsDialog(void* hParent) override;
    virtual const wchar_t* GetInfo(PluginInfoIndex index) override;
    virtual void OnExtenedInfo(ExtendedInfoIndex index, const wchar_t* data) override;

    // Plugin commands for right-click menu control
    virtual int GetCommandCount() override;
    virtual const wchar_t* GetCommandName(int command_index) override;
    virtual void* GetCommandIcon(int command_index) override;
    virtual void OnPluginCommand(int command_index, void* hWnd, void* para) override;
    virtual int IsCommandChecked(int command_index) override;

    // Config and accessors
    void SetIntervalSeconds(int seconds);
    const GeoInfo& GetGeoInfo() const;
    const std::wstring& GetDisplayText() const;

private:
    bool ShouldUpdate() const;
    void ComposeTexts();
    void ApplyCommandMode();

    // Data sources
    bool FetchViaIpApi(const std::wstring& ip_override = L""); // ip-api.com/json or json/{ip}
    bool FetchViaIpInfo();        // ipinfo.io/json
    bool FetchViaPconline();      // whois.pconline.com.cn/ip.jsp (CN fallback)
    bool FetchIPv4Address(std::wstring& out_ip); // ipv4-only address (icanhazip/ident.me)
    bool FetchIPv6Address(std::wstring& out_ip); // ipv6-only address (ip.sb/icanhazip/ident.me)

    static CGeoIP m_instance;
    CGeoIPItem* m_item_ptr{nullptr};

    int m_interval_sec{30};
    time_t m_last_update{0};

    GeoInfo m_geo{};
    std::wstring m_display_text;
    std::wstring m_tooltip_info;

    // template: "<%1%>·<%2%>" => <%1%>=countryCode, <%2%>=city
    // m_display_template: current template applied for display
    std::wstring m_display_template{L"<%1%>·<%2%>"};
    // m_display_template_config: template loaded from GeoIP.ini
    std::wstring m_display_template_config{L"<%1%>·<%2%>"};
    // command mode: 0=遵照配置, 1=国家代码·城市, 2=仅IP, 3=国家代码·城市·IP
    int m_command_mode{0};
    // prefer IPv4 path when resolving exit IP
    bool m_prefer_ipv4{false};
    // prefer IPv6 path when resolving exit IP
    bool m_prefer_ipv6{false};
    
    // config directory (from EI_CONFIG_DIR)
    std::wstring m_config_dir;
};

#ifdef __cplusplus
extern "C" {
#endif
__declspec(dllexport) ITMPlugin* TMPluginGetInstance();
#ifdef __cplusplus
}
#endif