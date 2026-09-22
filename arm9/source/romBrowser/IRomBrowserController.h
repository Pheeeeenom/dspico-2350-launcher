#pragma once
#include "core/SharedPtr.h"
#include "services/settings/AppSettings.h"

class SdFolder;
class RomBrowserStateMachine;
class RomBrowserViewModel;
class FileInfo;
class TaskQueueBase;
class ICoverRepository;
class IIconRepository;
class IBannerRepository;
class ICheatRepository;
class NdzDeltaIndex;

class IRomBrowserController
{
public:
    virtual ~IRomBrowserController() = 0;

    virtual void NavigateUp() = 0;
    virtual void NavigateToPath(const TCHAR* name) = 0;
    virtual void LaunchFile(const FileInfo& fileInfo) = 0;
    /// @brief Launches \p base with the rom hack \p delta layered on top. The hack has its own save, named after the delta file.
    virtual void LaunchDelta(const FileInfo& base, const FileInfo& delta) = 0;
    /// @brief Opens the version sheet for \p base, a .ndz with rom hacks attached.
    virtual void ShowHackSelect(const FileInfo& base) = 0;
    virtual void HideHackSelect() = 0;
    /// @brief Base the version sheet was opened for. Valid while it is shown.
    virtual const FileInfo& GetHackSelectBase() const = 0;
    /// @brief Delta .ndz files of the current folder joined to their bases, or nullptr before the first folder load.
    virtual const NdzDeltaIndex* GetNdzDeltaIndex() const = 0;
    /// @brief Sets whether the rom hack \p delta boots in NTR mode instead of TWL mode. Stored in the delta file's front matter.
    virtual void SetHackNtrMode(const FileInfo& delta, bool on) = 0;
    virtual void ShowGameInfo(const FileInfo& fileInfo) = 0;
    virtual void HideGameInfo() = 0;
    virtual void ShowDisplaySettings() = 0;
    virtual void HideDisplaySettings() = 0;
    virtual void GotoSettingsScreen() = 0;

    virtual void Update() = 0;

    virtual const SdFolder& GetSdFolder() const = 0;

    virtual const RomBrowserStateMachine& GetStateMachine() const = 0;

    virtual const SharedPtr<RomBrowserViewModel>& GetRomBrowserViewModel() = 0;

    virtual TaskQueueBase* GetIoTaskQueue() const = 0;
    virtual TaskQueueBase* GetBgTaskQueue() const = 0;
    virtual const ICoverRepository& GetCoverRepository() const = 0;
    virtual const IIconRepository& GetIconRepository() const = 0;
    virtual const IBannerRepository& GetBannerRepository() const = 0;
    virtual const ICheatRepository& GetCheatRepository() const = 0;

    virtual const RomBrowserDisplaySettings& GetRomBrowserDisplaySettings() const = 0;

    virtual void SetRomBrowserDisplaySettings(
        const RomBrowserDisplaySettings& romBrowserDisplaySettings) = 0;

    virtual const FileInfo& GetTriggerFileInfo() const = 0;
};

inline IRomBrowserController::~IRomBrowserController() { }
