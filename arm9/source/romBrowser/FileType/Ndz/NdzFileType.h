#pragma once
#include "../FileType.h"
#include "NdzInternalFileInfo.h"
#include "core/StringUtil.h"
#include "../../Theme/IThemeFileIconFactory.h"

/// @brief File type for .ndz roms (compressed .nds with front-matter).
class NdzFileType : public FileType
{
public:
    static const NdzFileType sInstance;

    std::unique_ptr<FileIcon> CreateFileIcon(const TCHAR* fileName,
        const IThemeFileIconFactory* themeFileIconFactory) const override
    {
        return themeFileIconFactory->CreateNdsFileIcon(fileName);
    }

    InternalFileInfo* CreateInternalFileInfo(const FastFileRef& fastFileRef) const override
    {
        return new NdzInternalFileInfo(fastFileRef);
    }

    bool TrySetLaunchParameters(pload_params_t* launchParameters, const char* filePath) const override
    {
        StringUtil::Copy(launchParameters->romPath, filePath, sizeof(launchParameters->romPath));
        return true;
    }

private:
    constexpr NdzFileType()
        : FileType("ndz", FileTypeClassification::Game) { }
};
