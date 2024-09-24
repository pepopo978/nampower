/*
    Copyright (c) 2017-2023, namreeb (legal@namreeb.org)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
    ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    The views and conclusions contained in the software and documentation are those
    of the authors and should not be interpreted as representing official policies,
    either expressed or implied, of the FreeBSD Project.
*/

#include "offsets.hpp"
#include "game.hpp"
#include "main.hpp"
#include "spellevents.hpp"
#include "spellcast.hpp"
#include "spellchannel.hpp"

#include <Windows.h>

#include <cstdint>
#include <memory>
#include <atomic>

#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>

BOOL WINAPI DllMain(HINSTANCE, DWORD, void *);

namespace Nampower {
    std::ofstream debugLogFile("nampower_debug.log");

    /* Configurable settings set by user */
    bool gQueueCastTimeSpells;
    bool gQueueInstantSpells;
    bool gQueueOnSwingSpells;
    bool gQueueChannelingSpells;
    bool gQueueTargetingSpells;

    bool gQuickcastTargetingSpells;

    DWORD gSpellQueueWindowMs;
    DWORD gOnSwingBufferCooldownMs;
    DWORD gChannelQueueWindowMs;
    DWORD gTargetingQueueWindowMs;

    DWORD gMinBufferTimeMs;
    DWORD gMaxBufferIncrease;
    /* */

    DWORD gStartTime;
    DWORD gCastEndMs;
    DWORD gGCDEndMs;
    DWORD gBufferTimeMs;   // adjusts dynamically depending on errors

    DWORD gLastCast;
    DWORD gLastCastStartTime;
    DWORD gLastChannelStartTime;
    DWORD gLastOnSwingCastTime;

    DWORD gLastErrorTimeMs;
    DWORD gLastBufferIncreaseTimeMs;
    DWORD gLastBufferDecreaseTimeMs;

    // true a spell is queued
    bool gSpellQueued;
    bool gNormalQueueTriggered;
    bool gChannelQueued;
    bool gOnSwingQueued;

    // true when we are simulating a server-based spell cancel to reset the cast bar
    bool gCancelling;

    // true when the current spell cancellation requires that we notify the server
    // (a client side cancellation of a non-instant cast spell)
    bool gNotifyServer;

    // true when we are in the middle of an attempt to cast a spell
    bool gCasting;
    bool gChanneling;
    uint32_t gChannelDuration;
    uint32_t gChannelCastCount;

    hadesmem::PatchDetourBase *onSwingDetour;
    hadesmem::PatchDetourBase *lastDetour;
    void *lastUnit;
    uint32_t lastSpellId;
    uint32_t lastOnSwingSpellId;
    void *lastItem;
    uint32_t lastCastTime;
    uint64_t lastGuid;

    std::unique_ptr<hadesmem::PatchDetour<SetCVarT>> gSetCVarDetour;
    std::unique_ptr<hadesmem::PatchDetour<CastSpellT>> gCastDetour;
    std::unique_ptr<hadesmem::PatchDetour<SendCastT>> gSendCastDetour;
    std::unique_ptr<hadesmem::PatchDetour<CancelSpellT>> gCancelSpellDetour;
    std::unique_ptr<hadesmem::PatchDetour<SignalEventT>> gSignalEventDetour;
    std::unique_ptr<hadesmem::PatchDetour<PacketHandlerT>> gSpellDelayedDetour;
    std::unique_ptr<hadesmem::PatchDetour<Spell_C_SpellFailedT>> gSpellFailedDetour;
    std::unique_ptr<hadesmem::PatchRaw> gCastbarPatch;
    std::unique_ptr<hadesmem::PatchDetour<ISceneEndT>> gIEndSceneDetour;
    std::unique_ptr<hadesmem::PatchDetour<SpellVisualsInitializeT >> gSpellVisualsInitDetour;
    std::unique_ptr<hadesmem::PatchDetour<SpellStartHandlerT>> gSpellStartHandlerDetour;
    std::unique_ptr<hadesmem::PatchDetour<SpellChannelStartHandlerT>> gSpellChannelStartHandlerDetour;
    std::unique_ptr<hadesmem::PatchDetour<SpellChannelUpdateHandlerT>> gSpellChannelUpdateHandlerDetour;
    std::unique_ptr<hadesmem::PatchDetour<CastResultHandlerT>> gCastResultHandlerDetour;
    std::unique_ptr<hadesmem::PatchDetour<SpellFailedHandlerT>> gSpellFailedHandlerDetour;
    std::unique_ptr<hadesmem::PatchDetour<Spell_C_GetAutoRepeatingSpellT>> gSpell_C_GetAutoRepeatingSpellDetour;
    std::unique_ptr<hadesmem::PatchDetour<SpellGoT>> gSpellGoDetour;
    std::unique_ptr<hadesmem::PatchDetour<SpellTargetUnitT>> gSpellTargetUnitDetour;
    std::unique_ptr<hadesmem::PatchDetour<Spell_C_HandleSpriteClickT>> gSpell_C_HandleSpriteClickDetour;
    std::unique_ptr<hadesmem::PatchDetour<Spell_C_TargetSpellT>> gSpell_C_TargetSpellDetour;

    DWORD GetTime() {
        return static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count()) - gStartTime;
    }

    void LuaCall(const char *code) {
        typedef void __fastcall func(const char *code, const char *unused);
        func *function = (func *) Offsets::lua_call;
        function(code, "Unused");
    }

    int *ISceneEndHook(hadesmem::PatchDetourBase *detour, uintptr_t *ptr) {
        auto const iSceneEnd = detour->GetTrampolineT<ISceneEndT>();

        if (lastDetour && gSpellQueued) {
            auto currentTime = GetTime();

            // get max of cooldown and gcd
            auto delay = gCastEndMs > gGCDEndMs ? gCastEndMs : gGCDEndMs;

            if (lastCastTime == 0) {
                delay += gBufferTimeMs;
            }

            if (delay <= currentTime) {
                // if more than 5 seconds have passed since the last cast, ignore
                if (currentTime - gLastCast < MAX_TIME_SINCE_LAST_CAST_FOR_QUEUE) {
                    DEBUG_LOG("triggering queued cast for " << game::GetSpellName(lastSpellId));
                    gNormalQueueTriggered = true;

                    // call cast spell again
                    CastSpellHook(lastDetour, lastUnit, lastSpellId, lastItem, lastGuid);

                    gSpellQueued = false;
                    gNormalQueueTriggered = false;

                    return iSceneEnd(ptr);
                } else {
                    DEBUG_LOG("ignoring queued cast of " << game::GetSpellName(lastSpellId)
                                                         << " due to max time since last cast");
                    gSpellQueued = false;
                    lastDetour = nullptr;
                }
            }
        }

        return iSceneEnd(ptr);
    }

    void update_from_cvar(const char *cvar, const char *value) {
        if (strcmp(cvar, "NP_QueueCastTimeSpells") == 0) {
            gQueueCastTimeSpells = atoi(value) != 0;
            DEBUG_LOG("Set NP_QueueCastTimeSpells to " << gQueueCastTimeSpells);
        } else if (strcmp(cvar, "NP_QueueInstantSpells") == 0) {
            gQueueInstantSpells = atoi(value) != 0;
            DEBUG_LOG("Set NP_QueueInstantSpells to " << gQueueInstantSpells);
        } else if (strcmp(cvar, "NP_QueueOnSwingSpells") == 0) {
            gQueueOnSwingSpells = atoi(value) != 0;
            DEBUG_LOG("Set NP_QueueOnSwingSpells to " << gQueueOnSwingSpells);
        } else if (strcmp(cvar, "NP_QueueChannelingSpells") == 0) {
            gQueueChannelingSpells = atoi(value) != 0;
            DEBUG_LOG("Set NP_QueueChannelingSpells to " << gQueueChannelingSpells);
        } else if (strcmp(cvar, "NP_QueueTargetingSpells") == 0) {
            gQueueTargetingSpells = atoi(value) != 0;
            DEBUG_LOG("Set NP_QueueTargetingSpells to " << gQueueTargetingSpells);

        } else if (strcmp(cvar, "NP_QuickcastTargetingSpells") == 0) {
            gQuickcastTargetingSpells = atoi(value) != 0;
            DEBUG_LOG("Set NP_QuickcastTargetingSpells to " << gQuickcastTargetingSpells);


        } else if (strcmp(cvar, "NP_MinBufferTimeMs") == 0) {
            gMinBufferTimeMs = atoi(value);
            DEBUG_LOG("Set NP_MinBufferTimeMs to " << gMinBufferTimeMs);
        } else if (strcmp(cvar, "NP_MaxBufferIncreaseMs") == 0) {
            gMaxBufferIncrease = atoi(value);
            DEBUG_LOG("Set NP_MaxBufferIncreaseMs to " << gMaxBufferIncrease);

        } else if (strcmp(cvar, "NP_SpellQueueWindowMs") == 0) {
            gSpellQueueWindowMs = atoi(value);
            DEBUG_LOG("Set NP_SpellQueueWindowMs to " << gSpellQueueWindowMs);
        } else if (strcmp(cvar, "NP_OnSwingBufferCooldownMs") == 0) {
            gOnSwingBufferCooldownMs = atoi(value);
            DEBUG_LOG("Set NP_OnSwingBufferCooldownMs to " << gOnSwingBufferCooldownMs);
        } else if (strcmp(cvar, "NP_ChannelQueueWindowMs") == 0) {
            gChannelQueueWindowMs = atoi(value);
            DEBUG_LOG("Set NP_ChannelQueueWindowMs to " << gChannelQueueWindowMs);
        } else if (strcmp(cvar, "NP_TargetingQueueWindowMs") == 0) {
            gTargetingQueueWindowMs = atoi(value);
            DEBUG_LOG("Set NP_TargetingQueueWindowMs to " << gTargetingQueueWindowMs);
        }
    }

    int Script_SetCVarHook(hadesmem::PatchDetourBase *detour, uintptr_t *luaPtr) {
        auto const cvarSetOrig = gSetCVarDetour->GetTrampolineT<SetCVarT>();

        auto const lua_isstring = reinterpret_cast<lua_isstringT>(Offsets::lua_isstring);
        if (lua_isstring(luaPtr, 1)) {
            auto const lua_tostring = reinterpret_cast<lua_tostringT>(Offsets::lua_tostring);
            auto const cVarName = lua_tostring(luaPtr, 1);
            auto const cVarValue = lua_tostring(luaPtr, 2);
            // if cvar starts with "NP_", then we need to handle it
            if (strncmp(cVarName, "NP_", 3) == 0) {
                update_from_cvar(cVarName, cVarValue);
            }
        }

        return cvarSetOrig(luaPtr);
    }

    int *get_cvar(const char *cvar) {
        auto const cvarLookup = hadesmem::detail::AliasCast<CVarLookupT>(Offsets::CVarLookup);
        uintptr_t *cvarPtr = cvarLookup(cvar);

        if (cvarPtr) {
            return reinterpret_cast<int *>(cvarPtr +
                                           10); // get intValue from CVar which is consistent, strValue more complicated
        }
        return nullptr;
    }

    void load_user_var(const char *cvar) {
        int *value = get_cvar(cvar);
        if (value) {
            update_from_cvar(cvar, std::to_string(*value).c_str());
        } else {
            DEBUG_LOG("Using default value for " << cvar);
        }
    }

    void load_config() {
        gStartTime = static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count());

        // default values
        gQueueCastTimeSpells = true;
        gQueueInstantSpells = true;
        gQueueOnSwingSpells = true;
        gQueueChannelingSpells = true;
        gQueueTargetingSpells = true;

        gQuickcastTargetingSpells = false;

        gMinBufferTimeMs = 55; // time in ms to buffer cast to minimize server failure
        gMaxBufferIncrease = 30;

        gSpellQueueWindowMs = 500; // time in ms before cast to allow queuing spells
        gOnSwingBufferCooldownMs = 500; // time in ms to wait before queuing on swing spell after a swing
        gChannelQueueWindowMs = 1500; // time in ms before channel ends to allow queuing spells
        gTargetingQueueWindowMs = 500; // time in ms before cast to allow targeting

        char defaultTrue[] = "1";
        char defaultFalse[] = "0";

        DEBUG_LOG("Registering/Loading CVars");

        // register cvars
        auto const CVarRegister = hadesmem::detail::AliasCast<CVarRegisterT>(Offsets::RegisterCVar);

        char name1[] = "NP_QueueCastTimeSpells";
        CVarRegister(name1, // name
                     nullptr, // help
                     0,  // unk1
                     defaultTrue, // default value address
                     nullptr, // callback
                     5, // category
                     0,  // unk2
                     0); // unk3

        char name2[] = "NP_QueueInstantSpells";
        CVarRegister(name2, // name
                     nullptr, // help
                     0,  // unk1
                     defaultTrue, // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char name3[] = "NP_QueueOnSwingSpells";
        CVarRegister(name3, // name
                     nullptr, // help
                     0,  // unk1
                     defaultTrue, // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char name4[] = "NP_QueueChannelingSpells";
        CVarRegister(name4, // name
                     nullptr, // help
                     0,  // unk1
                     defaultTrue, // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char name5[] = "NP_QueueTargetingSpells";
        CVarRegister(name5, // name
                     nullptr, // help
                     0,  // unk1
                     defaultTrue, // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char name6[] = "NP_QuickcastTargetingSpells";
        CVarRegister(name6, // name
                     nullptr, // help
                     0,  // unk1
                     defaultFalse, // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char name7[] = "NP_MinBufferTimeMs";
        CVarRegister(name7, // name
                     nullptr, // help
                     0,  // unk1
                     std::to_string(gMinBufferTimeMs).c_str(), // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char name8[] = "NP_MaxBufferIncreaseMs";
        CVarRegister(name8, // name
                     nullptr, // help
                     0,  // unk1
                     std::to_string(gMaxBufferIncrease).c_str(), // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char name9[] = "NP_SpellQueueWindowMs";
        CVarRegister(name9, // name
                     nullptr, // help
                     0,  // unk1
                     std::to_string(gSpellQueueWindowMs).c_str(), // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char name10[] = "NP_ChannelQueueWindowMs";
        CVarRegister(name10, // name
                     nullptr, // help
                     0,  // unk1
                     std::to_string(gChannelQueueWindowMs).c_str(), // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char name11[] = "NP_TargetingQueueWindowMs";
        CVarRegister(name11, // name
                     nullptr, // help
                     0,  // unk1
                     std::to_string(gTargetingQueueWindowMs).c_str(), // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char name12[] = "NP_OnSwingBufferCooldownMs";
        CVarRegister(name12, // name
                     nullptr, // help
                     0,  // unk1
                     std::to_string(gOnSwingBufferCooldownMs).c_str(), // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        // update from cvars
        load_user_var("NP_QueueCastTimeSpells");
        load_user_var("NP_QueueInstantSpells");
        load_user_var("NP_QueueOnSwingSpells");
        load_user_var("NP_QueueChannelingSpells");
        load_user_var("NP_QueueTargetingSpells");

        load_user_var("NP_QuickcastTargetingSpells");

        load_user_var("NP_MinBufferTimeMs");
        load_user_var("NP_MaxBufferIncreaseMs");

        load_user_var("NP_SpellQueueWindowMs");
        load_user_var("NP_ChannelQueueWindowMs");
        load_user_var("NP_TargetingQueueWindowMs");
        load_user_var("NP_OnSwingBufferCooldownMs");

        gBufferTimeMs = gMinBufferTimeMs;
    }

    void init_hooks() {
        const hadesmem::Process process(::GetCurrentProcessId());

        auto const setCVarOrig = hadesmem::detail::AliasCast<SetCVarT>(Offsets::Script_SetCVar);
        gSetCVarDetour = std::make_unique<hadesmem::PatchDetour<SetCVarT >>(process, setCVarOrig, &Script_SetCVarHook);
        gSetCVarDetour->Apply();

        // activate spellbar and our own internal cooldown on a successful cast attempt (result from server not available yet)
        auto const castSpellOrig = hadesmem::detail::AliasCast<CastSpellT>(Offsets::Spell_C_CastSpell);
        gCastDetour = std::make_unique<hadesmem::PatchDetour<CastSpellT >>(process, castSpellOrig, &CastSpellHook);
        gCastDetour->Apply();

        auto const sendCastOrig = hadesmem::detail::AliasCast<SendCastT>(Offsets::SendCast);
        gSendCastDetour = std::make_unique<hadesmem::PatchDetour<SendCastT >>(process, sendCastOrig, &SendCastHook);
        gSendCastDetour->Apply();

        // monitor for client-based spell interruptions to stop the castbar
        auto const cancelSpellOrig = hadesmem::detail::AliasCast<CancelSpellT>(Offsets::CancelSpell);
        gCancelSpellDetour = std::make_unique<hadesmem::PatchDetour<CancelSpellT >>(process, cancelSpellOrig,
                                                                                    &CancelSpellHook);
        gCancelSpellDetour->Apply();

        auto const spellChannelStartHandlerOrig = hadesmem::detail::AliasCast<SpellChannelStartHandlerT>(
                Offsets::SpellChannelStartHandler);
        gSpellChannelStartHandlerDetour =
                std::make_unique<hadesmem::PatchDetour<SpellChannelStartHandlerT >>(process,
                                                                                    spellChannelStartHandlerOrig,
                                                                                    &SpellChannelStartHandlerHook);
        gSpellChannelStartHandlerDetour->Apply();

        auto const spellChannelUpdateHandlerOrig = hadesmem::detail::AliasCast<SpellChannelUpdateHandlerT>(
                Offsets::SpellChannelUpdateHandler);
        gSpellChannelUpdateHandlerDetour =
                std::make_unique<hadesmem::PatchDetour<SpellChannelUpdateHandlerT >>(process,
                                                                                     spellChannelUpdateHandlerOrig,
                                                                                     &SpellChannelUpdateHandlerHook);
        gSpellChannelUpdateHandlerDetour->Apply();

        // this hook will alter cast bar behavior based on events from the game
        auto const signalEventOrig = hadesmem::detail::AliasCast<SignalEventT>(Offsets::SignalEvent);
        gSignalEventDetour = std::make_unique<hadesmem::PatchDetour<SignalEventT >>(process, signalEventOrig,
                                                                                    &SignalEventHook);
        gSignalEventDetour->Apply();

        auto const spellFailedOrig = hadesmem::detail::AliasCast<Spell_C_SpellFailedT>(Offsets::Spell_C_SpellFailed);
        gSpellFailedDetour =
                std::make_unique<hadesmem::PatchDetour<Spell_C_SpellFailedT >>(process, spellFailedOrig,
                                                                               &Spell_C_SpellFailedHook);
        gSpellFailedDetour->Apply();

        auto const spellGoOrig = hadesmem::detail::AliasCast<SpellGoT>(Offsets::SpellGo);
        gSpellGoDetour = std::make_unique<hadesmem::PatchDetour<SpellGoT >>(process, spellGoOrig, &SpellGoHook);
        gSpellGoDetour->Apply();

        // watch for pushback notifications from the server
        auto const spellDelayedOrig = hadesmem::detail::AliasCast<PacketHandlerT>(Offsets::SpellDelayed);
        gSpellDelayedDetour = std::make_unique<hadesmem::PatchDetour<PacketHandlerT >>(process, spellDelayedOrig,
                                                                                       &SpellDelayedHook);
        gSpellDelayedDetour->Apply();

        auto const spellTargetUnitOrig = hadesmem::detail::AliasCast<SpellTargetUnitT>(Offsets::Script_SpellTargetUnit);
        gSpellTargetUnitDetour = std::make_unique<hadesmem::PatchDetour<
                SpellTargetUnitT >>(process, spellTargetUnitOrig, &SpellTargetUnitHook);
        gSpellTargetUnitDetour->Apply();

        auto const spell_C_TargetSpellOrig = hadesmem::detail::AliasCast<Spell_C_TargetSpellT>(
                Offsets::Spell_C_TargetSpell);
        gSpell_C_TargetSpellDetour = std::make_unique<hadesmem::PatchDetour<Spell_C_TargetSpellT >>(process,
                                                                                                    spell_C_TargetSpellOrig,
                                                                                                    &Spell_C_TargetSpellHook);
        gSpell_C_TargetSpellDetour->Apply();


        // Hook the ISceneEnd function to get the EndScene function pointer
        auto const iEndSceneOrig = hadesmem::detail::AliasCast<ISceneEndT>(Offsets::ISceneEndPtr);
        gIEndSceneDetour = std::make_unique<hadesmem::PatchDetour<ISceneEndT >>(
                process, iEndSceneOrig, &ISceneEndHook);
        gIEndSceneDetour->Apply();
    }

    void SpellVisualsInitializeHook(hadesmem::PatchDetourBase *detour) {
        DEBUG_LOG("SpellVisualsInitializeHook");
        auto const spellVisualsInitialize = detour->GetTrampolineT<SpellVisualsInitializeT>();
        spellVisualsInitialize();
        load_config();
        init_hooks();
    }

    void load() {
        DEBUG_LOG("Loading nampower v1.9.0");

        // hook spell visuals initialize
        const hadesmem::Process process(::GetCurrentProcessId());

        auto const spellVisualsInitOrig = hadesmem::detail::AliasCast<SpellVisualsInitializeT>(
                Offsets::SpellVisualsInitialize);
        gSpellVisualsInitDetour = std::make_unique<hadesmem::PatchDetour<SpellVisualsInitializeT >>(process,
                                                                                                    spellVisualsInitOrig,
                                                                                                    &SpellVisualsInitializeHook);
        gSpellVisualsInitDetour->Apply();
    }

}

extern "C" __declspec(dllexport) DWORD Load() {
    Nampower::load();
    return EXIT_SUCCESS;
}
