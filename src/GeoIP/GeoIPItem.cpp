#include "pch.h"
#include "GeoIPItem.h"
#include "GeoIP.h"

const wchar_t* CGeoIPItem::GetItemName() const {
    return L"GeoIP";
}

const wchar_t* CGeoIPItem::GetItemId() const {
    // 唯一ID：仅字母数字
    return L"GeoIP01";
}

const wchar_t* CGeoIPItem::GetItemLableText() const {
    return L"";
}

const wchar_t* CGeoIPItem::GetItemValueText() const {
    return CGeoIP::Instance().GetDisplayText().c_str();
}

const wchar_t* CGeoIPItem::GetItemValueSampleText() const {
    // 返回较长的示例文本以确保任务栏分配足够的宽度，避免国家+城市+IP被截断
    // 注：此示例同时覆盖“国家代码·城市”和“国家代码·城市·IP”等显示模式的宽度需求
    return L"US·Los Angeles·255.255.255.255";
}