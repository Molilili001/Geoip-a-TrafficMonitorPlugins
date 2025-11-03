#pragma once
#include <string>

// 轻量HTTP工具，参考Weather插件，实现基于WinInet的简单GET
class CGeoHttp
{
public:
    // 将const char*转宽字串
    static std::wstring StrToUnicode(const char* str, bool utf8 = false);

    // 将宽字串转const char*所需编码
    static std::string UnicodeToStr(const wchar_t* wstr, bool utf8 = false);

    // GET 指定URL，返回文本内容（ANSI或UTF8按网站而定）
    static bool GetURL(const std::wstring& url, std::string& result, const std::wstring& user_agent = std::wstring());
};