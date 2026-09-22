#pragma once
#include "picoLoader7.h"

pload_params_t* pload_getLoadParams();
void pload_setBootDrive(PicoLoaderBootDrive bootDrive);
void pload_setLauncherPath(const char* launcherPath);
/// @brief Delta .ndz to layer over the rom in the load params, "" for none. Needs API version 4.
void pload_setDeltaPath(const char* deltaPath);
/// @brief API version of picoLoader7.bin on the card, or 0 when it can't be read. IO thread only.
u16 pload_readInstalledApiVersion();
void pload_setCheatData(const pload_cheats_t* cheatData);
void pload_start();
