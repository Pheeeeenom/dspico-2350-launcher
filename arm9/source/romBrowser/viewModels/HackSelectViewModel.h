#pragma once
#include <memory>
#include <string.h>
#include "core/String.h"
#include "core/StringUtil.h"
#include "../IRomBrowserController.h"
#include "../NdzDeltaIndex.h"
#include "RomBrowserViewModel.h"

/// @brief View model for the version sheet: row 0 is the base .ndz, the rest are its
///        hacks. Everything is copied at construction so nothing dangles while the sheet animates.
class HackSelectViewModel
{
public:
    struct Item
    {
        FileInfo file;
        String<char16_t, 96> name;
        const char* kind;
    };

    explicit HackSelectViewModel(IRomBrowserController* romBrowserController)
        : _romBrowserController(romBrowserController)
    {
        const FileInfo& base = romBrowserController->GetHackSelectBase();
        const NdzDeltaIndex* index = romBrowserController->GetNdzDeltaIndex();
        u32 hackCount = 0;
        if (index)
        {
            for (u32 i = 0; i < index->GetEntryCount(); i++)
            {
                if (IsEntryOf(index->GetEntry(i), base))
                    hackCount++;
            }
        }
        _itemCount = 1 + hackCount;
        _items = std::make_unique<Item[]>(_itemCount);

        auto& baseItem = _items[0];
        baseItem.file = FileInfo(base);
        baseItem.kind = "base";
        SetBaseName(baseItem, base);

        u32 itemIndex = 1;
        for (u32 i = 0; index && i < index->GetEntryCount(); i++)
        {
            const auto& entry = index->GetEntry(i);
            if (!IsEntryOf(entry, base))
                continue;
            auto& item = _items[itemIndex++];
            item.file = FileInfo(*entry.delta);
            item.kind = "hack";
            char16_t name[97];
            StringUtil::Copy(name, entry.label.GetString(), 97);
            item.name = name;
        }
    }

    u32 GetItemCount() const { return _itemCount; }
    const Item& GetItem(u32 index) const { return _items[index]; }

    constexpr int GetSelectedItem() const { return _selectedItem; }
    void SetSelectedItem(int selectedItem) { _selectedItem = selectedItem; }

    void ActivateItem(int index)
    {
        if (index < 0 || (u32)index >= _itemCount)
            return;
        if (index == 0)
            _romBrowserController->LaunchFile(_items[0].file);
        else
            _romBrowserController->LaunchDelta(_items[0].file, _items[index].file);
    }

    void Close()
    {
        _romBrowserController->HideHackSelect();
    }

private:
    IRomBrowserController* _romBrowserController;
    std::unique_ptr<Item[]> _items;
    u32 _itemCount = 0;
    int _selectedItem = -1;

    static bool IsEntryOf(const NdzDeltaEntry& entry, const FileInfo& base)
    {
        return strcmp(entry.base->GetFileName(), base.GetFileName()) == 0;
    }

    // first line of the banner title when it is loaded, else the file name
    void SetBaseName(Item& item, const FileInfo& base) const
    {
        const auto& viewModel = _romBrowserController->GetRomBrowserViewModel();
        if (viewModel.IsValid())
        {
            auto& fileInfoManager = viewModel->GetFileInfoManager();
            int index = fileInfoManager.GetItemIndex(base.GetFileName());
            if (index >= 0 && fileInfoManager.IsFileInfoLoaded(index))
            {
                const auto* info = fileInfoManager.GetInternalFileInfo(index);
                const char16_t* title = info ? info->GetGameTitle() : nullptr;
                if (title && title[0] != 0)
                {
                    char16_t firstLine[97];
                    u32 n = 0;
                    while (title[n] != 0 && title[n] != u'\n' && n < 96)
                    {
                        firstLine[n] = title[n];
                        n++;
                    }
                    firstLine[n] = 0;
                    item.name = firstLine;
                    return;
                }
            }
        }
        char16_t fileName[97];
        StringUtil::Copy(fileName, base.GetFileName(), 97);
        item.name = fileName;
    }
};
