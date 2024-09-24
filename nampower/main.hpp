//
// Created by pmacc on 9/21/2024.
//

#pragma once

#include <hadesmem/process.hpp>
#include <hadesmem/patcher.hpp>

#include <Windows.h>

#include <cstdint>
#include <memory>
#include <atomic>

#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>

#include "game.hpp"

namespace Nampower {
    extern std::ofstream debugLogFile;

    extern DWORD gStartTime;

    extern DWORD GetTime();

#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

// TODO uncomment once ready for release
//#ifdef _DEBUG
//std::ofstream debugLogFile("nampower_debug.log");
//#define DEBUG_LOG(msg) debugLogFile << "[DEBUG]" << GetTime() << ": " << msg << std::endl
//#else
//#define DEBUG_LOG(msg) // No-op in release mode
//#endif

#define DEBUG_LOG(msg) debugLogFile << "[DEBUG]" << GetTime() << ": " << msg << std::endl

#endif  // DEBUG_LOG_H

    constexpr DWORD MAX_TIME_SINCE_LAST_CAST_FOR_QUEUE = 10000; // time limit in ms after which queued casts are ignored in errors
    constexpr DWORD DYNAMIC_BUFFER_INCREMENT = 5; // amount to adjust buffer in ms on errors/lack of errors
    constexpr DWORD BUFFER_INCREASE_FREQUENCY = 5000; // time in ms between changes to raise buffer
    constexpr DWORD BUFFER_DECREASE_FREQUENCY = 10000; // time in ms between changes to lower buffer

    /* Configurable settings set by user */
    extern bool gQueueCastTimeSpells;
    extern bool gQueueInstantSpells;
    extern bool gQueueOnSwingSpells;
    extern bool gQueueChannelingSpells;
    extern bool gQueueTargetingSpells;

    extern bool gQuickcastTargetingSpells;

    extern DWORD gSpellQueueWindowMs;
    extern DWORD gOnSwingBufferCooldownMs;
    extern DWORD gChannelQueueWindowMs;
    extern DWORD gTargetingQueueWindowMs;

    extern DWORD gMinBufferTimeMs;
    extern DWORD gMaxBufferIncrease;
    /* */

    extern DWORD gCastEndMs;
    extern DWORD gGCDEndMs;
    extern DWORD gBufferTimeMs;   // adjusts dynamically depending on errors
    extern DWORD gLastCast;
    extern DWORD gLastCastStartTime;
    extern DWORD gLastCastOnGCD;
    extern DWORD gLastChannelStartTime;
    extern DWORD gLastOnSwingCastTime;

    extern DWORD gLastErrorTimeMs;
    extern DWORD gLastBufferIncreaseTimeMs;
    extern DWORD gLastBufferDecreaseTimeMs;

    extern bool gSpellQueued;
    extern bool gChannelQueued;
    extern bool gOnSwingQueued;

    extern bool gNormalQueueTriggered;

    // true when we are simulating a server-based spell cancel to reset the cast bar
    extern bool gCancelling;

    // true when we are in the middle of an attempt to cast a spell
    extern bool gCasting;
    extern bool gChanneling;
    extern uint32_t gChannelDuration;
    extern uint32_t gChannelCastCount;

    extern hadesmem::PatchDetourBase *onSwingDetour;
    extern hadesmem::PatchDetourBase *lastDetour;
    extern void *lastUnit;
    extern uint32_t lastSpellId;
    extern uint32_t lastOnSwingSpellId;
    extern void *lastItem;
    extern uint32_t lastCastTime;
    extern uint64_t lastGuid;

    using CastSpellT = bool (__fastcall *)(void *, uint32_t, void *, std::uint64_t);
    using SendCastT = void (__fastcall *)(game::SpellCast *, char unk);
    using CancelSpellT = void (__stdcall *)(game::SpellCastResult);
    using SignalEventT = void (__fastcall *)(game::Events);
    using PacketHandlerT = int (__stdcall *)(int, game::CDataStore *);
    using ISceneEndT = int *(__fastcall *)(uintptr_t *unk);
    using EndSceneT = int (__fastcall *)(uintptr_t *unk);
    using SpellStartHandlerT = void (__fastcall *)(int, int, int, game::CDataStore *);
    using SpellChannelStartHandlerT = int (__stdcall *)(int, game::CDataStore *);
    using SpellChannelUpdateHandlerT = int (__stdcall *)(int, game::CDataStore *);
    using Spell_C_SpellFailedT = void (__fastcall *)(int, game::SpellCastResult, int, int, char unk3);
    using Spell_C_GetAutoRepeatingSpellT = int (__cdecl *)();
    using SpellFailedHandlerT = void (__fastcall *)(int, game::CDataStore *);
    using CastResultHandlerT = bool (__fastcall *)(std::uint64_t, game::CDataStore *);
    using SpellGoT = void (__fastcall *)(uint64_t *, uint64_t *, uint32_t, game::CDataStore *);
    using SpellTargetUnitT = bool (__fastcall *)(uintptr_t *unitStr);
    using Spell_C_HandleSpriteClickT = bool (__fastcall *)(game::CSpriteClickEvent *event);
    using Spell_C_TargetSpellT = bool (__fastcall *)(
            uint32_t *player,
            uint32_t *spellId,
            uint32_t unk3,
            float unk4);
    using Script_GetGUIDFromNameT = std::uint64_t (__fastcall *)(const char *);
    using lua_isstringT = bool (__fastcall *)(uintptr_t *, int);
    using lua_tostringT = const char *(__fastcall *)(uintptr_t *, int);

    using SpellVisualsInitializeT = void (__stdcall *)(void);

    using CVarLookupT = uintptr_t *(__fastcall *)(const char *);
    using SetCVarT = int (__fastcall *)(uintptr_t *luaPtr);
    using CVarRegisterT = int *(__fastcall *)(char *name, char *help, int unk1, const char *defaultValuePtr,
                                              void *callbackPtr,
                                              int category, char unk2, int unk3);

    using CVarCallback = void (*)(char *);


    void LuaCall(const char *code);
}
