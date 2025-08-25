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

    constexpr uint32_t MAJOR_VERSION = 2;
    constexpr uint32_t MINOR_VERSION = 10;
    constexpr uint32_t PATCH_VERSION = 16;

    constexpr int32_t LUA_REGISTRYINDEX = -10000;
    constexpr int32_t LUA_GLOBALSINDEX = -10001;

    extern uint32_t gLastErrorTimeMs;
    extern uint32_t gLastBufferIncreaseTimeMs;
    extern uint32_t gLastBufferDecreaseTimeMs;

    extern uint32_t gBufferTimeMs;   // adjusts dynamically depending on errors

    extern bool gForceQueueCast;
    extern bool gNoQueueCast;

    extern uint64_t gNextCastId;

    extern uint32_t gRunningAverageLatencyMs;
    extern uint32_t gLastServerSpellDelayMs;

    extern hadesmem::PatchDetourBase *castSpellDetour;

    /* Configurable settings set by user */
    extern UserSettings gUserSettings;

    extern LastCastData gLastCastData;
    extern CastData gCastData;

    extern CastSpellParams gLastNormalCastParams;
    extern CastSpellParams gLastOnSwingCastParams;

    extern CastQueue gNonGcdCastQueue;
    extern CastQueue gCastHistory;

    extern bool gScriptQueued;

    using RangeCheckSelectedT = bool (__fastcall *)(uintptr_t *playerUnit, const game::SpellRec *,
                                                    std::uint64_t targetGuid, char ignoreErrors);
    using CastSpellT = bool (__fastcall *)(uintptr_t *playerUnit, uint32_t spellId, uintptr_t *item,
                                           std::uint64_t targetGuid);
    using SendCastT = void (__fastcall *)(game::SpellCast *, char unk);
    using CancelSpellT = void (__fastcall *)(bool, bool, game::SpellCastResult);
    using CancelAutoRepeatSpellT = void (__stdcall *)();
    using SignalEventT = void (__fastcall *)(game::Events);
    using PacketHandlerT = int (__stdcall *)(uint32_t *opCode, CDataStore *packet);
    using FastCallPacketHandlerT = int (__fastcall *)(uint32_t unk, uint32_t opCode, uint32_t unk2, CDataStore *packet);
    using ISceneEndT = int *(__fastcall *)(uintptr_t *unk);
    using EndSceneT = int (__fastcall *)(uintptr_t *unk);
    using OnSpriteRightClickT = int (__fastcall *)(uint64_t objectGUID);
    using CGGameUI_TargetT = void (__stdcall *)(uint64_t objectGUID);
    using Spell_C_SpellFailedT = void (__fastcall *)(uint32_t, game::SpellCastResult, int, int, char unk3);
    using Spell_C_GetAutoRepeatingSpellT = int (__cdecl *)();
    using SpellGoT = void (__fastcall *)(uint64_t *, uint64_t *, uint32_t, CDataStore *);
    using Spell_C_HandleSpriteClickT = bool (__fastcall *)(game::CSpriteClickEvent *event);
    using Spell_C_TargetSpellT = bool (__fastcall *)(
            uint32_t *player,
            uint32_t *spellId,
            uint32_t unk3,
            float unk4);
    using Spell_C_GetCastTimeT = uint32_t (__fastcall *)(uint32_t spellId, uint64_t *casterGuid, int avoidRounding);
    using Spell_C_GetSpellCooldownT = int (__fastcall *)(uint32_t spellId, uint32_t isPetSpell,
                                                         uint32_t *duration, uint64_t *startTime, uint32_t *enable);
    using Spell_C_IsSpellUsableT = int (__fastcall *)(const game::SpellRec *spellRec, uint32_t *usesManaReturn);

    using GetSpellSlotAndTypeT = int (__fastcall *)(const char *, uint32_t *);
    using GetTimeMsT = uint64_t (__stdcall *)();

    using GetClientConnectionT = uintptr_t *(__stdcall *)();
    using GetNetStatsT = void (__thiscall *)(uintptr_t *connection, float *param_1, float *param_2, uint32_t *param_3);
    using ClientServices_SendT = void (__fastcall *)(CDataStore *param_1);

    using LoadScriptFunctionsT = void (__stdcall *)();
    using FrameScript_RegisterFunctionT = void (__fastcall *)(char *name, uintptr_t *func);
    using FrameScript_CreateEventsT = void (__fastcall *)(int param_1, uint32_t maxEventId);

    using LuaGetContextT = uintptr_t *(__fastcall *)(void);
    using LuaGetTableT = void (__fastcall *)(uintptr_t *luaState, int globalsIndex);
    using LuaCallT = void (__fastcall *)(const char *code, const char *unused);
    using LuaScriptT = uint32_t (__fastcall *)(uintptr_t *luaState);
    using GetGUIDFromNameT = std::uint64_t (__fastcall *)(const char *);
    using GetUnitFromNameT = uintptr_t (__fastcall *)(const char *);
    using lua_gettableT = void (__fastcall *)(uintptr_t *luaState, int globalsIndex);
    using lua_isstringT = bool (__fastcall *)(uintptr_t *, int);
    using lua_isnumberT = bool (__fastcall *)(uintptr_t *, int);
    using lua_tostringT = char *(__fastcall *)(uintptr_t *, int);
    using lua_tonumberT = double (__fastcall *)(uintptr_t *, int);
    using lua_pushnumberT = void (__fastcall *)(uintptr_t *, double);
    using lua_pushstringT = void (__fastcall *)(uintptr_t *, char *);
    using lua_pcallT = int (__fastcall *)(uintptr_t *, int nArgs, int nResults, int errFunction);
    using lua_pushnilT = void (__fastcall *)(uintptr_t *);
    using lua_errorT = void (__cdecl *)(uintptr_t *, const char *);
    using lua_settopT = void (__fastcall *)(uintptr_t *, int);

    using Spell_C_CooldownEventTriggeredT = void (__fastcall *)(uint32_t spellId,
                                                                uint64_t *targetGUID,
                                                                int param_3,
                                                                int clearCooldowns);

    using SpellVisualsInitializeT = void (__stdcall *)(void);

    using PlaySpellVisual = void (__stdcall *)(int **param_1, void *param_2, int param_3, void **param_4);
    using CGUnit_C_ClearCastingSpellT = void (__thiscall *)(uintptr_t *unit, uint32_t param_1, int param_2, int param_3);
    using CGUnit_C_ClearSpellEffectT = void (__thiscall *)(uintptr_t *unit, uint32_t param_1, int param_2);

    using GetBuffByIndexT = uintptr_t *(__fastcall *)(int index);

    using CVarLookupT = uintptr_t *(__fastcall *)(const char *);
    using SetCVarT = int (__fastcall *)(uintptr_t *luaPtr);
    using CVarRegisterT = int *(__fastcall *)(char *name, char *help, int unk1, const char *defaultValuePtr,
                                              void *callbackPtr,
                                              int category, char unk2, int unk3);

    void RegisterLuaFunction(char *, uintptr_t *func);

    void LuaCall(const char *code);

    uintptr_t *GetLuaStatePtr();

    uint64_t GetWowTimeMs();

    uint32_t GetLatencyMs();

    uint32_t GetServerDelayMs();

    bool InSpellQueueWindow(uint32_t remainingCastTime, uint32_t remainingGcd, bool spellIsTargeting);

    bool IsNonSwingSpellQueued();

    void ResetChannelingFlags();

    void ResetCastFlags();

    void ResetOnSwingFlags();

    void ClearQueuedSpells();

    bool processQueues();

    uint32_t EffectiveCastEndMs();
}