#include "common.h"
#include <string.h>
#include "UnknownFileType.h"
#include "Nds/NdsFileType.h"
#include "Ndz/NdzFileType.h"
#include "Gba/GbaFileType.h"
#include "ExtensionFileTypeProvider.h"

static bool isNdsExtension(const char* extension)
{
    return !strcasecmp(extension, "nds")
        || !strcasecmp(extension, "srl")
        || !strcasecmp(extension, "dsi");
}

static bool isNdzExtension(const char* extension)
{
    return !strcasecmp(extension, "ndz");
}

static bool isGbaExtension(const char* extension)
{
    return !strcasecmp(extension, "gba")
        || !strcasecmp(extension, "agb");
}

ExtensionFileTypeProvider::ExtensionFileTypeProvider(const AppSettings& appSettings)
    : _appSettings(appSettings)
{
    _customFileTypes = std::make_unique_for_overwrite<CustomFileType[]>(appSettings.numberOfFileAssociations);
    for (u32 i = 0; i < appSettings.numberOfFileAssociations; i++)
    {
        const FileType* baseFileType = nullptr;
        if (isGbaExtension(appSettings.fileAssociations[i].extension))
        {
            baseFileType = &GbaFileType::sInstance;
        }
        _customFileTypes[i] = CustomFileType(&appSettings.fileAssociations[i], baseFileType);
    }
}

const FileType* ExtensionFileTypeProvider::GetFileType(const TCHAR* path) const
{
    const char* extension = strrchr(path, '.');
    if (extension)
    {
        extension++; // skip over dot

        if (isNdsExtension(extension))
        {
            return &NdsFileType::sInstance;
        }

        if (isNdzExtension(extension))
        {
            return &NdzFileType::sInstance;
        }

        for (u32 i = 0; i < _appSettings.numberOfFileAssociations; i++)
        {
            if (!strcasecmp(extension, _customFileTypes[i].GetShortName()))
            {
                return &_customFileTypes[i];
            }
        }
    }

    return &UnknownFileType::sInstance;
}