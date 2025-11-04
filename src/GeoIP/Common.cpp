#include "pch.h"
#include "Common.h"
#include <afxinet.h>
#include <wininet.h>

std::wstring CGeoHttp::StrToUnicode(const char* str, bool utf8)
{
    if (str == nullptr)
        return std::wstring();
    std::wstring result;
    int size;
    size = MultiByteToWideChar((utf8 ? CP_UTF8 : CP_ACP), 0, str, -1, NULL, 0);
    if (size <= 0) return std::wstring();
    wchar_t* str_unicode = new wchar_t[size + 1];
    MultiByteToWideChar((utf8 ? CP_UTF8 : CP_ACP), 0, str, -1, str_unicode, size);
    result.assign(str_unicode);
    delete[] str_unicode;
    return result;
}

std::string CGeoHttp::UnicodeToStr(const wchar_t* wstr, bool utf8)
{
    if (wstr == nullptr)
        return std::string();
    std::string result;
    int size{ 0 };
    size = WideCharToMultiByte((utf8 ? CP_UTF8 : CP_ACP), 0, wstr, -1, NULL, 0, NULL, NULL);
    if (size <= 0) return std::string();
    char* str = new char[size + 1];
    WideCharToMultiByte((utf8 ? CP_UTF8 : CP_ACP), 0, wstr, -1, str, size, NULL, NULL);
    result.assign(str);
    delete[] str;
    return result;
}

bool CGeoHttp::GetURL(const std::wstring& url, std::string& result, const std::wstring& user_agent)
{
    bool succeed{ false };
    CInternetSession* pSession{};
    CHttpFile* pfile{};
    try
    {
        pSession = new CInternetSession(user_agent.c_str());
        // 使用 BINARY 以获取原始字节，避免 WinInet 在 ASCII 模式下进行隐式字符集转换导致乱码
        DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_PRAGMA_NOCACHE | INTERNET_FLAG_TRANSFER_BINARY;
        pfile = (CHttpFile*)pSession->OpenURL(url.c_str(), 1, flags);
        DWORD dwStatusCode{};
        pfile->QueryInfoStatusCode(dwStatusCode);
        if (dwStatusCode == HTTP_STATUS_OK)
        {
            // 检测响应头中的 charset
            CString contentType;
            UINT codepage = CP_UTF8; // 默认按 UTF-8
            if (pfile->QueryInfo(HTTP_QUERY_CONTENT_TYPE, contentType))
            {
                std::string ct = UnicodeToStr(contentType.GetString(), true);
                // 转小写并解析 charset=xxxx
                for (auto& ch : ct) ch = (char)tolower((unsigned char)ch);
                size_t pos = ct.find("charset=");
                if (pos != std::string::npos)
                {
                    std::string cs = ct.substr(pos + 8);
                    size_t end = cs.find_first_of("; \r\n\t");
                    if (end != std::string::npos) cs = cs.substr(0, end);
                    if (cs == "utf-8" || cs == "utf8") codepage = CP_UTF8;
                    else if (cs == "gb2312" || cs == "gbk") codepage = 936;
                    else if (cs == "big5") codepage = 950;
                    else if (cs == "shift_jis" || cs == "shift-jis" || cs == "sjis") codepage = 932;
                    else if (cs == "euc-kr") codepage = 949;
                }
            }

            // 以字节方式读取响应体，避免 MFC 内部使用 ACP 导致编码被二次转换
            std::string raw;
            const size_t BUFSZ = 4096;
            char buf[BUFSZ];
            UINT nRead = 0;
            do
            {
                nRead = pfile->Read(buf, BUFSZ);
                if (nRead > 0) raw.append(buf, nRead);
            } while (nRead > 0);

            // 如果响应头未给出或错误给出 charset，则在响应体前 2KB 中尝试检测 <meta charset=...>
            if (codepage == CP_UTF8) {
                size_t probe_len = std::min<size_t>(raw.size(), 2048);
                std::string probe = raw.substr(0, probe_len);
                for (auto& ch : probe) ch = (char)tolower((unsigned char)ch);
                if (probe.find("charset=gbk") != std::string::npos || probe.find("charset=gb2312") != std::string::npos) {
                    codepage = 936;
                } else if (probe.find("charset=big5") != std::string::npos) {
                    codepage = 950;
                } else if (probe.find("charset=shift_jis") != std::string::npos || probe.find("charset=shift-jis") != std::string::npos || probe.find("charset=sjis") != std::string::npos) {
                    codepage = 932;
                } else if (probe.find("charset=euc-kr") != std::string::npos) {
                    codepage = 949;
                }
            }

            if (codepage == CP_UTF8)
            {
                // 直接作为 UTF-8 返回
                result = raw;
            }
            else
            {
                // 将指定 codepage 的原始字节转为 Unicode，再转为 UTF-8
                int wlen = MultiByteToWideChar(codepage, 0, raw.data(), static_cast<int>(raw.size()), NULL, 0);
                if (wlen > 0)
                {
                    std::wstring w;
                    w.resize(wlen);
                    MultiByteToWideChar(codepage, 0, raw.data(), static_cast<int>(raw.size()), &w[0], wlen);
                    result = UnicodeToStr(w.c_str(), true);
                }
                else
                {
                    // 兜底：按本地 ACP 读入再转 UTF-8（避免在非中文系统上出现乱码）
                    std::wstring w = StrToUnicode(raw.c_str(), false);
                    result = UnicodeToStr(w.c_str(), true);
                }
            }
            succeed = true;
        }
        pfile->Close();
        delete pfile;
        pSession->Close();
    }
    catch (CInternetException* e)
    {
        if (pfile != nullptr)
        {
            pfile->Close();
            delete pfile;
        }
        if (pSession != nullptr)
            pSession->Close();
        succeed = false;
        e->Delete();
        SAFE_DELETE(pSession);
    }
    SAFE_DELETE(pSession);
    return succeed;
}