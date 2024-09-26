//
// Created by pmacc on 9/21/2024.
//

#include "spellcast.hpp"
#include "helper.hpp"
#include "offsets.hpp"

namespace Nampower {
    void BeginCast(DWORD currentTime, std::uint32_t castTime, const game::SpellRec *spell) {
        auto bufferTime = castTime == 0 ? 0 : gBufferTimeMs;

        gLastCastStartTime = castTime;
        gChanneling = SpellIsChanneling(spell);
        gLastSpellQueued = false; // reset the last spell queued flag

        if (gChanneling) {
            auto const duration = game::GetDurationObject(spell->DurationIndex);
            gChannelDuration = duration->m_Duration2;
        }

        auto const spellOnGCD = SpellIsOnGCD(spell);
        gLastCastOnGCD = spellOnGCD;

        if (spellOnGCD) {
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

    bool
    CastSpellHook(hadesmem::PatchDetourBase *detour, void *unit, uint32_t spellId, void *item, std::uint64_t guid) {
        auto const spell = game::GetSpellInfo(spellId);
        auto const spellIsOnSwing = SpellIsOnSwing(spell);
        auto const spellName = game::GetSpellName(spellId);
        auto currentTime = GetTime();

        DEBUG_LOG("Attempt cast " << spellName << " on guid " << guid << " , time since last cast "
                                  << currentTime - gLastCast);

        // on swing spells are independent of cast bar / gcd, handle them separately
        if (spellIsOnSwing) {
            lastOnSwingSpellId = spellId;

            // try to cast the spell
            auto const castSpell = detour->GetTrampolineT<CastSpellT>();
            auto ret = castSpell(unit, spellId, item, guid);

            if (!ret && gQueueOnSwingSpells) {
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
        auto remainingCastTime = (gCastEndMs > currentTime) ? gCastEndMs - currentTime : 0;
        auto remainingGCD = (gGCDEndMs > currentTime) ? gGCDEndMs - currentTime : 0;

        auto remainingCD = (remainingCastTime > remainingGCD) ? remainingCastTime : remainingGCD;

        auto remainingCastTimeInQueueWindow = remainingCastTime > 0 && remainingCastTime < gSpellQueueWindowMs;
        auto remainingGCDInQueueWindow = remainingGCD > 0 && remainingGCD < gSpellQueueWindowMs;
        auto inSpellQueueWindow = remainingCastTimeInQueueWindow || remainingGCDInQueueWindow;

        auto const cursorMode = *reinterpret_cast<int *>(Offsets::CursorMode);
        if (spell->Targets == game::SpellTarget::TARGET_LOCATION_UNIT_POSITION &&
            cursorMode == 1) {
            auto remainingCastTimeInTargetingQueueWindow =
                    remainingCastTime > 0 && remainingCastTime < gTargetingQueueWindowMs;
            auto remainingGCDInTargetingQueueWindow = remainingGCD > 0 && remainingGCD < gTargetingQueueWindowMs;
            inSpellQueueWindow = remainingCastTimeInTargetingQueueWindow || remainingGCDInTargetingQueueWindow;

            if (gQueueTargetingSpells) {
                if (castTime > 0 && inSpellQueueWindow) {
                    if (gQueueCastTimeSpells) {
                        DEBUG_LOG("Queuing targeting for after cooldown: " << remainingCD << "ms " << spellName);
                        gSpellQueued = true;
                        // save the detour to trigger the cast again after the cooldown is up
                        lastDetour = detour;
                        return false;
                    }
                } else if (spellOnGCD && inSpellQueueWindow) {
                    if (gQueueInstantSpells) {
                        DEBUG_LOG("Queuing instant cast targeting for after cooldown: " << remainingCD << "ms "
                                                                                        << spellName);
                        gSpellQueued = true;
                        // save the detour to trigger the cast again after the cooldown is up
                        lastDetour = detour;
                        return false;
                    }
                }
            }
        } else if (castTime > 0 && inSpellQueueWindow) {
            if (gQueueCastTimeSpells) {
                DEBUG_LOG("Queuing for after cooldown: " << remainingCD << "ms " << spellName);
                gSpellQueued = true;

                // save the detour to trigger the cast again after the cooldown is up
                lastDetour = detour;

                return false;
            }
        } else if (spellOnGCD && inSpellQueueWindow) {
            if (gQueueInstantSpells) {
                DEBUG_LOG("Queuing instant cast for after cooldown: " << remainingCD << "ms " << spellName);
                gSpellQueued = true;

                // save the detour to trigger the cast again after the cooldown is up
                lastDetour = detour;

                return false;
            }
        } else if (spellIsChanneling && gChanneling) {
            if (gQueueChannelingSpells) {
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
        }

        // is there a cast? (ignore for on swing spells)
        if (remainingCastTime) {
            DEBUG_LOG("Cooldown active " << remainingCastTime << "ms remaining");
            return false;
        } else {
            gCastEndMs = 0;
        }

        // is there a GCD?
        if (spellOnGCD && remainingGCD) {
            DEBUG_LOG("Gcd active " << remainingGCD << "ms remaining");
            return false;
        } else {
            gGCDEndMs = 0;
        }

        gCasting = true;
        auto ret = castSpell(unit, spellId, item, guid);
        gCasting = false;

        // if this is a trade skill or item enchant, do nothing further
        if (spell->Effect[0] == game::SpellEffects::SPELL_EFFECT_TRADE_SKILL ||
            spell->Effect[0] == game::SpellEffects::SPELL_EFFECT_ENCHANT_ITEM ||
            spell->Effect[0] == game::SpellEffects::SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY ||
            spell->Effect[0] == game::SpellEffects::SPELL_EFFECT_CREATE_ITEM)
            return ret;

        // haven't gotten spell result from the previous cast yet, probably due to latency.
        // simulate a cancel to clear the cast bar but only when there should be a cast time
        // mining/herbing have cast time but aren't on GCD, don't cancel them
        if (!ret && gLastCastStartTime > 0 && gLastCastOnGCD) {
            if (*reinterpret_cast<int *>(Offsets::SpellIsTargeting) == 0) {
                DEBUG_LOG("Canceling spell cast due to previous spell having cast time of " << gLastCastStartTime);

                gCancelling = true;
                //JT: Suggest replacing CancelSpell with InterruptSpell (the API called when moving during casting).
                // The address of InterruptSpell needs to be dug out. It could possibly fix the sometimes broken animations.
                auto const cancelSpell = reinterpret_cast<CancelSpellT>(Offsets::CancelSpell);
                cancelSpell(true, false, game::SPELL_FAILED_INTERRUPTED);

                gCancelling = false;

                // try again...
                gCasting = true;
                ret = castSpell(unit, spellId, item, guid);
                gCasting = false;

                if (ret) {
                    gSpellQueued = false;
                }
            } else {
                DEBUG_LOG("Initial cast failed, not canceling spell cast due to targeting");
            }
        }

        //JT: cursorMode == 2 is for clickcasting
        if (!ret && !(spell->Attributes & game::SPELL_ATTR_RANGED) && cursorMode != 2) {
            DEBUG_LOG("Spell cast start failed castSpell returned " << ret << " spell " << spellId << " isRanged " <<
                                                                    !(spell->Attributes & game::SPELL_ATTR_RANGED));
        }

        return ret;
    }

    void SpellGoHook(hadesmem::PatchDetourBase *detour, uint64_t *casterGUID, uint64_t *targetGUID, uint32_t spellId,
                     game::CDataStore *spellData) {

        auto const castByActivePlayer = game::ClntObjMgrGetActivePlayer() == *casterGUID;

        // only care about our own casts and lastSpellId(ignore spell procs)
        if (gChanneling && castByActivePlayer) {
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
                    gLastSpellQueued = true;
                }
            }
        } else if (gOnSwingQueued && castByActivePlayer) {
            // check if spell is on hit
            auto const spell = game::GetSpellInfo(spellId);
            if (spell->Attributes & game::SPELL_ATTR_ON_NEXT_SWING_1) {
                DEBUG_LOG("On swing spell " << game::GetSpellName(spellId) <<
                                            " resolved, casting queued on swing spell "
                                            << game::GetSpellName(lastOnSwingSpellId));
                gLastOnSwingCastTime = GetTime();
                gOnSwingQueued = false;
                CastSpellHook(onSwingDetour, lastUnit, lastOnSwingSpellId, lastItem, lastGuid);
                gLastSpellQueued = true;
            }
        }

        auto const spellGo = detour->GetTrampolineT<SpellGoT>();
        spellGo(casterGUID, targetGUID, spellId, spellData);
    }

    bool Spell_C_TargetSpellHook(hadesmem::PatchDetourBase *detour,
                                 uint32_t *player,
                                 uint32_t *spellId,
                                 uint32_t unk3,
                                 float unk4) {
        auto const spellTarget = detour->GetTrampolineT<Spell_C_TargetSpellT>();
        auto result = spellTarget(player, spellId, unk3, unk4);

        if (!result) {
            auto const spellName = game::GetSpellName(*spellId);
            auto const spell = game::GetSpellInfo(*spellId);

            if (spell->Targets == game::SpellTarget::TARGET_LOCATION_UNIT_POSITION) {
                // if quickcast is on instantly trigger all casts
                // otherwise if this is a queued cast, trigger it instant cast
                if (gQuickcastTargetingSpells || (gQueueTargetingSpells && gNormalQueueTriggered)) {
                    DEBUG_LOG("Quickcasting terrain spell " << spellName
                                                            << " quickcast: " << gQuickcastTargetingSpells
                                                            << " queuetrigger: " << gNormalQueueTriggered);
                    LuaCall("CameraOrSelectOrMoveStart()");
                    LuaCall("CameraOrSelectOrMoveStop()");
                }
            }
        }
        return result;
    }

    void
    CancelSpellHook(hadesmem::PatchDetourBase *detour, bool failed, bool notifyServer, game::SpellCastResult reason) {
        DEBUG_LOG("Cancel spell cast failed:" << failed << " notifyServer:" << notifyServer << " reason:" << int(reason));

        // triggered by us, reset the cast bar
        if(notifyServer){
            ResetCastFlags();
        }

        auto const cancelSpell = detour->GetTrampolineT<CancelSpellT>();
        return cancelSpell(failed, notifyServer, reason);
    }

    void SendCastHook(hadesmem::PatchDetourBase *detour, game::SpellCast *cast, char unk) {
        auto const sendCast = detour->GetTrampolineT<SendCastT>();
        sendCast(cast, unk);

        auto const spell = game::GetSpellInfo(cast->spellId);
        BeginCast(GetTime(), lastCastTime, spell);
    }
}