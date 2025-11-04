#include "Common.h"
#include <windows.h>
#include <fstream>
#include <io.h>

namespace utilities
{
    bool CCommon::GetFileContent(const wchar_t* file_path, std::string& contents_buff)
    {
        std::ifstream file{ file_path, std::ios::binary };
        if (!file || file.fail())
            return false;
        // 使用 streampos/streamoff 计算长度，避免将负值或未定义值直接转换为 size_t
        file.seekg(0, std::ios::end);
        std::streampos end_pos = file.tellg();
        if (end_pos <= 0)
        {
            contents_buff.clear();
            return (end_pos == 0);
        }
        file.seekg(0, std::ios::beg);
        std::streampos beg_pos = file.tellg();
        if (beg_pos < 0)
            return false;

        std::streamoff len_off = end_pos - beg_pos;
        if (len_off <= 0)
        {
            contents_buff.clear();
            return (len_off == 0);
        }

        size_t length = static_cast<size_t>(len_off);
        contents_buff.resize(length);

        file.read(&contents_buff[0], static_cast<std::streamsize>(length));
        std::streamsize read_bytes = file.gcount();
        file.close();

        if (read_bytes <= 0)
        {
            contents_buff.clear();
            return false;
        }
        if (static_cast<size_t>(read_bytes) != length)
            contents_buff.resize(static_cast<size_t>(read_bytes));

        return true;
    }


    const char* CCommon::GetFileContent(const wchar_t* file_path, size_t& length)
    {
        std::ifstream file{ file_path, std::ios::binary };
        length = 0;
        if (!file || file.fail())
            return nullptr;
        // 使用 streampos/streamoff 计算长度，避免将负值直接转换为 size_t
        file.seekg(0, std::ios::end);
        std::streampos end_pos = file.tellg();
        if (end_pos <= 0)
            return nullptr;
        file.seekg(0, std::ios::beg);
        std::streampos beg_pos = file.tellg();
        if (beg_pos < 0)
            return nullptr;

        std::streamoff len_off = end_pos - beg_pos;
        if (len_off <= 0)
            return nullptr;

        size_t wanted = static_cast<size_t>(len_off);
        char* buff = new (std::nothrow) char[wanted];
        if (!buff)
            return nullptr;

        file.read(buff, static_cast<std::streamsize>(wanted));
        std::streamsize read_bytes = file.gcount();
        file.close();

        if (read_bytes <= 0)
        {
            delete[] buff;
            return nullptr;
        }

        length = static_cast<size_t>(read_bytes);
        return buff;
    }


    void CCommon::GetFiles(const wchar_t* path, std::vector<std::wstring>& files)
    {
        //�ļ����
        intptr_t hFile = 0;
        //�ļ���Ϣ
        _wfinddata_t fileinfo;
        if ((hFile = _wfindfirst(path, &fileinfo)) != -1)
        {
            do
            {
                std::wstring file_name(fileinfo.name);
                if (file_name != L"." && file_name != L"..")
                    files.push_back(file_name);  //���ļ�������(����"."��"..")
            } while (_wfindnext(hFile, &fileinfo) == 0);
        }
        _findclose(hFile);
    }


    /////////////////////////////////////////////////////////////////////////////////////////
    std::wstring StringHelper::StrToUnicode(const char* str, bool utf8 /*= false*/)
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

    std::string StringHelper::UnicodeToStr(const wchar_t* wstr, bool utf8 /*= false*/)
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

    bool StringHelper::StringReplace(std::wstring& str, const std::wstring& str_old, const std::wstring& str_new)
    {
        if (str.empty())
            return false;
        bool replaced{ false };
        size_t pos = 0;
        while ((pos = str.find(str_old, pos)) != std::wstring::npos)
        {
            str.replace(pos, str_old.length(), str_new);
            replaced = true;
            pos += str_new.length();    // ǰ�����滻����ַ���ĩβ
        }
        return replaced;
    }

    std::wstring StringHelper::StringFormat(const wchar_t* format_str, const std::initializer_list<CVariant>& paras)
    {
        std::wstring str = format_str;
        int index = 1;
        for (const auto& para : paras)
        {
            std::wstring format_str{ L"<%" + std::to_wstring(index) + L"%>" };
            StringHelper::StringReplace(str, format_str, para.ToString());
            index++;
        }
        return str;
    }

    template<class T>
    static void _StringNormalize(T& str)
    {
        if (str.empty()) return;

        int size = static_cast<int>(str.size());  //�ַ����ĳ���
        if (size < 0) return;
        int index1 = 0;     //�ַ����е�1�����ǿո������ַ���λ��
        int index2 = size - 1;  //�ַ��������һ�����ǿո������ַ���λ��
        while (index1 < size && str[index1] >= 0 && str[index1] <= 32)
            index1++;
        while (index2 >= 0 && str[index2] >= 0 && str[index2] <= 32)
            index2--;
        if (index1 > index2)    //���index1 > index2��˵���ַ���ȫ�ǿո������ַ�
            str.clear();
        else if (index1 == 0 && index2 == size - 1) //���index1��index2��ֵ�ֱ�Ϊ0��size - 1��˵���ַ���ǰ��û�пո������ַ���ֱ�ӷ���
            return;
        else
            str = str.substr(index1, index2 - index1 + 1);
    }


    void StringHelper::StringNormalize(std::wstring& str)
    {
        _StringNormalize(str);
    }

    void StringHelper::StringNormalize(std::string& str)
    {
        _StringNormalize(str);
    }

    //��һ���ַ����ָ�����ɸ��ַ���ģ������ֻ��Ϊstring��wstring��
    //str: ԭʼ�ַ���
    //div_ch: ���ڷָ���ַ�
    //result: ���շָ��Ľ��
    template<class T, class value_type>
    static void _StringSplit(const T& str, value_type div_ch, std::vector<T>& results, bool skip_empty = true, bool trim = true)
    {
        results.clear();
        typename T::size_type last = 0;
        typename T::size_type pos = 0;
        while ((pos = str.find(div_ch, last)) != T::npos)
        {
            T split_str = str.substr(last, pos - last);
            if (trim) _StringNormalize(split_str);
            if (!split_str.empty() || !skip_empty) results.push_back(split_str);
            last = pos + 1;
        }
        // tail
        T tail = (last <= str.size()) ? str.substr(last) : T();
        if (trim) _StringNormalize(tail);
        if (!tail.empty() || !skip_empty) results.push_back(tail);
    }

    template<class T>
    static void _StringSplit(const T& str, const T& div_str, std::vector<T>& results, bool skip_empty = true, bool trim = true)
    {
        results.clear();
        typename T::size_type last = 0;
        typename T::size_type pos = 0;
        const typename T::size_type div_len = div_str.size();
        if (div_len == 0)
        {
            // no delimiter: return whole string
            T s = str;
            if (trim) _StringNormalize(s);
            if (!s.empty() || !skip_empty) results.push_back(s);
            return;
        }
        while ((pos = str.find(div_str, last)) != T::npos)
        {
            T split_str = str.substr(last, pos - last);
            if (trim) _StringNormalize(split_str);
            if (!split_str.empty() || !skip_empty) results.push_back(split_str);
            last = pos + div_len;
        }
        // tail
        T tail = (last <= str.size()) ? str.substr(last) : T();
        if (trim) _StringNormalize(tail);
        if (!tail.empty() || !skip_empty) results.push_back(tail);
    }

    void StringHelper::StringSplit(const std::wstring& str, wchar_t div_ch, std::vector<std::wstring>& results, bool skip_empty, bool trim)
    {
        _StringSplit(str, div_ch, results, skip_empty, trim);
    }

    void StringHelper::StringSplit(const std::string& str, char div_ch, std::vector<std::string>& results, bool skip_empty, bool trim)
    {
        _StringSplit(str, div_ch, results, skip_empty, trim);
    }

    void StringHelper::StringSplit(const std::wstring& str, const std::wstring& div_str, std::vector<std::wstring>& results, bool skip_empty, bool trim)
    {
        _StringSplit(str, div_str, results, skip_empty, trim);
    }

    void StringHelper::StringSplit(const std::string& str, const std::string& div_str, std::vector<std::string>& results, bool skip_empty, bool trim)
    {
        _StringSplit(str, div_str, results, skip_empty, trim);
    }

}