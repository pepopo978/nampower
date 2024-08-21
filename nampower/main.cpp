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

#ifdef _DEBUG
    std::ofstream debugLogFile("nampower_debug.log");
    #define DEBUG_LOG(msg) debugLogFile << "[DEBUG]" << GetTime() << ": " << msg << std::endl
#else
    #define DEBUG_LOG(msg) // No-op in release mode
#endif

BOOL WINAPI DllMain(HINSTANCE, DWORD, void *);

namespace {
    static const DWORD MAX_TIME_SINCE_LAST_CAST_FOR_QUEUE = 10000;
    static DWORD gStartTime;
    static DWORD gCooldownMs;
    static DWORD gGCDCooldownMs;
    static DWORD gBufferTimeMs;   // adjusts dynamically depending on errors
    static DWORD gInstantCastBufferTimeMs;   // adjusts dynamically depending on errors
    static DWORD gMinBufferTimeMs; // set by user
    static DWORD gSpellQueueWindowMs;
    static DWORD gLastCast;

    static DWORD gLastErrorTimeMs;

    // true a spell is queued
    static std::atomic<bool> gSpellQueued;

    // true when we are simulating a server-based spell cancel to reset the cast bar
    static std::atomic<bool> gCancelling;

    // true when the current spell cancellation requires that we notify the server
    // (a client side cancellation of a non-instant cast spell)
    static std::atomic<bool> gNotifyServer;

    // true when we are in the middle of an attempt to cast a spell
    static std::atomic<bool> gCasting;

    static game::SpellFailedReason gCancelReason;

    static hadesmem::PatchDetourBase *lastDetour;
    static void *lastUnit;
    static int lastSpellId;
    static void *lastItem;
    static std::uint32_t lastCastTime;
    static std::uint64_t lastGuid;

    using CastSpellT = bool (__fastcall *)(void *, int, void *, std::uint64_t);
    using SendCastT = void (__fastcall *)(game::SpellCast *);
    using CancelSpellT = int (__fastcall *)(bool, bool, game::SpellFailedReason);
    using SignalEventT = void (__fastcall *)(game::Events);
    using PacketHandlerT = int (__stdcall *)(int, game::CDataStore *);
    using EndSceneT = int (__stdcall *)(uintptr_t *device);
    using ISceneEndT = int *(__fastcall *)(uintptr_t *unk);

    typedef int (__cdecl *tGetSpellCooldownByID)(int, int, DWORD *, DWORD *, DWORD *);

    typedef int (__cdecl *tGetTimer)();

    std::unique_ptr<hadesmem::PatchDetour<CastSpellT>> gCastDetour;
    std::unique_ptr<hadesmem::PatchDetour<SendCastT>> gSendCastDetour;
    std::unique_ptr<hadesmem::PatchDetour<CancelSpellT>> gCancelSpellDetour;
    std::unique_ptr<hadesmem::PatchDetour<SignalEventT>> gSignalEventDetour;
    std::unique_ptr<hadesmem::PatchDetour<PacketHandlerT>> gSpellDelayedDetour;
    std::unique_ptr<hadesmem::PatchRaw> gCastbarPatch;
    std::unique_ptr<hadesmem::PatchDetour<ISceneEndT>> gIEndSceneDetour;
    std::unique_ptr<hadesmem::PatchDetour<EndSceneT>> gEndSceneDetour;

    // Pointer to EndScene function
    static uintptr_t *gEndScenePtr = nullptr;

    DWORD GetTime() {
        return static_cast<DWORD>(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count()) - gStartTime;
    }


    uintptr_t ReadIntPtr(uintptr_t *address) {
        return *address;
    }

    uintptr_t ReadIntPtrOffset(uintptr_t *address, uintptr_t offset) {
        return *(address + offset);
    }

    // This crashes needs more work but plan is to use it to queue up instant cast spells
    DWORD GlobalCooldownEndTime(int spellId) {
        auto GetSpellCooldownByID = (tGetSpellCooldownByID) 0x00809000; // Spell_C__GetSpellCooldown_Proxy in the info thread
        auto GetTimer = (tGetTimer) 0x0086AE20; // PerformanceCounter in the info thread

        DWORD cdLength = 0, timeStarted = 0, enabled = 0, RemainingCD = 0;
        GetSpellCooldownByID(spellId, 0, &cdLength, &timeStarted, &enabled);

        if (cdLength > 0)
            RemainingCD = (timeStarted + cdLength) - GetTimer();

        return RemainingCD;
    }

    void BeginCast(DWORD currentTime, std::uint32_t castTime, int spellId) {
        gCooldownMs = castTime ? currentTime + castTime + gBufferTimeMs : 0;
        gGCDCooldownMs = currentTime + 1500;

        // if it has been 1 min since last error, can lower gBufferTimeMs
        if (gBufferTimeMs > gMinBufferTimeMs && gLastErrorTimeMs && currentTime - gLastErrorTimeMs > 30000) {
            gBufferTimeMs -= 5;
            gLastErrorTimeMs = currentTime; // wait another minute before potentially lowering again
        }

        DEBUG_LOG("BeginCast " << game::GetSpellName(spellId) <<
                               " with cast time " << castTime << " and buffer " << gBufferTimeMs);
        gLastCast = currentTime;
        //JT: Use the notification from server to activate cast bar - it is needed to have HealComm work.
        //if (castTime)
        //{
        //    void(*signalEvent)(std::uint32_t, const char *, ...) = reinterpret_cast<decltype(signalEvent)>(Offsets::SignalEventParam);
        //    signalEvent(game::Events::SPELLCAST_START, "%s%d", game::GetSpellName(spellId), castTime);
        //}
    }

    bool CastSpellHook(hadesmem::PatchDetourBase *detour, void *unit, int spellId, void *item, std::uint64_t guid) {
        auto const castTime = game::GetCastTime(unit, spellId);

        lastUnit = unit;
        lastSpellId = spellId;
        lastItem = item;
        lastGuid = guid;
        lastCastTime = castTime;
        gSpellQueued = false;

        if (spellId != lastSpellId) {
            lastDetour = nullptr; // reset the last detour if the spell is different
        }

        auto currentTime = GetTime();
        auto remainingCD = gCooldownMs - currentTime;
        auto remainingGCD = gGCDCooldownMs - currentTime;

        if (remainingCD > 0 && remainingCD < gSpellQueueWindowMs) {
            gSpellQueued = true;

            // save the detour to trigger the cast again after the cooldown is up
            lastDetour = detour;

            DEBUG_LOG("queuing for after cooldown: " << remainingCD << "ms");

            return false;
        } else if (remainingGCD > 0 && remainingGCD < gSpellQueueWindowMs) {
            gSpellQueued = true;

            // save the detour to trigger the cast again after the cooldown is up
            lastDetour = detour;

            DEBUG_LOG("queuing for after gcd: " << remainingGCD << "ms");

            return false;
        }

        DEBUG_LOG(
                "Attempt cast spell time elapsed since last cast " << currentTime - gLastCast);

        // is there a cooldown?
        if (gCooldownMs) {
            // is it still active?
            if (gCooldownMs > currentTime) {
                DEBUG_LOG("cooldown active " << gCooldownMs - currentTime << "ms remaining");
                return false;
            }

            gCooldownMs = 0;
        }
        // is there a GCD?
        if (gGCDCooldownMs) {
            // is it still active?
            if (gGCDCooldownMs > currentTime) {
                DEBUG_LOG("gcd active " << gGCDCooldownMs - currentTime << "ms remaining");
                return false;
            }

            gGCDCooldownMs = 0;
        }


        gCasting = true;

        auto const spell = game::GetSpellInfo(spellId);

        auto const castSpell = detour->GetTrampolineT<CastSpellT>();
        auto ret = castSpell(unit, spellId, item, guid);

        // if this is a trade skill or item enchant, do nothing further
        if (spell->Effect[0] == game::SpellEffects::SPELL_EFFECT_TRADE_SKILL ||
            spell->Effect[0] == game::SpellEffects::SPELL_EFFECT_ENCHANT_ITEM ||
            spell->Effect[0] == game::SpellEffects::SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY ||
            spell->Effect[0] == game::SpellEffects::SPELL_EFFECT_CREATE_ITEM)
            return ret;

        // haven't gotten spell result from the previous cast yet, probably due to latency.
        // simulate a cancel to clear the cast bar but only when there should be a cast time
        if (!ret) {
            gCancelling = true;
            //JT: Suggest replacing CancelSpell with InterruptSpell (the API called when moving during casting).
            // The address of InterruptSpell needs to be dug out. It could possibly fix the sometimes broken animations.
            auto const cancelSpell = reinterpret_cast<CancelSpellT>(Offsets::CancelSpell);
            cancelSpell(false, false, game::SpellFailedReason::SPELL_FAILED_ERROR);

            gCancelling = false;

            // try again...
            ret = castSpell(unit, spellId, item, guid);
        }

        auto const cursorMode = *reinterpret_cast<int *>(Offsets::CursorMode);
        //JT: cursorMode == 2 is for clickcasting
        if (ret && !(spell->Attributes & game::SPELL_ATTR_RANGED) /* && cursorMode != 2 */) {
            currentTime = GetTime();
            BeginCast(currentTime, castTime, spellId);
        } else {
            DEBUG_LOG("Spell cast start failed castSpell returned " << ret << " spell " << spellId << " isRanged " <<
                                                                    !(spell->Attributes & game::SPELL_ATTR_RANGED));
        }

        gCasting = false;

        return ret;
    }

    int
    CancelSpellHook(hadesmem::PatchDetourBase *detour, bool failed, bool notifyServer, game::SpellFailedReason reason) {
        gNotifyServer = notifyServer;
        gCancelReason = reason;

        auto const cancelSpell = detour->GetTrampolineT<CancelSpellT>();
        auto const ret = cancelSpell(failed, notifyServer, reason);

        return ret;
    }

    //JT: Using this breaks animations. It is only useful for AOE spells. We can live without speeding those up.
//    void SendCastHook(hadesmem::PatchDetourBase *detour, game::SpellCast *cast) {
//        auto const cursorMode = *reinterpret_cast<int *>(Offsets::CursorMode);
//        DEBUG_LOG("SendCastHook cursor " << cursorMode);
//
//        // if we were waiting for a target, it means there is no cast bar yet.  make one \o/
//        if (cursorMode == 2) {
//            auto const unit = game::GetObjectPtr(cast->caster);
//            auto const castTime = game::GetCastTime(unit, cast->spellId);
//
//            BeginCast(std::chrono::high_resolution_clock::now(), castTime, cast->spellId);
//        }
//
//        auto const sendCast = detour->GetTrampolineT<SendCastT>();
//        sendCast(cast);
//    }

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
                gCooldownMs = 0;
                gGCDCooldownMs = 0;
            }

                // if this is from the server but it is happening too early, it is for one of two reasons.
                // 1) it is for the last cast, in which case we can ignore it
                // 2) it is for our current cast and the server decided to cast sooner than we expected
                //    this can happen from mage 8/8 t2 proc or presence of mind
            else if (!gCasting && !gCancelling && currentTime <= gCooldownMs) {
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
            gCooldownMs = 0;
            gGCDCooldownMs = 0;
            gCasting = false;

            if (eventId == game::Events::SPELLCAST_FAILED) {
                // don't spam retry if out of mana
                if (gLastErrorTimeMs - currentTime < 200) {
                    DEBUG_LOG("Spellcast failed, not queuing retry due to recent error at " << gLastErrorTimeMs);
                    return;
                } else {
                    DEBUG_LOG("Spellcast failed, queuing a retry");
                }
                gSpellQueued = true;
                gLastErrorTimeMs = currentTime;

                if (gBufferTimeMs < 200) {
                    gBufferTimeMs += 5;
                }
            } else {
                DEBUG_LOG("Spellcast interrupted");
            }
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
            if (currentTime < gCooldownMs) {
                gCooldownMs += delay;
            }
        }

        return spellDelayed(opCode, packet);
    }

    int *ISceneEndHook(hadesmem::PatchDetourBase *detour, uintptr_t *ptr) {
        auto const iSceneEnd = detour->GetTrampolineT<ISceneEndT>();

        if (lastDetour && gSpellQueued) {
            auto currentTime = GetTime();

            // get max of cooldown and gcd
            auto delay = gCooldownMs > gGCDCooldownMs ? gCooldownMs : gGCDCooldownMs;

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
    gCooldownMs = 0;
    gGCDCooldownMs = 0;
    gMinBufferTimeMs = 0; // time in ms to buffer cast to minimize server failure
    gInstantCastBufferTimeMs = 30; // time in ms to buffer instant cast to minimize server failure
    gSpellQueueWindowMs = 1000; // time in ms before cast is possible to sleep to try to cast at the perfect time

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
                iss >> gInstantCastBufferTimeMs;
                DEBUG_LOG("Instant Cast Buffer time: " << gInstantCastBufferTimeMs << " ms");
            }
            lineNum++;
        }
    }

    gBufferTimeMs = gMinBufferTimeMs;

    gLastCast = 0;

    gCancelling = false;
    gNotifyServer = false;
    gCasting = false;

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

    //JT: Disable this. This is only for AOE spells and breaks animations.
    // monitor for spell cast triggered after target (terrain, item, etc.) is selected
//    auto const sendCastOrig = hadesmem::detail::AliasCast<SendCastT>(Offsets::SendCast);
//    gSendCastDetour = std::make_unique<hadesmem::PatchDetour<SendCastT>>(process, sendCastOrig, &SendCastHook);
//    gSendCastDetour->Apply();

    // this hook will alter cast bar behavior based on events from the game
    auto const signalEventOrig = hadesmem::detail::AliasCast<SignalEventT>(Offsets::SignalEvent);
    gSignalEventDetour = std::make_unique<hadesmem::PatchDetour<SignalEventT>>(process, signalEventOrig,
                                                                               &SignalEventHook);
    gSignalEventDetour->Apply();

    // watch for pushback notifications from the server
    auto const spellDelayedOrig = hadesmem::detail::AliasCast<PacketHandlerT>(Offsets::SpellDelayed);
    gSpellDelayedDetour = std::make_unique<hadesmem::PatchDetour<PacketHandlerT>>(process, spellDelayedOrig,
                                                                                  &SpellDelayedHook);
    gSpellDelayedDetour->Apply();
    //JT: Disabling SPELLCAST_START from the server breaks HealComm. It needs time to have passed between CastSpell and
    // SPELLCAST_START. Rather have the castbar appear slightly late (only visual mismatch).
    // prevent spellbar re-activation upon successful cast notification from server
//    const std::vector<std::uint8_t> patch(5, 0x90);
//    gCastbarPatch = std::make_unique<hadesmem::PatchRaw>(process, reinterpret_cast<void *>(Offsets::CreateCastbar), patch);
//    gCastbarPatch->Apply();

//    sleep_for(std::chrono::milliseconds(8000));


    // Hook the ISceneEnd function to get the EndScene function pointer
    auto const iEndSceneOrig = hadesmem::detail::AliasCast<ISceneEndT>(Offsets::ISceneEndPtr);
    gIEndSceneDetour = std::make_unique<hadesmem::PatchDetour<ISceneEndT>>(
            process, iEndSceneOrig, &ISceneEndHook);
    gIEndSceneDetour->Apply();

    return EXIT_SUCCESS;
}
