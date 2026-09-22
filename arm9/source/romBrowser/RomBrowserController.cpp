#include "common.h"
#include <array>
#include "picoLoaderBootstrap.h"
#include "PicoLoaderProcess.h"
#include "settings/SettingsProcess.h"
#include "FileType/ExtensionFileTypeProvider.h"
#include "FileType/FileType.h"
#include "SdFolderFactory.h"
#include "services/settings/IAppSettingsService.h"
#include "cheats/UsrCheatRepositoryFactory.h"
#include "cheats/EmptyCheatRepository.h"
#include "cheats/PicoLoaderCheatDataFactory.h"
#include "fat/File.h"
#include "FileType/Ndz/ndzFormat.h"
#include "RomBrowserController.h"

RomBrowserController::RomBrowserController(
    IAppSettingsService* appSettingsService, TaskQueueBase* ioTaskQueue,
    TaskQueueBase* bgTaskQueue)
    : _appSettingsService(appSettingsService)
    , _ioTaskQueue(ioTaskQueue), _bgTaskQueue(bgTaskQueue)
    , _fileTypeProvider(appSettingsService->GetAppSettings()) { }

void RomBrowserController::NavigateToPath(const TCHAR* name)
{
    StringUtil::Copy(_navigatePath, name, sizeof(_navigatePath) / sizeof(_navigatePath[0]));
    _stateMachine.Fire(RomBrowserStateTrigger::Navigate);
}

void RomBrowserController::LaunchFile(const FileInfo& fileInfo)
{
    _triggerFileInfo = FileInfo(fileInfo);
    _triggerHasDelta = false;
    _stateMachine.Fire(RomBrowserStateTrigger::Launch);
}

void RomBrowserController::LaunchDelta(const FileInfo& base, const FileInfo& delta)
{
    _triggerFileInfo = FileInfo(base);
    _triggerDeltaFileInfo = FileInfo(delta);
    _triggerHasDelta = true;
    _stateMachine.Fire(RomBrowserStateTrigger::Launch);
}

void RomBrowserController::ShowHackSelect(const FileInfo& base)
{
    _hackSelectBase = FileInfo(base);
    _stateMachine.Fire(RomBrowserStateTrigger::ShowHackSelect);
}

void RomBrowserController::HideHackSelect()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideHackSelect);
}

void RomBrowserController::SetHackNtrMode(const FileInfo& delta, bool on)
{
    if (_ndzDeltaIndex)
        _ndzDeltaIndex->SetNtrMode(delta.GetFileName(), on);
    // opened by path: a write closes through the directory entry, which the
    // fast file ref lacks. The path is a member because an IO task slot is 32 bytes.
    BuildCurrentFolderFilePath(delta.GetFileName(), _hackOptionPath,
        sizeof(_hackOptionPath) / sizeof(_hackOptionPath[0]));
    _hackOptionNtrMode = on;
    _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        auto file = std::make_unique<File>();
        if (file->Open(_hackOptionPath, FA_READ | FA_WRITE) != FR_OK)
        {
            LOG_ERROR("Couldn't open %s to change its options\n", _hackOptionPath);
            return TaskResult<void>::Completed();
        }
        u32 options = 0;
        u32 written = 0;
        if (file->Seek(NDZ_OFFSET_DELTA_OPTIONS) == FR_OK && file->ReadExact(&options, sizeof(options)))
        {
            options = _hackOptionNtrMode
                ? (options | NDZ_DELTA_OPT_NTR_MODE)
                : (options & ~NDZ_DELTA_OPT_NTR_MODE);
            if (file->Seek(NDZ_OFFSET_DELTA_OPTIONS) != FR_OK
                || file->Write(&options, sizeof(options), written) != FR_OK || written != sizeof(options))
            {
                LOG_ERROR("Couldn't write the options of %s\n", _hackOptionPath);
            }
        }
        file->Close();
        return TaskResult<void>::Completed();
    });
}

void RomBrowserController::ShowGameInfo(const FileInfo& fileInfo)
{
    _triggerFileInfo = FileInfo(fileInfo);
    _stateMachine.Fire(RomBrowserStateTrigger::ShowGameInfo);
}

void RomBrowserController::HideGameInfo()
{
    _stateMachine.Fire(RomBrowserStateTrigger::HideGameInfo);
}

void RomBrowserController::ShowDisplaySettings()
{
    _stateMachine.Fire(RomBrowserStateTrigger::ShowDisplaySettings);
}

void RomBrowserController::HideDisplaySettings()
{
    if (_saveSettingsPending)
    {
        _saveSettingsPending = false;
        _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
        {
            _appSettingsService->Save();
            return TaskResult<void>::Completed();
        });
    }
    _stateMachine.Fire(RomBrowserStateTrigger::HideDisplaySettings);
}

void RomBrowserController::GotoSettingsScreen()
{
    _stateMachine.Fire(RomBrowserStateTrigger::GotoSettingsScreen);
}

void RomBrowserController::SetRomBrowserDisplaySettings(
    const RomBrowserDisplaySettings& romBrowserDisplaySettings)
{
    _appSettingsService->GetAppSettings().romBrowserDisplaySettings = romBrowserDisplaySettings;
    _saveSettingsPending = true;
    _stateMachine.Fire(RomBrowserStateTrigger::ChangeDisplayMode);
}

void RomBrowserController::Update()
{
    _stateMachine.Update();
    if (_stateMachine.HasStateChanged())
    {
        HandleTrigger();
    }
    switch (_stateMachine.GetCurrentState())
    {
        case RomBrowserState::Start:
        {
            LOG_DEBUG("RomBrowserState::Start\n");
            const auto& lastUsed = _appSettingsService->GetAppSettings().lastUsedFilePath;
            if (strlen(lastUsed.GetString()) != 0)
            {
                NavigateToPath(lastUsed.GetString());
            }
            else
            {
                NavigateToPath("/");
            }
            break;
        }
        case RomBrowserState::LoadingFolder:
        {
            if (_navigateTask.GetTask().IsCompletedSuccessfully())
            {
                _navigateTask.Dispose();
                _stateMachine.Fire(RomBrowserStateTrigger::FolderLoadDone);
            }
            break;
        }
        case RomBrowserState::Launching:
        default:
        {
            break;
        }
    }
}

void RomBrowserController::HandleTrigger()
{
    switch (_stateMachine.GetLastTrigger())
    {
        case RomBrowserStateTrigger::Navigate:
            HandleNavigateTrigger();
            break;

        case RomBrowserStateTrigger::FolderLoadDone:
            HandleFolderLoadDoneTrigger();
            break;

        case RomBrowserStateTrigger::Launch:
            HandleLaunchTrigger();
            break;

        case RomBrowserStateTrigger::ChangeDisplayMode:
            HandleChangeDisplayModeTrigger();
            break;

        case RomBrowserStateTrigger::GotoSettingsScreen:
            HandleGotoSettingsScreenTrigger();
            break;

        default:
            break;
    }
}

void RomBrowserController::HandleNavigateTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::Navigate\n");
    _navigateTask = _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        if (!_coverRepository)
        {
            _coverRepository = std::make_unique<CoverRepository>();
            _coverRepository->Initialize();
        }
        if (!_iconRepository)
        {
            _iconRepository = std::make_unique<IconRepository>();
            _iconRepository->Initialize();
        }
        if (!_bannerRepository)
        {
            _bannerRepository = std::make_unique<BannerRepository>();
            _bannerRepository->Initialize();
        }
        if (!_cheatRepository)
        {
            _cheatRepository = UsrCheatRepositoryFactory().FromUsrCheatDat("/_pico/usrcheat.dat");
            if (!_cheatRepository)
            {
                // When usrcheat.dat is not found or cannot be read use a dummy empty cheat repository
                _cheatRepository = std::make_unique<EmptyCheatRepository>();
            }
        }

        u64 startTick = gTickCounter.GetValue();
        _navigateFileName = nullptr;
        if (strcmp(_navigatePath, "/") != 0) // can't f_stat on root dir
        {
            FILINFO fileInfo;
            if (f_stat(_navigatePath, &fileInfo) != FR_OK)
            {
                StringUtil::Copy(_navigatePath, "/", sizeof(_navigatePath) / sizeof(_navigatePath[0]));
            }
            else if (!(fileInfo.fattrib & AM_DIR))
            {
                _navigateFileName = strrchr(_navigatePath, '/') + 1;
                _navigateFileName[-1] = 0;
            }
        }
        f_chdir(_navigatePath);
        SdFolderFactory sdFolderFactory { &_fileTypeProvider };
        _newSdFolder = sdFolderFactory.CreateFromPath(".");
        // join delta .ndz files to their base while the card is ours to read
        _newNdzDeltaIndex = _newSdFolder ? NdzDeltaIndex::Build(*_newSdFolder) : nullptr;
        u64 endTick = gTickCounter.GetValue();
        LOG_DEBUG("Loading files in folder took: %d us\n", (u32)TickCounter::TicksToMicroSeconds(endTick - startTick));
        return TaskResult<void>::Completed();
    });
}

void RomBrowserController::HandleFolderLoadDoneTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::FolderLoadDone\n");
    _romBrowserViewModel.Reset();
    _sdFolder = std::move(_newSdFolder);
    _ndzDeltaIndex = std::move(_newNdzDeltaIndex); // points into _sdFolder
    _romBrowserViewModel = SharedPtr<RomBrowserViewModel>::MakeShared(this, _navigateFileName);
}

void RomBrowserController::HandleLaunchTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::Launch\n");
    _ioTaskQueue->Enqueue([this] (const vu8& cancelRequested)
    {
        UpdateLastUsedFilepath();
        SetPicoLoaderParams();
        LoadCheats();
        return TaskResult<void>::Completed();
    });
}

void RomBrowserController::HandleChangeDisplayModeTrigger()
{
    LOG_DEBUG("RomBrowserStateTrigger::ChangeDisplayMode\n");
    _romBrowserViewModel = SharedPtr<RomBrowserViewModel>::MakeShared(this);
}

void RomBrowserController::HandleGotoSettingsScreenTrigger()
{
    gProcessManager.Goto<SettingsProcess>();
}

void RomBrowserController::UpdateLastUsedFilepath()
{
    f_getcwd(_navigatePath, sizeof(_navigatePath) / sizeof(_navigatePath[0]));
    int idx = strlcat(_navigatePath, "/", sizeof(_navigatePath));
    if (_navigatePath[idx - 2] == '/')
    {
        _navigatePath[idx - 1] = 0;
    }
    strlcat(_navigatePath, _triggerFileInfo.GetFileName(), sizeof(_navigatePath));
    _appSettingsService->GetAppSettings().lastUsedFilePath = _navigatePath;
    _appSettingsService->Save();
}

void RomBrowserController::BuildCurrentFolderFilePath(const char* fileName, TCHAR* buffer, u32 bufferLength) const
{
    buffer[0] = 0;
    f_getcwd(buffer, bufferLength);
    int idx = strlcat(buffer, "/", bufferLength);
    if (idx >= 2 && buffer[idx - 2] == '/')
        buffer[idx - 1] = 0;
    strlcat(buffer, fileName, bufferLength);
}

void RomBrowserController::SetPicoLoaderParams() const
{
    auto loadParams = pload_getLoadParams();
    loadParams->savePath[0] = 0;
    loadParams->arguments[0] = 0;
    loadParams->argumentsLength = 0;
    // an older loader would boot the plain base and call it the hack
    u16 loaderApiVersion = _triggerHasDelta ? pload_readInstalledApiVersion() : 0;
    if (_triggerHasDelta && loaderApiVersion < 4)
    {
        LOG_FATAL("picoLoader7 is API %d, a delta .ndz needs 4.\n", loaderApiVersion);
        return;
    }
    if (_triggerFileInfo.GetFileType()->TrySetLaunchParameters(loadParams, _navigatePath))
    {
        if (_triggerHasDelta)
        {
            // romPath is the base; the save belongs to the hack, so it is named after the delta
            TCHAR deltaPath[256];
            BuildCurrentFolderFilePath(_triggerDeltaFileInfo.GetFileName(), deltaPath,
                sizeof(deltaPath) / sizeof(deltaPath[0]));
            pload_setDeltaPath(deltaPath);
            StringUtil::Copy(loadParams->savePath, deltaPath, sizeof(loadParams->savePath));
            char* extension = strrchr(loadParams->savePath, '.');
            if (!extension)
                extension = loadParams->savePath + strlen(loadParams->savePath);
            StringUtil::Copy(extension, ".sav",
                sizeof(loadParams->savePath) - (extension - loadParams->savePath));
        }
        else
        {
            pload_setDeltaPath("");
        }
        gProcessManager.Goto<PicoLoaderProcess>();
    }
    else
    {
        LOG_FATAL("Failed to set launch parameters.\n");
    }
}

void RomBrowserController::LoadCheats() const
{
    auto cheats = _cheatRepository->GetCheatsForGame(_triggerFileInfo.GetFastFileRef());
    auto cheatData = PicoLoaderCheatDataFactory().CreateCheatData(cheats);
    pload_setCheatData(cheatData);
}
