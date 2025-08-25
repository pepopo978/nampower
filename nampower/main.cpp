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

#include "logging.hpp"
#include "offsets.hpp"
#include "game.hpp"
#include "main.hpp"
#include "spellevents.hpp"
#include "spellcast.hpp"
#include "scripts.hpp"
#include "spellchannel.hpp"
#include "helper.hpp"

#include <cstdint>
#include <memory>
#include <atomic>

#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>

BOOL WINAPI DllMain(HINSTANCE, uint32_t, void *);

namespace Nampower {
    uint32_t gLastErrorTimeMs;
    uint32_t gLastBufferIncreaseTimeMs;
    uint32_t gLastBufferDecreaseTimeMs;

    uint32_t gBufferTimeMs;   // adjusts dynamically depending on errors

    bool gForceQueueCast;
    bool gNoQueueCast;

    bool lastCastUsedServerDelay;

    uint64_t gNextCastId = 1;

    uint32_t gRunningAverageLatencyMs;
    uint32_t gLastServerSpellDelayMs;

    hadesmem::PatchDetourBase *castSpellDetour;

    UserSettings gUserSettings;

    LastCastData gLastCastData;
    CastData gCastData;

    CastSpellParams gLastNormalCastParams;
    CastSpellParams gLastOnSwingCastParams;

    CastQueue gNonGcdCastQueue = CastQueue(6);

    CastQueue gCastHistory = CastQueue(30);


    std::unique_ptr<hadesmem::PatchDetour<SpellVisualsInitializeT >> gSpellVisualsInitDetour;
    std::unique_ptr<hadesmem::PatchDetour<LoadScriptFunctionsT >> gLoadScriptFunctionsDetour;
    std::unique_ptr<hadesmem::PatchDetour<FrameScript_CreateEventsT >> gCreateEventsDetour;

    std::unique_ptr<hadesmem::PatchDetour<SetCVarT>> gSetCVarDetour;
    std::unique_ptr<hadesmem::PatchDetour<CastSpellT>> gCastDetour;
    std::unique_ptr<hadesmem::PatchDetour<SendCastT>> gSendCastDetour;
    std::unique_ptr<hadesmem::PatchDetour<CancelSpellT>> gCancelSpellDetour;
    std::unique_ptr<hadesmem::PatchDetour<SignalEventT>> gSignalEventDetour;
    std::unique_ptr<hadesmem::PatchDetour<Spell_C_SpellFailedT>> gSpellFailedDetour;
    std::unique_ptr<hadesmem::PatchRaw> gCastbarPatch;
    std::unique_ptr<hadesmem::PatchDetour<ISceneEndT>> gIEndSceneDetour;
    std::unique_ptr<hadesmem::PatchDetour<Spell_C_GetAutoRepeatingSpellT>> gSpell_C_GetAutoRepeatingSpellDetour;
    std::unique_ptr<hadesmem::PatchDetour<Spell_C_CooldownEventTriggeredT >> gSpell_C_CooldownEventTriggeredDetour;
    std::unique_ptr<hadesmem::PatchDetour<SpellGoT>> gSpellGoDetour;
    std::unique_ptr<hadesmem::PatchDetour<LuaScriptT>> gSpellTargetUnitDetour;
    std::unique_ptr<hadesmem::PatchDetour<LuaScriptT>> gSpellStopCastingDetour;
    std::unique_ptr<hadesmem::PatchDetour<LuaScriptT>> gCastSpellByNameNoQueueDetour;
    std::unique_ptr<hadesmem::PatchDetour<LuaScriptT>> gQueueSpellByNameDetour;
    std::unique_ptr<hadesmem::PatchDetour<LuaScriptT>> qQueueScriptDetour;
    std::unique_ptr<hadesmem::PatchDetour<LuaScriptT>> gIsSpellInRangeDetour;
    std::unique_ptr<hadesmem::PatchDetour<LuaScriptT>> gIsSpellUsableDetour;
    std::unique_ptr<hadesmem::PatchDetour<LuaScriptT>> gGetCurrentCastingInfoDetour;
    std::unique_ptr<hadesmem::PatchDetour<LuaScriptT>> gGetSpellIdForNameDetour;
    std::unique_ptr<hadesmem::PatchDetour<LuaScriptT>> gGetSpellNameAndRankForIdDetour;
    std::unique_ptr<hadesmem::PatchDetour<LuaScriptT>> gGetSpellSlotAndTypeForNameDetour;
    std::unique_ptr<hadesmem::PatchDetour<LuaScriptT>> gChannelStopCastingNextTickDetour;
    std::unique_ptr<hadesmem::PatchDetour<OnSpriteRightClickT>> gOnSpriteRightClickDetour;
    std::unique_ptr<hadesmem::PatchDetour<Spell_C_HandleSpriteClickT>> gSpell_C_HandleSpriteClickDetour;
    std::unique_ptr<hadesmem::PatchDetour<Spell_C_TargetSpellT>> gSpell_C_TargetSpellDetour;

    std::unique_ptr<hadesmem::PatchDetour<LuaScriptT>> gGetNampowerVersionDetour;
    std::unique_ptr<hadesmem::PatchDetour<LuaScriptT>> gGetItemLevelDetour;

    std::unique_ptr<hadesmem::PatchDetour<PacketHandlerT>> gSpellCooldownDetour;
    std::unique_ptr<hadesmem::PatchDetour<PacketHandlerT>> gSpellDelayedDetour;
    std::unique_ptr<hadesmem::PatchDetour<PacketHandlerT>> gCastResultHandlerDetour;
    std::unique_ptr<hadesmem::PatchDetour<PacketHandlerT>> gSpellFailedHandlerDetour;
    std::unique_ptr<hadesmem::PatchDetour<PacketHandlerT>> gSpellChannelStartHandlerDetour;
    std::unique_ptr<hadesmem::PatchDetour<PacketHandlerT>> gSpellChannelUpdateHandlerDetour;
    std::unique_ptr<hadesmem::PatchDetour<PacketHandlerT>> gPlaySpellVisualHandlerDetour;

    std::unique_ptr<hadesmem::PatchDetour<FastCallPacketHandlerT>> gSpellStartHandlerDetour;
    std::unique_ptr<hadesmem::PatchDetour<FastCallPacketHandlerT>> gPeriodicAuraLogHandlerDetour;
    std::unique_ptr<hadesmem::PatchDetour<FastCallPacketHandlerT>> gSpellNonMeleeDmgLogHandlerDetour;

    uint32_t GetTime() {
        return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count()) - gStartTime;
    }

    std::string GetHumanReadableTime() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::tm buf;
        localtime_s(&buf, &in_time_t);

        std::stringstream ss;
        ss << std::put_time(&buf, "%Y-%m-%d %X");
        ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    uint64_t GetWowTimeMs() {
        auto const osGetAsyncTimeMs = reinterpret_cast<GetTimeMsT>(Offsets::OsGetAsyncTimeMs);
        return osGetAsyncTimeMs();
    }

    void LuaCall(const char *code) {
        auto function = (LuaCallT) Offsets::lua_call;
        function(code, "Unused");
    }

    uintptr_t* GetLuaStatePtr() {
        typedef uintptr_t* (__fastcall* GETCONTEXT)(void);
        static auto p_GetContext = reinterpret_cast<GETCONTEXT>(0x7040D0);
        return p_GetContext();
    }


    void RegisterLuaFunction(char *name, uintptr_t *func) {
        DEBUG_LOG("Registering " << name << " to " << func);
        auto const registerFunction = reinterpret_cast<FrameScript_RegisterFunctionT>(Offsets::FrameScript_RegisterFunction);
        registerFunction(name, func);
    }

    // when the game first launches there won't be a cached latency value for 10-20 seconds and this will return 0
    uint32_t GetLatencyMs() {
        auto const getConnection = reinterpret_cast<GetClientConnectionT>(Offsets::GetClientConnection);
        auto const getNetStats = reinterpret_cast<GetNetStatsT>(Offsets::GetNetStats);

        auto connectionPtr = getConnection();

        float bytesIn, bytesOut;
        uint32_t latency;

        getNetStats(connectionPtr, &bytesIn, &bytesOut, &latency);

        // update running average
        if (latency > 0) {
            if (gRunningAverageLatencyMs == 0) {
                // first time
                gRunningAverageLatencyMs = latency;
                // ignore big spikes
            } else if (latency < gRunningAverageLatencyMs * 2) {
                gRunningAverageLatencyMs = (gRunningAverageLatencyMs * 9 + latency) / 10;
            }
        }

        return gRunningAverageLatencyMs;
    }

    uint32_t GetServerDelayMs() {
        // if we have a gLastServerSpellDelayMs that seems reasonable, use it
        // occasionally the server will take a long time to respond to a spell cast, in which case default to gBufferTimeMs
        // if gLastServerSpellDelayMs == 0, then we don't have a valid value for the last cast
        if (gUserSettings.optimizeBufferUsingPacketTimings && !lastCastUsedServerDelay) {
            if (gLastServerSpellDelayMs > 0 && gLastServerSpellDelayMs < 200) {
                lastCastUsedServerDelay = true;
                return gLastServerSpellDelayMs;
            }
        }

        lastCastUsedServerDelay = false;

        return gBufferTimeMs;
    }

    bool InSpellQueueWindow(uint32_t remainingCastTime, uint32_t remainingGcd, bool spellIsTargeting) {
        auto currentTime = GetTime();

        uint32_t queueWindow = 0;

        if (gCastData.channeling) {
            if (gUserSettings.queueChannelingSpells) {
                if (gCastData.cancelChannelNextTick) {
                    return true;
                }

                auto const remainingChannelTime = (gCastData.channelEndMs > currentTime) ? gCastData.channelEndMs -
                                                                                           currentTime : 0;
                return remainingChannelTime < gUserSettings.channelQueueWindowMs;
            }
        } else if (spellIsTargeting) {
            queueWindow = gUserSettings.targetingQueueWindowMs;
        } else {
            queueWindow = gUserSettings.spellQueueWindowMs;
        }

        if (remainingCastTime > 0) {
            return remainingCastTime < queueWindow || gForceQueueCast;
        }

        if (remainingGcd > 0) {
            return remainingGcd < queueWindow || gForceQueueCast;
        }

        return false;
    }

    bool IsNonSwingSpellQueued() {
        return gCastData.nonGcdSpellQueued || gCastData.normalSpellQueued;
    }

    uint32_t EffectiveCastEndMs() {
        if (gCastData.channeling && gUserSettings.queueChannelingSpells) {
            if (gUserSettings.interruptChannelsOutsideQueueWindow) {
                auto currentTime = GetTime();
                auto const remainingChannelTime = (gCastData.channelEndMs > currentTime) ? gCastData.channelEndMs -
                                                                                           currentTime : 0;
                if (remainingChannelTime < gUserSettings.channelQueueWindowMs) {
                    return gCastData.channelEndMs;
                }
            } else {
                return gCastData.channelEndMs;
            }
        }

        return max(gCastData.castEndMs, gCastData.delayEndMs);
    }

    void ResetChannelingFlags() {
        gCastData.cancelChannelNextTick = false;

        gCastData.channeling = false;
        gCastData.channelStartMs = 0;
        gCastData.channelEndMs = 0;
        gCastData.channelSpellId = 0;

        gCastData.channelTickTimeMs = 0;
        gCastData.channelNumTicks = 0;

    }

    void ResetCastFlags() {
        // don't reset delayEndMs
        gCastData.castEndMs = 0;
        gCastData.gcdEndMs = 0;
        ResetChannelingFlags();
    }

    void ResetOnSwingFlags() {
        gCastData.onSwingQueued = false;
        gCastData.onSwingSpellId = 0;
    }

    void ClearQueuedSpells() {
        if (gCastData.normalSpellQueued || gCastData.cooldownNormalSpellQueued) {
            TriggerSpellQueuedEvent(NORMAL_QUEUE_POPPED, gLastNormalCastParams.spellId);
            gCastData.normalSpellQueued = false;
            gCastData.cooldownNormalSpellQueued = false;
        }

        if (gCastData.nonGcdSpellQueued || gCastData.cooldownNonGcdSpellQueued) {
            while (!gNonGcdCastQueue.isEmpty()) {
                auto castParams = gNonGcdCastQueue.pop();
                TriggerSpellQueuedEvent(NON_GCD_QUEUE_POPPED, castParams.spellId);
            }
            gCastData.nonGcdSpellQueued = false;
            gCastData.cooldownNonGcdSpellQueued = false;
        }
    }

    void checkForStopChanneling() {
        if (gUserSettings.queueChannelingSpells && IsNonSwingSpellQueued()) {
            // for channels just end channeling
            auto const currentTime = GetTime();
            auto const elapsed = currentTime - gLastCastData.channelStartTimeMs;

            auto remainingChannelTime = (gCastData.channelEndMs > currentTime) ? gCastData.channelEndMs - currentTime
                                                                               : 0;

            auto const currentLatency = GetLatencyMs();
            uint32_t latencyReduction = 0;
            if (currentLatency > 0 && gUserSettings.channelLatencyReductionPercentage != 0) {
                latencyReduction = ((uint32_t) currentLatency * gUserSettings.channelLatencyReductionPercentage) / 100;

                if (remainingChannelTime > latencyReduction) {
                    remainingChannelTime -= latencyReduction;
                } else {
                    remainingChannelTime = 0;
                }
            }


            if (remainingChannelTime <= 0) {
                DEBUG_LOG("Ending channel [" << elapsed << " elapsed > "
                                             << gCastData.channelDuration << " original duration "
                                             << " latency reduction " << latencyReduction << "]"
                                             << " triggering queued spells");

                ResetChannelingFlags();
            } else if (gCastData.cancelChannelNextTick &&
                       gCastData.channelStartMs > 0 &&
                       currentTime - gCastData.channelStartMs < 60000) {
                auto nextTickTimeMs = gCastData.channelStartMs;

                // find the next tick time but don't choose the next tick until gBufferTimeMs has passed after a tick
                while (nextTickTimeMs < currentTime - gBufferTimeMs) {
                    nextTickTimeMs += gCastData.channelTickTimeMs;
                }

                uint32_t remainingTickTime = 0;
                if (nextTickTimeMs > currentTime) {
                    remainingTickTime = nextTickTimeMs - currentTime;
                }

                if (remainingTickTime > 0) {
                    if (currentLatency > 0 && gUserSettings.channelLatencyReductionPercentage != 0) {
                        if (remainingTickTime > latencyReduction) {
                            remainingTickTime -= latencyReduction;
                        } else {
                            remainingTickTime = 0;
                        }
                    } else {
                        // default to reducing by gBufferTimeMs
                        if (remainingTickTime > gBufferTimeMs) {
                            remainingTickTime -= gBufferTimeMs;
                        } else {
                            remainingTickTime = 0;
                        }
                    }
                }

                if (remainingTickTime <= 0) {
                    DEBUG_LOG("Ending channel due to cancelChannelNextTick. "
                                      << "Remaining tick time: " << nextTickTimeMs - currentTime
                                      << " latency reduction: " << latencyReduction);
                    ResetChannelingFlags();
                }
            }
        }
    }

    bool processQueues() {
        if (!gCastData.channeling) {
            // check for high priority script
            if (RunQueuedScript(1)) {
                // script ran, stop processing
                return true;
            }

            // check for non gcd spell
            if (gCastData.nonGcdSpellQueued) {
                auto currentTime = GetTime();

                if (EffectiveCastEndMs() <= currentTime) {
                    CastQueuedNonGcdSpell();
                    return true;
                }
            }

            // check for cooldown non gcd spell
            if (gCastData.cooldownNonGcdSpellQueued) {
                auto currentTime = GetTime();

                if (gCastData.cooldownNonGcdEndMs <= currentTime) {
                    DEBUG_LOG("Non gcd spell cooldown up, casting queued spell");
                    // trigger the regular non gcd queuing and turn off cooldownNonGcdSpellQueued
                    gCastData.cooldownNonGcdSpellQueued = false;
                    gCastData.nonGcdSpellQueued = true;
                    CastQueuedNonGcdSpell();
                    return true;
                }
            }

            // check for medium priority script
            if (RunQueuedScript(2)) {
                // script ran, stop processing
                return true;
            }

            // check for normal spell
            if (gCastData.normalSpellQueued) {
                auto currentTime = GetTime();

                auto effectiveCastEndMs = EffectiveCastEndMs();
                // get max of cooldown and gcd
                auto delay = effectiveCastEndMs > gCastData.gcdEndMs ? effectiveCastEndMs : gCastData.gcdEndMs;

                if (delay <= currentTime) {
                    // if more than MAX_TIME_SINCE_LAST_CAST_FOR_QUEUE seconds have passed since the last cast, ignore
                    if (currentTime - gLastCastData.startTimeMs < MAX_TIME_SINCE_LAST_CAST_FOR_QUEUE) {
                        CastQueuedNormalSpell();
                        return true;
                    } else {
                        DEBUG_LOG("Ignoring queued cast of " << game::GetSpellName(gLastNormalCastParams.spellId)
                                                             << " due to max time since last cast");
                        TriggerSpellQueuedEvent(NORMAL_QUEUE_POPPED, gLastNormalCastParams.spellId);
                        gCastData.normalSpellQueued = false;
                    }
                }
            }

            // check for cooldown normal spell
            if (gCastData.cooldownNormalSpellQueued) {
                auto currentTime = GetTime();

                if (gCastData.cooldownNormalEndMs <= currentTime) {
                    DEBUG_LOG("Spell cooldown up, casting queued spell");
                    // trigger the regular non gcd queuing and turn off cooldownNonGcdSpellQueued
                    gCastData.cooldownNormalSpellQueued = false;
                    gCastData.normalSpellQueued = true;
                    CastQueuedNormalSpell();
                    return true;
                }
            }

            if (RunQueuedScript(3)) {
                // script ran, stop processing
                return true;
            }
        }

        return false;
    }

    int OnSpriteRightClickHook(hadesmem::PatchDetourBase *detour, uint64_t objectGUID) {
        auto const onSpriteRightClick = detour->GetTrampolineT<OnSpriteRightClickT>();

        auto const currentTargetGuid = game::GetCurrentTargetGuid();

        // if we have a target that is not the right click target, ignore the right click
        if (gUserSettings.preventRightClickTargetChange && currentTargetGuid &&
            currentTargetGuid != objectGUID) {

            auto unitOrPlayer = game::ClntObjMgrObjectPtr(
                    static_cast<game::TypeMask>(game::TYPEMASK_PLAYER | game::TYPEMASK_UNIT), objectGUID);

            // only prevent right click if guid is a unit/player
            if (unitOrPlayer) {
                auto GetUnitFromName = reinterpret_cast<GetUnitFromNameT>(Offsets::GetUnitFromName);
                auto const unit = GetUnitFromName("player");

                // combat check from Script_UnitAffectingCombat
                // only prevent right click if in combat
                if (unit && ((*(uint32_t *) (*(int32_t *) (unit + 0x110) + 0xa0) >> 0x13 & 1) != 0)) {
                    return 1;
                }
            }
        }

        return onSpriteRightClick(objectGUID);
    }

    int *ISceneEndHook(hadesmem::PatchDetourBase *detour, uintptr_t *ptr) {
        auto const iSceneEnd = detour->GetTrampolineT<ISceneEndT>();

        // check if it's time to end channeling
        if (gCastData.channeling) {
            checkForStopChanneling();
        }

        // process any queued spells/scripts
        processQueues();

        return iSceneEnd(ptr);
    }

    void updateFromCvar(const char *cvar, const char *value) {
        if (strcmp(cvar, "NP_QueueCastTimeSpells") == 0) {
            gUserSettings.queueCastTimeSpells = atoi(value) != 0;
            DEBUG_LOG("Set NP_QueueCastTimeSpells to " << gUserSettings.queueCastTimeSpells);
        } else if (strcmp(cvar, "NP_QueueInstantSpells") == 0) {
            gUserSettings.queueInstantSpells = atoi(value) != 0;
            DEBUG_LOG("Set NP_QueueInstantSpells to " << gUserSettings.queueInstantSpells);
        } else if (strcmp(cvar, "NP_QueueOnSwingSpells") == 0) {
            gUserSettings.queueOnSwingSpells = atoi(value) != 0;
            DEBUG_LOG("Set NP_QueueOnSwingSpells to " << gUserSettings.queueOnSwingSpells);
        } else if (strcmp(cvar, "NP_QueueChannelingSpells") == 0) {
            gUserSettings.queueChannelingSpells = atoi(value) != 0;
            DEBUG_LOG("Set NP_QueueChannelingSpells to " << gUserSettings.queueChannelingSpells);
        } else if (strcmp(cvar, "NP_QueueTargetingSpells") == 0) {
            gUserSettings.queueTargetingSpells = atoi(value) != 0;
            DEBUG_LOG("Set NP_QueueTargetingSpells to " << gUserSettings.queueTargetingSpells);
        } else if (strcmp(cvar, "NP_QueueSpellsOnCooldown") == 0) {
            gUserSettings.queueSpellsOnCooldown = atoi(value) != 0;
            DEBUG_LOG("Set NP_QueueSpellsOnCooldown to " << gUserSettings.queueSpellsOnCooldown);

        } else if (strcmp(cvar, "NP_InterruptChannelsOutsideQueueWindow") == 0) {
            gUserSettings.interruptChannelsOutsideQueueWindow = atoi(value) != 0;
            DEBUG_LOG("Set NP_InterruptChannelsOutsideQueueWindow to "
                              << gUserSettings.interruptChannelsOutsideQueueWindow);

        } else if ((strcmp(cvar, "NP_RetryServerRejectedSpells") == 0)) {
            gUserSettings.retryServerRejectedSpells = atoi(value) != 0;
            DEBUG_LOG("Set NP_RetryServerRejectedSpells to " << gUserSettings.retryServerRejectedSpells);
        } else if (strcmp(cvar, "NP_QuickcastTargetingSpells") == 0) {
            gUserSettings.quickcastTargetingSpells = atoi(value) != 0;
            DEBUG_LOG("Set NP_QuickcastTargetingSpells to " << gUserSettings.quickcastTargetingSpells);
        } else if (strcmp(cvar, "NP_ReplaceMatchingNonGcdCategory") == 0) {
            gUserSettings.replaceMatchingNonGcdCategory = atoi(value) != 0;
            DEBUG_LOG("Set NP_ReplaceMatchingNonGcdCategory to " << gUserSettings.replaceMatchingNonGcdCategory);
        } else if (strcmp(cvar, "NP_OptimizeBufferUsingPacketTimings") == 0) {
            gUserSettings.optimizeBufferUsingPacketTimings = atoi(value) != 0;
            DEBUG_LOG("Set NP_OptimizeBufferUsingPacketTimings to " << gUserSettings.optimizeBufferUsingPacketTimings);

        } else if (strcmp(cvar, "NP_PreventRightClickTargetChange") == 0) {
            gUserSettings.preventRightClickTargetChange = atoi(value) != 0;
            DEBUG_LOG("Set NP_PreventRightClickTargetChange to " << gUserSettings.preventRightClickTargetChange);

        } else if (strcmp(cvar, "NP_DoubleCastToEndChannelEarly") == 0) {
            gUserSettings.doubleCastToEndChannelEarly = atoi(value) != 0;
            DEBUG_LOG("Set NP_DoubleCastToEndChannelEarly to " << gUserSettings.doubleCastToEndChannelEarly);

        } else if (strcmp(cvar, "NP_MinBufferTimeMs") == 0) {
            gUserSettings.minBufferTimeMs = atoi(value);
            DEBUG_LOG("Set NP_MinBufferTimeMs and current buffer to " << gUserSettings.minBufferTimeMs);
            gBufferTimeMs = gUserSettings.minBufferTimeMs;
        } else if (strcmp(cvar, "NP_NonGcdBufferTimeMs") == 0) {
            gUserSettings.nonGcdBufferTimeMs = atoi(value);
            DEBUG_LOG("Set NP_NonGcdBufferTimeMs to " << gUserSettings.nonGcdBufferTimeMs);
        } else if (strcmp(cvar, "NP_MaxBufferIncreaseMs") == 0) {
            gUserSettings.maxBufferIncreaseMs = atoi(value);
            DEBUG_LOG("Set NP_MaxBufferIncreaseMs to " << gUserSettings.maxBufferIncreaseMs);

        } else if (strcmp(cvar, "NP_SpellQueueWindowMs") == 0) {
            gUserSettings.spellQueueWindowMs = atoi(value);
            DEBUG_LOG("Set NP_SpellQueueWindowMs to " << gUserSettings.spellQueueWindowMs);
        } else if (strcmp(cvar, "NP_OnSwingBufferCooldownMs") == 0) {
            gUserSettings.onSwingBufferCooldownMs = atoi(value);
            DEBUG_LOG("Set NP_OnSwingBufferCooldownMs to " << gUserSettings.onSwingBufferCooldownMs);
        } else if (strcmp(cvar, "NP_ChannelQueueWindowMs") == 0) {
            gUserSettings.channelQueueWindowMs = atoi(value);
            DEBUG_LOG("Set NP_ChannelQueueWindowMs to " << gUserSettings.channelQueueWindowMs);
        } else if (strcmp(cvar, "NP_TargetingQueueWindowMs") == 0) {
            gUserSettings.targetingQueueWindowMs = atoi(value);
            DEBUG_LOG("Set NP_TargetingQueueWindowMs to " << gUserSettings.targetingQueueWindowMs);
        } else if (strcmp(cvar, "NP_CooldownQueueWindowMs") == 0) {
            gUserSettings.cooldownQueueWindowMs = atoi(value);
            DEBUG_LOG("Set NP_CooldownQueueWindowMs to " << gUserSettings.cooldownQueueWindowMs);

        } else if (strcmp(cvar, "NP_ChannelLatencyReductionPercentage") == 0) {
            gUserSettings.channelLatencyReductionPercentage = atoi(value);
            DEBUG_LOG(
                    "Set NP_ChannelLatencyReductionPercentage to " << gUserSettings.channelLatencyReductionPercentage);

        } else if (strcmp(cvar, "NP_NameplateDistance") == 0) {
            auto distance = std::stof(value);
            SetNameplateDistance(distance);
            DEBUG_LOG(
                    "Set NP_NameplateDistance to " << distance);
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
                updateFromCvar(cVarName, cVarValue);
            }
        } // original function handles errors

        return cvarSetOrig(luaPtr);
    }

    int *getCvar(const char *cvar) {
        auto const cvarLookup = hadesmem::detail::AliasCast<CVarLookupT>(Offsets::CVarLookup);
        uintptr_t *cvarPtr = cvarLookup(cvar);

        if (cvarPtr) {
            return reinterpret_cast<int *>(cvarPtr +
                                           10); // get intValue from CVar which is consistent, strValue more complicated
        }
        return nullptr;
    }

    void loadUserVar(const char *cvar) {
        int *value = getCvar(cvar);
        if (value) {
            updateFromCvar(cvar, std::to_string(*value).c_str());
        } else {
            DEBUG_LOG("Using default value for " << cvar);
        }
    }

    void loadConfig() {
        gStartTime = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count());

        try {
            // remove/rename previous logs
            remove("nampower_debug.log.3");
            rename("nampower_debug.log.2", "nampower_debug.log.3");
            rename("nampower_debug.log.1", "nampower_debug.log.2");
            rename("nampower_debug.log", "nampower_debug.log.1");
        } catch (...) {
            // ignore any exceptions during log rotation
        }


        // open new log file
        debugLogFile.open("nampower_debug.log");

        DEBUG_LOG("Loading nampower v" << MAJOR_VERSION << "." << MINOR_VERSION << "." << PATCH_VERSION);

        // default values
        gUserSettings.queueCastTimeSpells = true;
        gUserSettings.queueInstantSpells = true;
        gUserSettings.queueChannelingSpells = true;
        gUserSettings.queueTargetingSpells = true;
        gUserSettings.queueOnSwingSpells = false;
        gUserSettings.queueSpellsOnCooldown = true;

        gUserSettings.interruptChannelsOutsideQueueWindow = false;

        gUserSettings.retryServerRejectedSpells = true;
        gUserSettings.quickcastTargetingSpells = false;
        gUserSettings.replaceMatchingNonGcdCategory = false;
        gUserSettings.optimizeBufferUsingPacketTimings = false;

        gUserSettings.preventRightClickTargetChange = false;

        gUserSettings.doubleCastToEndChannelEarly = false;

        gUserSettings.minBufferTimeMs = 55; // time in ms to buffer cast to minimize server failure
        gUserSettings.nonGcdBufferTimeMs = 100; // time in ms to buffer non-GCD spells to minimize server failure
        gUserSettings.maxBufferIncreaseMs = 30;

        gUserSettings.spellQueueWindowMs = 500; // time in ms before cast to allow queuing spells
        gUserSettings.onSwingBufferCooldownMs = 500; // time in ms to wait before queuing on swing spell after a swing
        gUserSettings.channelQueueWindowMs = 1500; // time in ms before channel ends to allow queuing spells
        gUserSettings.targetingQueueWindowMs = 500; // time in ms before cast to allow targeting
        gUserSettings.cooldownQueueWindowMs = 250; // time in ms before cooldown is up to allow queuing spells

        gUserSettings.channelLatencyReductionPercentage = 75; // percent of latency to reduce channel time by

        char defaultTrue[] = "1";
        char defaultFalse[] = "0";

        DEBUG_LOG("Registering/Loading CVars");

        // register cvars
        auto const CVarRegister = hadesmem::detail::AliasCast<CVarRegisterT>(Offsets::RegisterCVar);

        char NP_QueueCastTimeSpells[] = "NP_QueueCastTimeSpells";
        CVarRegister(NP_QueueCastTimeSpells, // name
                     nullptr, // help
                     0,  // unk1
                     gUserSettings.queueCastTimeSpells ? defaultTrue : defaultFalse, // default value address
                     nullptr, // callback
                     5, // category
                     0,  // unk2
                     0); // unk3

        char NP_QueueInstantSpells[] = "NP_QueueInstantSpells";
        CVarRegister(NP_QueueInstantSpells, // name
                     nullptr, // help
                     0,  // unk1
                     gUserSettings.queueInstantSpells ? defaultTrue : defaultFalse, // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char NP_QueueChannelingSpells[] = "NP_QueueChannelingSpells";
        CVarRegister(NP_QueueChannelingSpells, // name
                     nullptr, // help
                     0,  // unk1
                     gUserSettings.queueChannelingSpells ? defaultTrue : defaultFalse, // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char NP_QueueTargetingSpells[] = "NP_QueueTargetingSpells";
        CVarRegister(NP_QueueTargetingSpells, // name
                     nullptr, // help
                     0,  // unk1
                     gUserSettings.queueTargetingSpells ? defaultTrue : defaultFalse, // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char NP_QueueOnSwingSpells[] = "NP_QueueOnSwingSpells";
        CVarRegister(NP_QueueOnSwingSpells, // name
                     nullptr, // help
                     0,  // unk1
                     gUserSettings.queueOnSwingSpells ? defaultTrue : defaultFalse, // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char NP_QueueSpellsOnCooldown[] = "NP_QueueSpellsOnCooldown";
        CVarRegister(NP_QueueSpellsOnCooldown, // name
                     nullptr, // help
                     0,  // unk1
                     gUserSettings.queueSpellsOnCooldown ? defaultTrue : defaultFalse, // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char NP_InterruptChannelsOutsideQueueWindow[] = "NP_InterruptChannelsOutsideQueueWindow";
        CVarRegister(NP_InterruptChannelsOutsideQueueWindow, // name
                     nullptr, // help
                     0,  // unk1
                     gUserSettings.interruptChannelsOutsideQueueWindow ? defaultTrue
                                                                       : defaultFalse, // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char NP_RetryServerRejectedSpells[] = "NP_RetryServerRejectedSpells";
        CVarRegister(NP_RetryServerRejectedSpells, // name
                     nullptr, // help
                     0,  // unk1
                     gUserSettings.retryServerRejectedSpells ? defaultTrue : defaultFalse, // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char NP_QuickcastTargetingSpells[] = "NP_QuickcastTargetingSpells";
        CVarRegister(NP_QuickcastTargetingSpells, // name
                     nullptr, // help
                     0,  // unk1
                     gUserSettings.quickcastTargetingSpells ? defaultTrue : defaultFalse, // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char NP_MinBufferTimeMs[] = "NP_MinBufferTimeMs";
        CVarRegister(NP_MinBufferTimeMs, // name
                     nullptr, // help
                     0,  // unk1
                     std::to_string(gUserSettings.minBufferTimeMs).c_str(), // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char NP_NonGcdBufferTimeMs[] = "NP_NonGcdBufferTimeMs";
        CVarRegister(NP_NonGcdBufferTimeMs, // name
                     nullptr, // help
                     0,  // unk1
                     std::to_string(gUserSettings.nonGcdBufferTimeMs).c_str(), // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char NP_MaxBufferIncreaseMs[] = "NP_MaxBufferIncreaseMs";
        CVarRegister(NP_MaxBufferIncreaseMs, // name
                     nullptr, // help
                     0,  // unk1
                     std::to_string(gUserSettings.maxBufferIncreaseMs).c_str(), // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char NP_SpellQueueWindowMs[] = "NP_SpellQueueWindowMs";
        CVarRegister(NP_SpellQueueWindowMs, // name
                     nullptr, // help
                     0,  // unk1
                     std::to_string(gUserSettings.spellQueueWindowMs).c_str(), // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char NP_ChannelQueueWindowMs[] = "NP_ChannelQueueWindowMs";
        CVarRegister(NP_ChannelQueueWindowMs, // name
                     nullptr, // help
                     0,  // unk1
                     std::to_string(gUserSettings.channelQueueWindowMs).c_str(), // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char NP_TargetingQueueWindowMs[] = "NP_TargetingQueueWindowMs";
        CVarRegister(NP_TargetingQueueWindowMs, // name
                     nullptr, // help
                     0,  // unk1
                     std::to_string(gUserSettings.targetingQueueWindowMs).c_str(), // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char NP_CooldownQueueWindowMs[] = "NP_CooldownQueueWindowMs";
        CVarRegister(NP_CooldownQueueWindowMs, // name
                     nullptr, // help
                     0,  // unk1
                     std::to_string(gUserSettings.cooldownQueueWindowMs).c_str(), // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char NP_OnSwingBufferCooldownMs[] = "NP_OnSwingBufferCooldownMs";
        CVarRegister(NP_OnSwingBufferCooldownMs, // name
                     nullptr, // help
                     0,  // unk1
                     std::to_string(gUserSettings.onSwingBufferCooldownMs).c_str(), // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char NP_ReplaceMatchingNonGcdCategory[] = "NP_ReplaceMatchingNonGcdCategory";
        CVarRegister(NP_ReplaceMatchingNonGcdCategory, // name
                     nullptr, // help
                     0,  // unk1
                     gUserSettings.replaceMatchingNonGcdCategory ? defaultTrue : defaultFalse, // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char NP_OptimizeBufferUsingPacketTimings[] = "NP_OptimizeBufferUsingPacketTimings";
        CVarRegister(NP_OptimizeBufferUsingPacketTimings, // name
                     nullptr, // help
                     0,  // unk1
                     gUserSettings.optimizeBufferUsingPacketTimings ? defaultTrue
                                                                    : defaultFalse, // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char NP_PreventRightClickTargetChange[] = "NP_PreventRightClickTargetChange";
        CVarRegister(NP_PreventRightClickTargetChange, // name
                     nullptr, // help
                     0,  // unk1
                     gUserSettings.preventRightClickTargetChange ? defaultTrue : defaultFalse, // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char NP_DoubleCastToEndChannelEarly[] = "NP_DoubleCastToEndChannelEarly";
        CVarRegister(NP_DoubleCastToEndChannelEarly, // name
                     nullptr, // help
                     0,  // unk1
                     gUserSettings.doubleCastToEndChannelEarly ? defaultTrue : defaultFalse, // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char NP_ChannelLatencyReductionPercentage[] = "NP_ChannelLatencyReductionPercentage";
        CVarRegister(NP_ChannelLatencyReductionPercentage, // name
                     nullptr, // help
                     0,  // unk1
                     std::to_string(gUserSettings.channelLatencyReductionPercentage).c_str(), // default value address
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        char NP_NameplateDistance[] = "NP_NameplateDistance";
        CVarRegister(NP_NameplateDistance, // name
                     nullptr, // help
                     0,  // unk1
                     std::to_string(GetNameplateDistance()).c_str(), // use the game's DAT value as the default
                     nullptr, // callback
                     1, // category
                     0,  // unk2
                     0); // unk3

        // update from cvars
        loadUserVar("NP_QueueCastTimeSpells");
        loadUserVar("NP_QueueInstantSpells");
        loadUserVar("NP_QueueOnSwingSpells");
        loadUserVar("NP_QueueChannelingSpells");
        loadUserVar("NP_QueueTargetingSpells");
        loadUserVar("NP_QueueSpellsOnCooldown");

        loadUserVar("NP_InterruptChannelsOutsideQueueWindow");

        loadUserVar("NP_RetryServerRejectedSpells");
        loadUserVar("NP_QuickcastTargetingSpells");
        loadUserVar("NP_ReplaceMatchingNonGcdCategory");
        loadUserVar("NP_OptimizeBufferUsingPacketTimings");

        loadUserVar("NP_PreventRightClickTargetChange");

        loadUserVar("NP_DoubleCastToEndChannelEarly");

        loadUserVar("NP_MinBufferTimeMs");
        loadUserVar("NP_NonGcdBufferTimeMs");
        loadUserVar("NP_MaxBufferIncreaseMs");

        loadUserVar("NP_SpellQueueWindowMs");
        loadUserVar("NP_ChannelQueueWindowMs");
        loadUserVar("NP_TargetingQueueWindowMs");
        loadUserVar("NP_OnSwingBufferCooldownMs");
        loadUserVar("NP_CooldownQueueWindowMs");

        loadUserVar("NP_ChannelLatencyReductionPercentage");

        loadUserVar("NP_NameplateDistance");

        gBufferTimeMs = gUserSettings.minBufferTimeMs;
    }

    void initCustomEvents() {
        auto strPtr = reinterpret_cast<uintptr_t *>(Offsets::QueueEventStringPtr);
        const char *SPELL_QUEUE_EVENT = "SPELL_QUEUE_EVENT";
        // Make 0x00BE175C which is the unused event string ptr point to SPELL_QUEUE_EVENT (369)
        *strPtr = reinterpret_cast<uintptr_t>(SPELL_QUEUE_EVENT);

        strPtr = reinterpret_cast<uintptr_t *>(Offsets::CastEventStringPtr);
        const char *SPELL_CAST_EVENT = "SPELL_CAST_EVENT";
        // Make 0X00BE1A08 which is the unused event string ptr point to SPELL_CAST_EVENT (540)
        *strPtr = reinterpret_cast<uintptr_t>(SPELL_CAST_EVENT);

        strPtr = reinterpret_cast<uintptr_t *>(Offsets::SpellDamageEventSelfStringPtr);
        const char *SPELL_DAMAGE_EVENT_SELF = "SPELL_DAMAGE_EVENT_SELF";
        // Make 0X00BE1A2C which is the unused event string ptr point to SPELL_DAMAGE_EVENT_SELF (549)
        *strPtr = reinterpret_cast<uintptr_t>(SPELL_DAMAGE_EVENT_SELF);

        strPtr = reinterpret_cast<uintptr_t *>(Offsets::SpellDamageEventOtherStringPtr);
        const char *SPELL_DAMAGE_EVENT_OTHER = "SPELL_DAMAGE_EVENT_OTHER";
        // Make 0X00BE1A30 which is the unused event string ptr point to SPELL_DAMAGE_EVENT_OTHER (550)
        *strPtr = reinterpret_cast<uintptr_t>(SPELL_DAMAGE_EVENT_OTHER);
    }

    // Template function to simplify hook initialization with specific storage
    template<typename FuncT, typename HookT>
    std::unique_ptr<hadesmem::PatchDetour<FuncT>> createHook(const hadesmem::Process& process, Offsets offset, HookT hookFunc) {
        auto const originalFunc = hadesmem::detail::AliasCast<FuncT>(offset);
        auto detour = std::make_unique<hadesmem::PatchDetour<FuncT>>(process, originalFunc, hookFunc);
        detour->Apply();
        return detour;
    }

    void initHooks() {
        const hadesmem::Process process(::GetCurrentProcessId());

        initCustomEvents();

        gSetCVarDetour = createHook<SetCVarT>(process, Offsets::Script_SetCVar, &Script_SetCVarHook);
        gCastDetour = createHook<CastSpellT>(process, Offsets::Spell_C_CastSpell, &Spell_C_CastSpellHook);
        gSendCastDetour = createHook<SendCastT>(process, Offsets::SendCast, &SendCastHook);
        gCancelSpellDetour = createHook<CancelSpellT>(process, Offsets::CancelSpell, &CancelSpellHook);
        gCastResultHandlerDetour = createHook<PacketHandlerT>(process, Offsets::CastResultHandler, &CastResultHandlerHook);
        gSpellStartHandlerDetour = createHook<FastCallPacketHandlerT>(process, Offsets::SpellStartHandler, &SpellStartHandlerHook);
        gPeriodicAuraLogHandlerDetour = createHook<FastCallPacketHandlerT>(process, Offsets::PeriodicAuraLogHandler, &PeriodicAuraLogHandlerHook);
        gSpellNonMeleeDmgLogHandlerDetour = createHook<FastCallPacketHandlerT>(process, Offsets::SpellNonMeleeDmgLogHandler, &SpellNonMeleeDmgLogHandlerHook);
        gSpellChannelStartHandlerDetour = createHook<PacketHandlerT>(process, Offsets::SpellChannelStartHandler, &SpellChannelStartHandlerHook);
        gSpellChannelUpdateHandlerDetour = createHook<PacketHandlerT>(process, Offsets::SpellChannelUpdateHandler, &SpellChannelUpdateHandlerHook);
        gSpellFailedDetour = createHook<Spell_C_SpellFailedT>(process, Offsets::Spell_C_SpellFailed, &Spell_C_SpellFailedHook);
        gSpellGoDetour = createHook<SpellGoT>(process, Offsets::SpellGo, &SpellGoHook);
        gSpellDelayedDetour = createHook<PacketHandlerT>(process, Offsets::SpellDelayed, &SpellDelayedHook);
        gSpellTargetUnitDetour = createHook<LuaScriptT>(process, Offsets::Script_SpellTargetUnit, &Script_SpellTargetUnitHook);
        gSpellStopCastingDetour = createHook<LuaScriptT>(process, Offsets::Script_SpellStopCasting, &Script_SpellStopCastingHook);
        gSpell_C_TargetSpellDetour = createHook<Spell_C_TargetSpellT>(process, Offsets::Spell_C_TargetSpell, &Spell_C_TargetSpellHook);
        gCastSpellByNameNoQueueDetour = createHook<LuaScriptT>(process, Offsets::Script_CastSpellByNameNoQueue, Script_CastSpellByNameNoQueue);
        gQueueSpellByNameDetour = createHook<LuaScriptT>(process, Offsets::Script_QueueSpellByName, Script_QueueSpellByName);
        qQueueScriptDetour = createHook<LuaScriptT>(process, Offsets::Script_QueueScript, Script_QueueScript);
        gIsSpellInRangeDetour = createHook<LuaScriptT>(process, Offsets::Script_IsSpellInRange, Script_IsSpellInRange);
        gIsSpellUsableDetour = createHook<LuaScriptT>(process, Offsets::Script_IsSpellUsable, Script_IsSpellUsable);
        gGetCurrentCastingInfoDetour = createHook<LuaScriptT>(process, Offsets::Script_GetCurrentCastingInfo, Script_GetCurrentCastingInfo);
        gGetSpellIdForNameDetour = createHook<LuaScriptT>(process, Offsets::Script_GetSpellIdForName, Script_GetSpellIdForName);
        gGetSpellNameAndRankForIdDetour = createHook<LuaScriptT>(process, Offsets::Script_GetSpellNameAndRankForId, Script_GetSpellNameAndRankForId);
        gGetSpellSlotAndTypeForNameDetour = createHook<LuaScriptT>(process, Offsets::Script_GetSpellSlotTypeIdForName, Script_GetSpellSlotTypeIdForName);
        gOnSpriteRightClickDetour = createHook<OnSpriteRightClickT>(process, Offsets::OnSpriteRightClick, OnSpriteRightClickHook);
        gChannelStopCastingNextTickDetour = createHook<LuaScriptT>(process, Offsets::Script_ChannelStopCastingNextTick, Script_ChannelStopCastingNextTick);
        gGetNampowerVersionDetour = createHook<LuaScriptT>(process, Offsets::Script_GetNampowerVersion, Script_GetNampowerVersion);
        gGetItemLevelDetour = createHook<LuaScriptT>(process, Offsets::Script_GetItemLevel, Script_GetItemLevel);
        gIEndSceneDetour = createHook<ISceneEndT>(process, Offsets::ISceneEndPtr, &ISceneEndHook);
    }

    void SpellVisualsInitializeHook(hadesmem::PatchDetourBase *detour) {
        auto const spellVisualsInitialize = detour->GetTrampolineT<SpellVisualsInitializeT>();
        spellVisualsInitialize();
        loadConfig();
        initHooks();
    }

    void FrameScript_CreateEventsHook(hadesmem::PatchDetourBase *detour, int param_1, uint32_t maxEventId) {
        auto const createEvents = detour->GetTrampolineT<FrameScript_CreateEventsT>();

        if (maxEventId == 549) {
            maxEventId = 551; // add two more events
        }

        createEvents(param_1, maxEventId);
    }

    void LoadScriptFunctionsHook(hadesmem::PatchDetourBase *detour) {
        auto const loadScriptFunctions = detour->GetTrampolineT<LoadScriptFunctionsT>();
        loadScriptFunctions();

        // register our own lua functions
        DEBUG_LOG("Registering Custom Lua functions");
        char queueSpellByName[] = "QueueSpellByName";
        RegisterLuaFunction(queueSpellByName, reinterpret_cast<uintptr_t *>(Offsets::Script_QueueSpellByName));

        char castSpellByNameNoQueue[] = "CastSpellByNameNoQueue";
        RegisterLuaFunction(castSpellByNameNoQueue,
                            reinterpret_cast<uintptr_t *>(Offsets::Script_CastSpellByNameNoQueue));

        char queueScript[] = "QueueScript";
        RegisterLuaFunction(queueScript, reinterpret_cast<uintptr_t *>(Offsets::Script_QueueScript));

        char isSpellInRange[] = "IsSpellInRange";
        RegisterLuaFunction(isSpellInRange, reinterpret_cast<uintptr_t *>(Offsets::Script_IsSpellInRange));

        char isSpellUsable[] = "IsSpellUsable";
        RegisterLuaFunction(isSpellUsable, reinterpret_cast<uintptr_t *>(Offsets::Script_IsSpellUsable));

        char getCurrentCastingInfo[] = "GetCurrentCastingInfo";
        RegisterLuaFunction(getCurrentCastingInfo,
                            reinterpret_cast<uintptr_t *>(Offsets::Script_GetCurrentCastingInfo));

        char getSpellIdForName[] = "GetSpellIdForName";
        RegisterLuaFunction(getSpellIdForName,
                            reinterpret_cast<uintptr_t *>(Offsets::Script_GetSpellIdForName));

        char getSpellNameAndRankForId[] = "GetSpellNameAndRankForId";
        RegisterLuaFunction(getSpellNameAndRankForId,
                            reinterpret_cast<uintptr_t *>(Offsets::Script_GetSpellNameAndRankForId));

        char getSpellSlotTypeIdForName[] = "GetSpellSlotTypeIdForName";
        RegisterLuaFunction(getSpellSlotTypeIdForName,
                            reinterpret_cast<uintptr_t *>(Offsets::Script_GetSpellSlotTypeIdForName));

        char channelStopCastingNextTick[] = "ChannelStopCastingNextTick";
        RegisterLuaFunction(channelStopCastingNextTick,
                            reinterpret_cast<uintptr_t *>(Offsets::Script_ChannelStopCastingNextTick));

        char getNampowerVersion[] = "GetNampowerVersion";
        RegisterLuaFunction(getNampowerVersion, reinterpret_cast<uintptr_t *>(Offsets::Script_GetNampowerVersion));

        char getItemILevel[] = "GetItemLevel";
        RegisterLuaFunction(getItemILevel, reinterpret_cast<uintptr_t *>(Offsets::Script_GetItemLevel));
    }

    std::once_flag loadFlag;

    void load() {
        std::call_once(loadFlag, []() {
                           // hook spell visuals initialize
                           const hadesmem::Process process(::GetCurrentProcessId());

                           auto const spellVisualsInitOrig = hadesmem::detail::AliasCast<SpellVisualsInitializeT>(
                                   Offsets::SpellVisualsInitialize);
                           gSpellVisualsInitDetour = std::make_unique<hadesmem::PatchDetour<SpellVisualsInitializeT >>(process,
                                                                                                                       spellVisualsInitOrig,
                                                                                                                       &SpellVisualsInitializeHook);
                           gSpellVisualsInitDetour->Apply();

                           auto const loadScriptFunctionsOrig = hadesmem::detail::AliasCast<LoadScriptFunctionsT>(
                                   Offsets::LoadScriptFunctions);
                           gLoadScriptFunctionsDetour = std::make_unique<hadesmem::PatchDetour<LoadScriptFunctionsT >>(process,
                                                                                                                       loadScriptFunctionsOrig,
                                                                                                                       &LoadScriptFunctionsHook);
                           gLoadScriptFunctionsDetour->Apply();

                           auto const createEventsOrig = hadesmem::detail::AliasCast<FrameScript_CreateEventsT>(
                                   Offsets::FrameScript_CreateEvents);
                           gCreateEventsDetour = std::make_unique<hadesmem::PatchDetour<FrameScript_CreateEventsT >>(process,
                                                                                                                     createEventsOrig,
                                                                                                                     &FrameScript_CreateEventsHook);
                           gCreateEventsDetour->Apply();
                       }
        );
    }

}

extern "C" __declspec(dllexport) uint32_t Load() {
    Nampower::load();
    return EXIT_SUCCESS;
}
