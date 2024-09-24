//
// Created by pmacc on 9/21/2024.
//

#include "spell_cast.hpp"
#include "init.hpp"
#include "helper.hpp"

namespace Nampower {
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
            if (*reinterpret_cast<int *>(Offsets::SpellIsTargeting) == 0) {
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
}