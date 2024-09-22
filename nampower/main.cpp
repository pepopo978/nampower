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

using std::this_thread::sleep_for;

// TODO uncomment once ready for release
//#ifdef _DEBUG
//std::ofstream debugLogFile("nampower_debug.log");
//#define DEBUG_LOG(msg) debugLogFile << "[DEBUG]" << GetTime() << ": " << msg << std::endl
//#else
//#define DEBUG_LOG(msg) // No-op in release mode
//#endif
std::ofstream debugLogFile("nampower_debug.log");
#define DEBUG_LOG(msg) debugLogFile << "[DEBUG]" << GetTime() << ": " << msg << std::endl

BOOL WINAPI DllMain(HINSTANCE, DWORD, void *);

namespace {
    static const DWORD MAX_TIME_SINCE_LAST_CAST_FOR_QUEUE = 10000; // time limit in ms after which queued casts are ignored in errors
    static const DWORD DYNAMIC_BUFFER_INCREMENT = 5; // amount to adjust buffer in ms on errors/lack of errors
    static const DWORD BUFFER_INCREASE_FREQUENCY = 5000; // time in ms between changes to raise buffer
    static const DWORD BUFFER_DECREASE_FREQUENCY = 10000; // time in ms between changes to lower buffer

    static DWORD gStartTime;
    static DWORD gCastEndMs;
    static DWORD gGCDEndMs;
    static DWORD gMaxBufferIncrease;
    static DWORD gBufferTimeMs;   // adjusts dynamically depending on errors
    static DWORD gInstantCastBufferTimeMs;   // adjusts dynamically depending on errors
    static DWORD gMaxInstantCastBufferIncrease;
    static DWORD gMinBufferTimeMs; // set by user
    static DWORD gMinInstantCastBufferTimeMs; // set by user
    static DWORD gOnSwingBufferCooldownMs;
    static DWORD gSpellQueueWindowMs;
    static DWORD gChannelQueueWindowMs;
    static DWORD gTargetingQueueWindowMs;
    static DWORD gLastCast;
    static DWORD gLastCastStartTime;
    static DWORD gLastChannelStartTime;
    static DWORD gLastOnSwingCastTime;

    static DWORD gLastErrorTimeMs;
    static DWORD gLastBufferIncreaseTimeMs;
    static DWORD gLastBufferDecreaseTimeMs;

    // true a spell is queued
    static std::atomic<bool> gSpellQueued;
    static std::atomic<bool> gChannelQueued;
    static std::atomic<bool> gOnSwingQueued;

    // true when we are simulating a server-based spell cancel to reset the cast bar
    static std::atomic<bool> gCancelling;

    // true when the current spell cancellation requires that we notify the server
    // (a client side cancellation of a non-instant cast spell)
    static std::atomic<bool> gNotifyServer;

    // true when we are in the middle of an attempt to cast a spell
    static std::atomic<bool> gCasting;
    static std::atomic<bool> gChanneling;
    static int gChannelDuration;
    static int gChannelCastCount;

    static hadesmem::PatchDetourBase *onSwingDetour;
    static hadesmem::PatchDetourBase *lastDetour;
    static void *lastUnit;
    static int lastSpellId;
    static int lastOnSwingSpellId;
    static void *lastItem;
    static std::uint32_t lastCastTime;
    static std::uint64_t lastGuid;

    using CastSpellT = bool (__fastcall *)(void *, int, void *, std::uint64_t);
    using SendCastT = void (__fastcall *)(game::SpellCast *);
    using CancelSpellT = int (__fastcall *)(bool, bool, game::SpellCastResult);
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
    using Script_GetGUIDFromNameT = std::uint64_t (__fastcall *)(const char *);
    using lua_isstringT = bool (__fastcall *)(uintptr_t *, int);
    using lua_tostringT = const char *(__fastcall *)(uintptr_t *, int);

    std::unique_ptr<hadesmem::PatchDetour<CastSpellT>> gCastDetour;
    std::unique_ptr<hadesmem::PatchDetour<SendCastT>> gSendCastDetour;
    std::unique_ptr<hadesmem::PatchDetour<CancelSpellT>> gCancelSpellDetour;
    std::unique_ptr<hadesmem::PatchDetour<SignalEventT>> gSignalEventDetour;
    std::unique_ptr<hadesmem::PatchDetour<PacketHandlerT>> gSpellDelayedDetour;
    std::unique_ptr<hadesmem::PatchDetour<Spell_C_SpellFailedT>> gSpellFailedDetour;
    std::unique_ptr<hadesmem::PatchRaw> gCastbarPatch;
    std::unique_ptr<hadesmem::PatchDetour<ISceneEndT>> gIEndSceneDetour;
    std::unique_ptr<hadesmem::PatchDetour<EndSceneT>> gEndSceneDetour;
    std::unique_ptr<hadesmem::PatchDetour<SpellStartHandlerT>> gSpellStartHandlerDetour;
    std::unique_ptr<hadesmem::PatchDetour<SpellChannelStartHandlerT>> gSpellChannelStartHandlerDetour;
    std::unique_ptr<hadesmem::PatchDetour<SpellChannelUpdateHandlerT>> gSpellChannelUpdateHandlerDetour;
    std::unique_ptr<hadesmem::PatchDetour<CastResultHandlerT>> gCastResultHandlerDetour;
    std::unique_ptr<hadesmem::PatchDetour<SpellFailedHandlerT>> gSpellFailedHandlerDetour;
    std::unique_ptr<hadesmem::PatchDetour<Spell_C_GetAutoRepeatingSpellT>> gSpell_C_GetAutoRepeatingSpellDetour;
    std::unique_ptr<hadesmem::PatchDetour<SpellGoT>> gSpellGoDetour;
    std::unique_ptr<hadesmem::PatchDetour<SpellTargetUnitT>> gSpellTargetUnitDetour;
    std::unique_ptr<hadesmem::PatchDetour<Spell_C_HandleSpriteClickT>> gSpell_C_HandleSpriteClickDetour;

    DWORD GetTime() {
        return static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count()) - gStartTime;
    }

    bool SpellIsOnGCD(const game::SpellRec *spell) {
        return spell->StartRecoveryCategory == 133;
    }

    bool SpellIsChanneling(const game::SpellRec *spell) {
        return spell->AttributesEx & game::SPELL_ATTR_EX_IS_CHANNELED ||
               spell->AttributesEx & game::SPELL_ATTR_EX_IS_SELF_CHANNELED;
    }

    bool SpellIsOnSwing(const game::SpellRec *spell) {
        return spell->Attributes & game::SPELL_ATTR_ON_NEXT_SWING_1;
    }

    void BeginCast(DWORD currentTime, std::uint32_t castTime, const game::SpellRec *spell) {
        auto bufferTime = castTime == 0 ? 0 : gBufferTimeMs;

        gLastCastStartTime = castTime;
        gChanneling = SpellIsChanneling(spell);

        if (gChanneling) {
            auto const duration = game::GetDurationObject(spell->DurationIndex);
            gChannelDuration = duration->m_Duration2;
        }


        if (SpellIsOnGCD(spell)) {
            gCastEndMs = castTime ? currentTime + castTime + gBufferTimeMs : 0;

            auto gcd = spell->StartRecoveryTime;
            if (gcd == 0) {
                gcd = 1500;
            }
            gGCDEndMs = currentTime + spell->StartRecoveryTime;
            DEBUG_LOG("BeginCast " << game::GetSpellName(spell->Id)
                                   << " cast time: " << castTime << " buffer: "
                                   << bufferTime << " GCD: " << gcd);
        } else {
            DEBUG_LOG("BeginCast " << game::GetSpellName(spell->Id)
                                   << " cast time: " << castTime << " buffer: "
                                   << bufferTime << " NO GCD");
            gGCDEndMs = 0;
        }

        // check if we can lower buffers
        if (currentTime - gLastBufferDecreaseTimeMs > BUFFER_DECREASE_FREQUENCY) {
            if (gBufferTimeMs > gMinBufferTimeMs) {
                gBufferTimeMs -= DYNAMIC_BUFFER_INCREMENT;
                DEBUG_LOG("Decreasing buffer to " << gBufferTimeMs);
            }

            if (gInstantCastBufferTimeMs > gMinInstantCastBufferTimeMs) {
                gInstantCastBufferTimeMs -= DYNAMIC_BUFFER_INCREMENT;
                DEBUG_LOG("Decreasing instant cast buffer to " << gInstantCastBufferTimeMs);
            }

            gLastBufferDecreaseTimeMs = currentTime; // update the last error time to prevent lowering buffer too often
        }

        gLastCast = currentTime;
        //JT: Use the notification from server to activate cast bar - it is needed to have HealComm work.
        //if (castTime)
        //{
        //    void(*signalEvent)(std::uint32_t, const char *, ...) = reinterpret_cast<decltype(signalEvent)>(Offsets::SignalEventParam);
        //    signalEvent(game::Events::SPELLCAST_START, "%s%d", game::GetSpellName(spellId), castTime);
        //}
    }

    bool CastSpellHook(hadesmem::PatchDetourBase *detour, void *unit, int spellId, void *item, std::uint64_t guid) {
        auto const spell = game::GetSpellInfo(spellId);
        auto const spellIsOnSwing = SpellIsOnSwing(spell);
        auto const spellName = game::GetSpellName(spellId);

        DEBUG_LOG("Cast spell guid " << guid << " spell " << spellName);

        // on swing spells are independent of cast bar / gcd, handle them separately
        if (spellIsOnSwing) {
            lastOnSwingSpellId = spellId;

            // try to cast the spell
            auto const castSpell = detour->GetTrampolineT<CastSpellT>();
            auto ret = castSpell(unit, spellId, item, guid);

            if (!ret) {
                // if not in cooldown window
                if (GetTime() - gLastOnSwingCastTime > gOnSwingBufferCooldownMs) {
                    DEBUG_LOG("Queuing on swing spell " << spellName);
                    gOnSwingQueued = true;
                    // save the detour to trigger the cast again after the cooldown is up
                    onSwingDetour = detour;
                }
            } else {
                DEBUG_LOG("Successful on swing spell " << spellName);
                gOnSwingQueued = false;
                onSwingDetour = nullptr;
            }

            return ret;
        }

        lastUnit = unit;
        lastSpellId = spellId;
        lastItem = item;
        lastGuid = guid;

        auto const castTime = game::GetCastTime(unit, spellId);

        auto const spellOnGCD = SpellIsOnGCD(spell);
        auto const spellIsChanneling = SpellIsChanneling(spell);

        lastCastTime = castTime;

        gSpellQueued = false;
        gChannelQueued = false;

        auto const castSpell = detour->GetTrampolineT<CastSpellT>();
        auto currentTime = GetTime();
        auto remainingCastTime = gCastEndMs - currentTime;
        auto remainingGCD = gGCDEndMs - currentTime;

        auto const cursorMode = *reinterpret_cast<int *>(Offsets::CursorMode);
        if (spell->Targets == game::SpellTarget::TARGET_LOCATION_UNIT_POSITION && cursorMode == 1) {
            // if the spell is a ground target spell, allowing targeting while casting or waiting for gcd
            if ((remainingCastTime > 0 && remainingCastTime < gTargetingQueueWindowMs) ||
                (remainingGCD > 0 && remainingGCD < gTargetingQueueWindowMs)) {
                castSpell(unit, spellId, item, guid); // start the cast which will trigger targeting
            }
        } else if (remainingCastTime > 0 && remainingCastTime < gSpellQueueWindowMs) {
            DEBUG_LOG("Queuing for after cooldown: " << remainingCastTime << "ms " << spellName);
            gSpellQueued = true;

            // save the detour to trigger the cast again after the cooldown is up
            lastDetour = detour;

            return false;
        } else if (spellOnGCD && remainingGCD > 0 && remainingGCD < gSpellQueueWindowMs) {
            DEBUG_LOG("Queuing for after gcd: " << remainingGCD << "ms " << spellName);
            gSpellQueued = true;

            // save the detour to trigger the cast again after the cooldown is up
            lastDetour = detour;

            return false;
        } else if (spellIsChanneling && gChanneling) {
            // calculate the time remaining in the channel
            auto const remainingChannelTime = gChannelDuration - (currentTime - gLastChannelStartTime);

            if (remainingChannelTime < gChannelQueueWindowMs) {
                DEBUG_LOG("Queuing for after channeling " << spellName);
                gChannelQueued = true;

                // save the detour to trigger the cast again after the cooldown is up
                lastDetour = detour;

                return false;
            }
        }

        DEBUG_LOG("Attempt cast " << spellName << " , time elapsed since last cast " << currentTime - gLastCast);

        // is there a cooldown? (ignore for on swing spells)
        if (gCastEndMs) {
            // is it still active?
            if (gCastEndMs > currentTime) {
                DEBUG_LOG("Cooldown active " << gCastEndMs - currentTime << "ms remaining");
                return false;
            }

            gCastEndMs = 0;
        }
        // is there a GCD?
        if (spellOnGCD && gGCDEndMs) {
            // is it still active?
            if (gGCDEndMs > currentTime) {
                DEBUG_LOG("Gcd active " << gGCDEndMs - currentTime << "ms remaining");
                return false;
            }

            gGCDEndMs = 0;
        }


        gCasting = true;

        auto ret = castSpell(unit, spellId, item, guid);

        // if this is a trade skill or item enchant, do nothing further
        if (spell->Effect[0] == game::SpellEffects::SPELL_EFFECT_TRADE_SKILL ||
            spell->Effect[0] == game::SpellEffects::SPELL_EFFECT_ENCHANT_ITEM ||
            spell->Effect[0] == game::SpellEffects::SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY ||
            spell->Effect[0] == game::SpellEffects::SPELL_EFFECT_CREATE_ITEM)
            return ret;

        // haven't gotten spell result from the previous cast yet, probably due to latency.
        // simulate a cancel to clear the cast bar but only when there should be a cast time
        if (!ret && gLastCastStartTime > 0) {
            if(*reinterpret_cast<int *>(Offsets::SpellIsTargeting) == 0) {
                DEBUG_LOG("Canceling spell cast due to previous spell having cast time of " << gLastCastStartTime);

                gCancelling = true;
                //JT: Suggest replacing CancelSpell with InterruptSpell (the API called when moving during casting).
                // The address of InterruptSpell needs to be dug out. It could possibly fix the sometimes broken animations.
                auto const cancelSpell = reinterpret_cast<CancelSpellT>(Offsets::CancelSpell);
                cancelSpell(false, false, game::SPELL_FAILED_ERROR);

                gCancelling = false;

                // try again...
                ret = castSpell(unit, spellId, item, guid);
                if (ret) {
                    DEBUG_LOG("Retry cast successful, clearing queued error cast");
                    gSpellQueued = false;
                }
            } else {
                DEBUG_LOG("Initial cast failed, not canceling spell cast due to targeting");
            }
        }

        //JT: cursorMode == 2 is for clickcasting
        if (ret && !(spell->Attributes & game::SPELL_ATTR_RANGED) /* && cursorMode != 2 */) {
            BeginCast(GetTime(), castTime, spell);
        } else {
            DEBUG_LOG("Spell cast start failed castSpell returned " << ret << " spell " << spellId << " isRanged " <<
                                                                    !(spell->Attributes & game::SPELL_ATTR_RANGED));
        }

        gCasting = false;

        return ret;
    }

    int
    CancelSpellHook(hadesmem::PatchDetourBase *detour, bool failed, bool notifyServer, game::SpellCastResult reason) {
        gNotifyServer = notifyServer;

        auto const cancelSpell = detour->GetTrampolineT<CancelSpellT>();
        auto const ret = cancelSpell(failed, notifyServer, reason);

        return ret;
    }

    void SpellGoHook(hadesmem::PatchDetourBase *detour, uint64_t *casterGUID, uint64_t *targetGUID, uint32_t spellId,
                     game::CDataStore *spellData) {
        // only care about our own casts and lastSpellId(ignore spell procs)
        if (gChanneling && game::ClntObjMgrGetActivePlayer() == *casterGUID) {
            gChannelCastCount++;

            if (gChannelQueued || gSpellQueued) {
                // add extra 500ms to account for latency, channel updates every sec
                auto const elapsed = 500 + GetTime() - gLastChannelStartTime;

                DEBUG_LOG("Channel cast elapsed " << elapsed << " duration " << gChannelDuration);

                // if within 500ms of the end of the channel, trigger the queued cast
                if (elapsed > gChannelDuration) {
                    gChannelQueued = false;
                    gChanneling = false;
                    gChannelCastCount = 0;
                    DEBUG_LOG("Channel is complete, triggering queued cast of " << game::GetSpellName(lastSpellId));
                    CastSpellHook(lastDetour, lastUnit, lastSpellId, lastItem, lastGuid);
                }
            }
        } else if (gOnSwingQueued && game::ClntObjMgrGetActivePlayer() == *casterGUID) {
            // check if spell is on hit
            auto const spell = game::GetSpellInfo(spellId);
            if (spell->Attributes & game::SPELL_ATTR_ON_NEXT_SWING_1) {
                DEBUG_LOG("On swing spell " << game::GetSpellName(spellId) <<
                                            " resolved, casting queued on swing spell "
                                            << game::GetSpellName(lastOnSwingSpellId));
                gLastOnSwingCastTime = GetTime();
                gOnSwingQueued = false;
                CastSpellHook(onSwingDetour, lastUnit, lastOnSwingSpellId, lastItem, lastGuid);
            }
        }

        auto const spellGo = detour->GetTrampolineT<SpellGoT>();
        spellGo(casterGUID, targetGUID, spellId, spellData);
    }

    void Spell_C_SpellFailedHook(hadesmem::PatchDetourBase *detour, int spellId,
                                 game::SpellCastResult spellResult, int unk1, int unk2, char unk3) {
        if (lastDetour &&
            (spellResult == game::SpellCastResult::SPELL_FAILED_NOT_READY ||
             spellResult == game::SpellCastResult::SPELL_FAILED_ITEM_NOT_READY ||
             spellResult == game::SpellCastResult::SPELL_FAILED_SPELL_IN_PROGRESS)
                ) {
            auto const currentTime = GetTime();

            // ignore error spam
            if (currentTime - gLastErrorTimeMs < 200) {
                lastDetour = nullptr; // reset the last detour
                DEBUG_LOG("Cast failed code " << int(spellResult) << ", not queuing retry due to recent error at "
                                              << gLastErrorTimeMs);
                return;
            } else {
                DEBUG_LOG("Cast failed code " << int(spellResult) << ", queuing a retry");
            }
            gLastErrorTimeMs = currentTime;
            gSpellQueued = true;

            // check if we should increase the buffer time
            if (currentTime - gLastBufferIncreaseTimeMs > BUFFER_INCREASE_FREQUENCY) {
                if (lastCastTime == 0) {
                    // check if gInstantCastBufferTimeMs is already at max
                    if (gInstantCastBufferTimeMs - gMinInstantCastBufferTimeMs <
                        gMaxInstantCastBufferIncrease) {
                        gInstantCastBufferTimeMs += DYNAMIC_BUFFER_INCREMENT;
                        DEBUG_LOG("Increasing instant cast buffer to " << gInstantCastBufferTimeMs);
                        gLastBufferIncreaseTimeMs = currentTime;
                    }
                } else {
                    // check if gBufferTimeMs is already at max
                    if (gBufferTimeMs - gMinBufferTimeMs < gMaxBufferIncrease) {
                        gBufferTimeMs += DYNAMIC_BUFFER_INCREMENT;
                        DEBUG_LOG("Increasing buffer to " << gBufferTimeMs);
                        gLastBufferIncreaseTimeMs = currentTime;
                    }
                }
            }
        } else if (lastDetour) {
            DEBUG_LOG("Cast failed code " << int(spellResult) << " ignored");
        }

        auto const spellFailed = detour->GetTrampolineT<Spell_C_SpellFailedT>();
        spellFailed(spellId, spellResult, unk1, unk2, unk3);
    }

    int SpellChannelStartHandlerHook(hadesmem::PatchDetourBase *detour, int channelStart, game::CDataStore *dataPtr) {
        gLastChannelStartTime = GetTime();
        DEBUG_LOG("Channel start " << channelStart);
        if (channelStart) {
            gChanneling = true;
            gChannelCastCount = 0;
        }

        auto const spellChannelStartHandler = detour->GetTrampolineT<SpellChannelStartHandlerT>();
        return spellChannelStartHandler(channelStart, dataPtr);
    }

    int SpellChannelUpdateHandlerHook(hadesmem::PatchDetourBase *detour, int channelUpdate, game::CDataStore *dataPtr) {
        auto const elapsed = 500 + GetTime() - gLastChannelStartTime;
        DEBUG_LOG("Channel update elapsed " << elapsed << " duration " << gChannelDuration);

        if (elapsed > gChannelDuration) {
            DEBUG_LOG("Channel done");
            gChanneling = false;
            gChannelCastCount = 0;

            if (gChannelQueued) {
                gChannelQueued = false;
                DEBUG_LOG("Channeling ended, triggering queued cast");
                CastSpellHook(lastDetour, lastUnit, lastSpellId, lastItem, lastGuid);
            }
        }

        auto const spellChannelUpdateHandler = detour->GetTrampolineT<SpellChannelUpdateHandlerT>();
        return spellChannelUpdateHandler(channelUpdate, dataPtr);
    }

// events without parameters go here
    void SignalEventHook(hadesmem::PatchDetourBase *detour, game::Events eventId) {
        auto const currentTime = GetTime();

        // SPELLCAST_STOP means the cast started and then stopped.  it may or may not have completed.
        // this can happen by one of two conditions:
        // 1) it is caused by us in CastSpellHook() because the player is spamming
        //    casts and we have not yet received result of the last one (or by the
        //    client through some other means)
        // 2) the player is casting slowly enough (or possibly not at all) such that
        //    the result of the last cast arrives before our next attempt to cast
        // for scenario #1 we want to allow the event as it will reset the cast bar
        // on the last attempt.
        // for scenario #2, we have already reset the cast bar, and we do not want to
        // do it again because we may already be casting the next spell.


//        DEBUG_LOG("Event " << eventId);

        if (eventId == game::Events::SPELLCAST_STOP) {
            // if this is from the client, we don't care about anything else.  immediately stop
            // the cast bar and reset our internal cooldown.
            if (gNotifyServer) {
                DEBUG_LOG("Spellcast stop, resetting cooldown");
                gCastEndMs = 0;
                gGCDEndMs = 0;
                gChanneling = false;
                gChannelCastCount = 0;
            }

                // if this is from the server but it is happening too early, it is for one of two reasons.
                // 1) it is for the last cast, in which case we can ignore it
                // 2) it is for our current cast and the server decided to cast sooner than we expected
                //    this can happen from mage 8/8 t2 proc or presence of mind
            else if (!gCasting && !gCancelling && currentTime <= gCastEndMs) {
                DEBUG_LOG("Server triggered spellcast stop");
                return;
            }
        }
            // SPELLCAST_FAILED means the attempt was rejected by either the client or the server,
            // depending on the value of gNotifyServer.  if it was rejected by the server, this
            // could mean that our latency has decreased since the previous cast.  when that happens
            // the server perceives too little time as having passed to allow another cast.  i dont
            // think there is anything we can do about this except to honor the servers request to
            // abort the cast.  reset our cooldown and allow
        else if (eventId == game::Events::SPELLCAST_FAILED/* && !gNotifyServer*/ ||
                 //JT: !gNotifyServer breaks QuickHeal
                 eventId ==
                 game::Events::SPELLCAST_INTERRUPTED) //JT: moving to cancel the spell sends "SPELLCAST_INTERRUPTED"
        {
            gCastEndMs = 0;
            gGCDEndMs = 0;
            gCasting = false;
            gChanneling = false;
            gChannelCastCount = 0;
        }

        auto const signalEvent = detour->GetTrampolineT<SignalEventT>();
        signalEvent(eventId);
    }

    int SpellDelayedHook(hadesmem::PatchDetourBase *detour, int opCode, game::CDataStore *packet) {
        auto const spellDelayed = detour->GetTrampolineT<PacketHandlerT>();

        auto const rpos = packet->m_read;

        auto const guid = packet->Get<std::uint64_t>();
        auto const delay = packet->Get<std::uint32_t>();

        packet->m_read = rpos;

        auto const activePlayer = game::ClntObjMgrGetActivePlayer();

        if (guid == activePlayer) {
            auto const currentTime = GetTime();

            // if we are casting a spell and it was delayed, update our own state so we do not allow a cast too soon
            if (currentTime < gCastEndMs) {
                gCastEndMs += delay;
            }
        }

        return spellDelayed(opCode, packet);
    }

    bool SpellTargetUnitHook(hadesmem::PatchDetourBase *detour, uintptr_t *unitStr) {
        auto const spellTargetUnit = detour->GetTrampolineT<SpellTargetUnitT>();

        // check if valid string
        auto const lua_isstring = reinterpret_cast<lua_isstringT>(Offsets::lua_isstring);
        if(lua_isstring(unitStr, 1)) {
            auto const lua_tostring = reinterpret_cast<lua_tostringT>(Offsets::lua_tostring);
            auto const unitName = lua_tostring(unitStr, 1);

            auto const getGUIDFromName = reinterpret_cast<Script_GetGUIDFromNameT>(Offsets::Script_GetGUIDFromName);
            auto const guid = getGUIDFromName(unitName);
            if (guid) {
                DEBUG_LOG("Spell target unit " << unitName << " guid " << guid);
                lastGuid = guid;
            }
        }

        return spellTargetUnit(unitStr);
    }

    int *ISceneEndHook(hadesmem::PatchDetourBase *detour, uintptr_t *ptr) {
        auto const iSceneEnd = detour->GetTrampolineT<ISceneEndT>();

//        auto const getAutoRepeatingSpell = reinterpret_cast<Spell_C_GetAutoRepeatingSpellT>(Offsets::Spell_C_GetAutoRepeatingSpell);
//        auto const autoRepeatingSpell = getAutoRepeatingSpell();
//        if (autoRepeatingSpell) {
//            DEBUG_LOG("Auto repeating spell " << autoRepeatingSpell);
//        }

        if (lastDetour && gSpellQueued) {
            auto currentTime = GetTime();

            // get max of cooldown and gcd
            auto delay = gCastEndMs > gGCDEndMs ? gCastEndMs : gGCDEndMs;

            if (lastCastTime == 0) {
                delay += gInstantCastBufferTimeMs;
            }

            if (delay <= currentTime) {
                // if more than 5 seconds have passed since the last cast, ignore
                if (currentTime - gLastCast < MAX_TIME_SINCE_LAST_CAST_FOR_QUEUE) {
                    if (lastCastTime == 0) {
                        DEBUG_LOG("triggering queued cast with instant cast buffer " << gInstantCastBufferTimeMs
                                                                                     << " for " << game::GetSpellName(
                                lastSpellId));
                    } else {
                        DEBUG_LOG("triggering queued cast for " << game::GetSpellName(lastSpellId));
                    }
                    // call cast spell again
                    CastSpellHook(lastDetour, lastUnit, lastSpellId, lastItem, lastGuid);
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
}

extern "C" __declspec(dllexport) DWORD Load() {
    gStartTime = static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count());
    gCastEndMs = 0;
    gGCDEndMs = 0;
    gMaxBufferIncrease = 10;
    gMaxInstantCastBufferIncrease = 30;
    gMinBufferTimeMs = 10; // time in ms to buffer cast to minimize server failure
    gInstantCastBufferTimeMs = 60; // time in ms to buffer instant cast to minimize server failure
    gSpellQueueWindowMs = 500; // time in ms before cast to allow queuing spells
    gOnSwingBufferCooldownMs = 500; // time in ms to wait before queuing on swing spell after a swing
    gChannelQueueWindowMs = 1500; // time in ms before channel ends to allow queuing spells
    gTargetingQueueWindowMs = 1500; // time in ms before cast to allow targeting

    DEBUG_LOG("Loading nampower v1.9.0");

    std::ifstream inputFile("nampower.cfg");
    if (inputFile.is_open()) {
        std::string line;
        int lineNum = 0;
        while (std::getline(inputFile, line)) {
            std::istringstream iss(line);
            if (lineNum == 0) {
                iss >> gMinBufferTimeMs;
                DEBUG_LOG("Buffer time: " << gMinBufferTimeMs << " ms");
            } else if (lineNum == 1) {
                iss >> gSpellQueueWindowMs;
                DEBUG_LOG("Spell queuing window: " << gSpellQueueWindowMs << " ms");
            } else if (lineNum == 2) {
                iss >> gMinInstantCastBufferTimeMs;
                DEBUG_LOG("Instant Cast Buffer time: " << gMinInstantCastBufferTimeMs << " ms");
            } else if (lineNum == 3) {
                iss >> gMaxBufferIncrease;
                DEBUG_LOG("Max buffer increase: " << gMaxBufferIncrease << " ms");
            } else if (lineNum == 4) {
                iss >> gMaxInstantCastBufferIncrease;
                DEBUG_LOG("Max instant cast buffer increase: " << gMaxInstantCastBufferIncrease << " ms");
            } else if (lineNum == 5) {
                iss >> gOnSwingBufferCooldownMs;
                DEBUG_LOG("On swing buffer cooldown: " << gOnSwingBufferCooldownMs << " ms");
            } else if (lineNum == 6) {
                iss >> gChannelQueueWindowMs;
                DEBUG_LOG("Channel queuing window: " << gChannelQueueWindowMs << " ms");
            } else if (lineNum == 7) {
                iss >> gTargetingQueueWindowMs;
                DEBUG_LOG("Targeting queuing window: " << gTargetingQueueWindowMs << " ms");
            }
            lineNum++;
        }
    }

    gBufferTimeMs = gMinBufferTimeMs;
    gInstantCastBufferTimeMs = gMinInstantCastBufferTimeMs;

    gLastCast = 0;

    gCancelling = false;
    gNotifyServer = false;
    gCasting = false;
    gChanneling = false;

    const hadesmem::Process process(::GetCurrentProcessId());

    // activate spellbar and our own internal cooldown on a successful cast attempt (result from server not available yet)
    auto const castSpellOrig = hadesmem::detail::AliasCast<CastSpellT>(Offsets::CastSpell);
    gCastDetour = std::make_unique<hadesmem::PatchDetour<CastSpellT>>(process, castSpellOrig, &CastSpellHook);
    gCastDetour->Apply();

    // monitor for client-based spell interruptions to stop the castbar
    auto const cancelSpellOrig = hadesmem::detail::AliasCast<CancelSpellT>(Offsets::CancelSpell);
    gCancelSpellDetour = std::make_unique<hadesmem::PatchDetour<CancelSpellT>>(process, cancelSpellOrig,
                                                                               &CancelSpellHook);
    gCancelSpellDetour->Apply();

    auto const spellChannelStartHandlerOrig = hadesmem::detail::AliasCast<SpellChannelStartHandlerT>(
            Offsets::SpellChannelStartHandler);
    gSpellChannelStartHandlerDetour = std::make_unique<hadesmem::PatchDetour<SpellChannelStartHandlerT>>(process,
                                                                                                         spellChannelStartHandlerOrig,
                                                                                                         &SpellChannelStartHandlerHook);
    gSpellChannelStartHandlerDetour->Apply();

    auto const spellChannelUpdateHandlerOrig = hadesmem::detail::AliasCast<SpellChannelUpdateHandlerT>(
            Offsets::SpellChannelUpdateHandler);
    gSpellChannelUpdateHandlerDetour = std::make_unique<hadesmem::PatchDetour<SpellChannelUpdateHandlerT>>(process,
                                                                                                           spellChannelUpdateHandlerOrig,
                                                                                                           &SpellChannelUpdateHandlerHook);
    gSpellChannelUpdateHandlerDetour->Apply();

    // this hook will alter cast bar behavior based on events from the game
    auto const signalEventOrig = hadesmem::detail::AliasCast<SignalEventT>(Offsets::SignalEvent);
    gSignalEventDetour = std::make_unique<hadesmem::PatchDetour<SignalEventT>>(process, signalEventOrig,
                                                                               &SignalEventHook);
    gSignalEventDetour->Apply();

    auto const spellFailedOrig = hadesmem::detail::AliasCast<Spell_C_SpellFailedT>(Offsets::Spell_C_SpellFailed);
    gSpellFailedDetour = std::make_unique<hadesmem::PatchDetour<Spell_C_SpellFailedT>>(process, spellFailedOrig,
                                                                                       &Spell_C_SpellFailedHook);
    gSpellFailedDetour->Apply();

    auto const spellGoOrig = hadesmem::detail::AliasCast<SpellGoT>(Offsets::SpellGo);
    gSpellGoDetour = std::make_unique<hadesmem::PatchDetour<SpellGoT>>(process, spellGoOrig, &SpellGoHook);
    gSpellGoDetour->Apply();

    // watch for pushback notifications from the server
    auto const spellDelayedOrig = hadesmem::detail::AliasCast<PacketHandlerT>(Offsets::SpellDelayed);
    gSpellDelayedDetour = std::make_unique<hadesmem::PatchDetour<PacketHandlerT>>(process, spellDelayedOrig,
                                                                                  &SpellDelayedHook);
    gSpellDelayedDetour->Apply();

    auto const spellTargetUnitOrig = hadesmem::detail::AliasCast<SpellTargetUnitT>(Offsets::Script_SpellTargetUnit);
    gSpellTargetUnitDetour = std::make_unique<hadesmem::PatchDetour<SpellTargetUnitT>>(process, spellTargetUnitOrig, &SpellTargetUnitHook);
    gSpellTargetUnitDetour->Apply();

    // Hook the ISceneEnd function to get the EndScene function pointer
    auto const iEndSceneOrig = hadesmem::detail::AliasCast<ISceneEndT>(Offsets::ISceneEndPtr);
    gIEndSceneDetour = std::make_unique<hadesmem::PatchDetour<ISceneEndT>>(
            process, iEndSceneOrig, &ISceneEndHook);
    gIEndSceneDetour->Apply();

    return EXIT_SUCCESS;
}
