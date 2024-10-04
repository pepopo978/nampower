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
#include "types.h"
#include "castqueue.h"
#include "cdatastore.hpp"

namespace Nampower {
    constexpr uint32_t MAX_TIME_SINCE_LAST_CAST_FOR_QUEUE = 10000; // time limit in ms after which queued casts are ignored in errors
    constexpr uint32_t DYNAMIC_BUFFER_INCREMENT = 5; // amount to adjust buffer in ms on errors/lack of errors
    constexpr uint32_t BUFFER_INCREASE_FREQUENCY = 5000; // time in ms between changes to raise buffer
    constexpr uint32_t BUFFER_DECREASE_FREQUENCY = 10000; // time in ms between changes to lower buffer

    extern uint32_t gLastErrorTimeMs;
    extern uint32_t gLastBufferIncreaseTimeMs;
    extern uint32_t gLastBufferDecreaseTimeMs;

    extern uint32_t gBufferTimeMs;   // adjusts dynamically depending on errors

    extern hadesmem::PatchDetourBase *castSpellDetour;

    /* Configurable settings set by user */
    extern UserSettings gUserSettings;

    extern LastCastData gLastCastData;
    extern CastData gCastData;

    extern CastSpellParams gLastNormalCastParams;
    extern CastSpellParams gLastOnSwingCastParams;

    extern CastQueue gNonGcdCastQueue;
    extern CastQueue gCastHistory;

    using CastSpellT = bool (__fastcall *)(uint32_t *, uint32_t, void *, std::uint64_t);
    using SendCastT = void (__fastcall *)(game::SpellCast *, char unk);
    using CancelSpellT = void (__fastcall *)(bool, bool, game::SpellCastResult);
    using CancelAutoRepeatSpellT = void (__stdcall *)();
    using SignalEventT = void (__fastcall *)(game::Events);
    using PacketHandlerT = int (__stdcall *)(uint32_t *opCode, CDataStore *packet);
    using FastCallPacketHandlerT = int (__fastcall *)(uint32_t unk, uint32_t opCode, uint32_t unk2, CDataStore *packet);
    using ISceneEndT = int *(__fastcall *)(uintptr_t *unk);
    using EndSceneT = int (__fastcall *)(uintptr_t *unk);
    using SpellChannelStartHandlerT = int (__stdcall *)(int, CDataStore *);
    using SpellChannelUpdateHandlerT = int (__stdcall *)(int, CDataStore *);
    using Spell_C_SpellFailedT = void (__fastcall *)(uint32_t, game::SpellCastResult, int, int, char unk3);
    using Spell_C_GetAutoRepeatingSpellT = int (__cdecl *)();
    using SpellGoT = void (__fastcall *)(uint64_t *, uint64_t *, uint32_t, CDataStore *);
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

    using Spell_C_CooldownEventTriggeredT = void (__fastcall *)(uint32_t spellId,
                                                                uint64_t *targetGUID,
                                                                int param_3,
                                                                int clearCooldowns);

    using SpellVisualsInitializeT = void (__stdcall *)(void);

    using CVarLookupT = uintptr_t *(__fastcall *)(const char *);
    using SetCVarT = int (__fastcall *)(uintptr_t *luaPtr);
    using CVarRegisterT = int *(__fastcall *)(char *name, char *help, int unk1, const char *defaultValuePtr,
                                              void *callbackPtr,
                                              int category, char unk2, int unk3);

    void LuaCall(const char *code);

    bool IsNonSwingSpellQueued();

    void ResetCastFlags();

    uint32_t EffectiveCastEndMs();
}