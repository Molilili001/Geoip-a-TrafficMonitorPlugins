#pragma once
#include "PluginInterface.h"
#include <string>

class CGeoIP;

class CGeoIPItem : public IPluginItem {
public:
    virtual const wchar_t* GetItemName() const override;
    virtual const wchar_t* GetItemId() const override;
    virtual const wchar_t* GetItemLableText() const override;
    virtual const wchar_t* GetItemValueText() const override;
    virtual const wchar_t* GetItemValueSampleText() const override;
};