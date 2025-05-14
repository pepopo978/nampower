//
// Created by pmacc on 9/21/2024.
//

#include "spellcast.hpp"
#include "helper.hpp"
#include "offsets.hpp"
#include "logging.hpp"

namespace Nampower {
    uint32_t GetChannelBaseDuration(const game::SpellRec *spell) {
        auto const duration = game::GetDurationObject(spell->DurationIndex);
        return duration->m_Duration;
    }

    void BeginCast(uint32_t castTime, const game::SpellRec *spell, const game::SpellCast *cast) {
        auto currentTime = GetTime();

        gLastCastData.castTimeMs = castTime;

        gCastData.channeling = SpellIsChanneling(spell);
        gLastCastData.wasQueued = false; // reset the last spell queued flag

        if (gCastData.channeling) {
            gCastData.channelDuration = GetChannelBaseDuration(spell);
            gCastData.channelEndMs = currentTime + gCastData.channelDuration;
        }

        auto const spellOnGcd = SpellIsOnGcd(spell);
        gLastCastData.wasOnGcd = spellOnGcd;

        auto lastCastParams = gCastHistory.peek();
        uint64_t lastCastId = 0;
        if (lastCastParams != nullptr) {
            gLastCastData.wasItem = lastCastParams->item != nullptr;
            lastCastParams->castResult = CastResult::WAITING_FOR_SERVER;
            lastCastId = lastCastParams->castId;
        }

        auto bufferMs = GetServerDelayMs();
        // Reset the server delay
        gLastServerSpellDelayMs = 0;

        if (spellOnGcd) {
            auto gcdTime = GetGcdOrCooldownForSpell(spell->Id);
            if (gcdTime > 1500) {
                gcdTime = 1500; // items with spells on gcd will return their item gcd, make sure not to use that
            }

            if (castTime < gcdTime - 50) {
                bufferMs = 0; // no longer need to buffer spells with cast time 50ms < gcd
            } else if (castTime < gcdTime) {
                auto const diff = gcdTime - castTime;
                if (bufferMs > diff) {
                    bufferMs -= diff; // subtract the difference from the buffer
                } else {
                    bufferMs = 0; // if the buffer is less than the difference, set it to 0
                }
            }

            gCastData.gcdEndMs = currentTime + gcdTime + bufferMs;
            DEBUG_LOG("BeginCast #" << lastCastId
                                    << " " << game::GetSpellName(spell->Id)
                                    << " cast time: " << castTime
                                    << " buffer: " << bufferMs
                                    << " Gcd: " << gcdTime
                                    << " latency: " << GetLatencyMs()
                                    << " time since last cast " << currentTime - gLastCastData.startTimeMs);
        } else {
            gCastData.delayEndMs = currentTime +
                                   gUserSettings.nonGcdBufferTimeMs; // set small "cast time" to avoid attempting next spell too fast
            DEBUG_LOG("BeginCast #" << lastCastId
                                    << " " << game::GetSpellName(spell->Id)
                                    << " cast time: " << castTime
                                    << " buffer: " << bufferMs
                                    << " NO Gcd"
                                    << " latency: " << GetLatencyMs()
                                    << " time since last cast " << currentTime - gLastCastData.startTimeMs);
        }

        gCastData.castEndMs = castTime ? currentTime + castTime + bufferMs : 0;
        gCastData.bufferMs = bufferMs;

        // check if we can lower buffers
        if (currentTime - gLastBufferDecreaseTimeMs > BUFFER_DECREASE_FREQUENCY) {
            if (gBufferTimeMs > gUserSettings.minBufferTimeMs) {
                gBufferTimeMs -= DYNAMIC_BUFFER_INCREMENT;
                DEBUG_LOG("Decreasing default buffer to " << gBufferTimeMs);
            }

            gLastBufferDecreaseTimeMs = currentTime; // update the last error time to prevent lowering buffer too often
        }

        gLastCastData.startTimeMs = currentTime;

        // if queued cast, simulate spellcast start
    }

    void CastQueuedNonGcdSpell() {
        if (gCastData.nonGcdSpellQueued) {
            auto nonGcdCastParams = gNonGcdCastQueue.pop();
            if (nonGcdCastParams.spellId > 0) {
                DEBUG_LOG("Triggering queued non gcd cast of " << game::GetSpellName(nonGcdCastParams.spellId));

                gCastData.castingQueuedSpell = true;
                gCastData.numRetries = nonGcdCastParams.numRetries;
                Spell_C_CastSpellHook(castSpellDetour, nonGcdCastParams.playerUnit, nonGcdCastParams.spellId,
                                      nonGcdCastParams.item, nonGcdCastParams.guid);
                gLastCastData.wasQueued = true;
            } else {
                DEBUG_LOG("Ignoring queued non gcd cast, no spell id");
                gLastCastData.wasQueued = false;
            }
            TriggerSpellQueuedEvent(NON_GCD_QUEUE_POPPED, nonGcdCastParams.spellId);
            gCastData.nonGcdSpellQueued = !gNonGcdCastQueue.isEmpty();
            gCastData.castingQueuedSpell = false;
            gCastData.numRetries = 0;
        }
    }

    void CastQueuedNormalSpell() {
        if (gCastData.normalSpellQueued) {
            if (gLastNormalCastParams.spellId > 0) {
                DEBUG_LOG("Triggering queued cast of " << game::GetSpellName(gLastNormalCastParams.spellId));
                gCastData.castingQueuedSpell = true;
                gCastData.numRetries = gLastNormalCastParams.numRetries;
                Spell_C_CastSpellHook(castSpellDetour, gLastNormalCastParams.playerUnit, gLastNormalCastParams.spellId,
                                      gLastNormalCastParams.item, gLastNormalCastParams.guid);
                gLastCastData.wasQueued = true;
            } else {
                DEBUG_LOG("Ignoring queued cast, no spell id");
                gLastCastData.wasQueued = false;
            }
            TriggerSpellQueuedEvent(NORMAL_QUEUE_POPPED, gLastNormalCastParams.spellId);
            gCastData.normalSpellQueued = false;
            gCastData.castingQueuedSpell = false;
            gCastData.numRetries = 0;
        }
    }

    void CastQueuedSpells() {
        if (gCastData.nonGcdSpellQueued) {
            CastQueuedNonGcdSpell();
        } else {
            CastQueuedNormalSpell();
        }
    }

    void SaveCastParams(CastSpellParams *params,
                        uint32_t *playerUnit,
                        uint32_t spellId,
                        uintptr_t *item,
                        std::uint64_t guid,
                        uint32_t gcDCategory,
                        uint32_t castTimeMs,
                        uint32_t castStartTimeMs,
                        CastType castType,
                        uint32_t numRetries) {
        params->playerUnit = playerUnit;
        params->spellId = spellId;
        params->item = item;
        params->guid = guid;

        params->gcDCategory = gcDCategory;
        params->castTimeMs = castTimeMs;
        params->castStartTimeMs = castStartTimeMs;
        params->castType = castType;
        params->numRetries = numRetries;
    }

    void TriggerSpellQueuedEvent(QueueEvents queueEventCode, uint32_t spellId) {
        ((int (__cdecl *)(int, char *, uint32_t, uint32_t)) Offsets::SignalEventParam)(
                game::SPELL_QUEUE_EVENT,  // SPELL_QUEUE_EVENT event we are adding
                (char *) Offsets::IntIntParamFormat,
                queueEventCode,
                spellId);
    }

    void
    TriggerSpellCastEvent(bool result, uint32_t spellId, CastType castType, std::uint64_t guid, uint32_t itemId) {
        char format[] = "%d%d%d%s%d";
        char *guidStr = new char[21]; // 2 for 0x prefix, 18 for the number, and 1 for '\0'
        std::snprintf(guidStr, 21, "0x%016llX", static_cast<unsigned long long>(guid));

        ((int (__cdecl *)(int, char *, uint32_t, uint32_t, uint32_t, char *, uint32_t)) Offsets::SignalEventParam)(
                game::SPELL_CAST_EVENT,  // SPELL_CAST_EVENT event we are adding
                format,
                result,
                spellId,
                castType,
                guidStr,
                itemId);
    }

    void clearCastingSpell() {
        // clearing current casting spell id if needed
        // this prevents client from failing to cast spells without a casttime
        // due to not receiving spell result yet
        auto const castingSpellId = reinterpret_cast<uint32_t *>(Offsets::CastingSpellId);
        if (*castingSpellId > 0) {
            *castingSpellId = 0;
        }
    }

    bool
    Spell_C_CastSpellHook(hadesmem::PatchDetourBase *detour, uint32_t *playerUnit, uint32_t spellId, uintptr_t *item,
                          std::uint64_t guid) {
        // save the detour to allow quickly calling this hook
        castSpellDetour = detour;

        auto const spell = game::GetSpellInfo(spellId);
        auto const spellIsOnSwing = SpellIsOnSwing(spell);
        auto const spellName = game::GetSpellName(spellId);
        auto currentTime = GetTime();
        auto const castTime = game::GetCastTime(playerUnit, spellId);
        gCastData.attemptedCastTimeMs = castTime;

        auto const spellOnGcd = SpellIsOnGcd(spell);
        auto const spellIsChanneling = SpellIsChanneling(spell);
        auto const spellIsTargeting = SpellIsTargeting(spell);
        auto const isSpecialSpell = SpellIsAttackTradeskillOrEnchant(spell);

        // check for double press to interrupt channeling early
        if (gCastData.channeling && !gCastData.cancelChannelNextTick &&
            gCastData.numRetries == 0 &&
            gUserSettings.doubleCastToEndChannelEarly &&
            gCastData.channelStartMs > 0) {
            // wait 500ms after the start of a channel before allowing double cast to end it early
            if (currentTime - gCastData.channelStartMs > 500) {
                // check if same spell is being cast again within 350ms
                if (gLastCastData.attemptSpellId == spellId && currentTime - gLastCastData.attemptTimeMs < 350) {
                    DEBUG_LOG("Double cast detected for " << spellName << ", ending channel early");
                    gCastData.cancelChannelNextTick = true;
                }
            }
        }

        if (gCastData.nonGcdSpellQueued && spellOnGcd) {
            // it is possible when spamming to attempt to cast before non gcd spells are processed, process queue first
            if (processQueues()) {
                return false;
            }
        }

        gLastCastData.attemptTimeMs = currentTime;
        gLastCastData.attemptSpellId = spellId;

        uint32_t itemId = 0;
        if (item) {
            itemId = game::GetItemId((game::CGItem_C *) item);
        }

        DEBUG_LOG("Attempt cast " << spellName << " item " << item << " on guid " << guid
                                  << ", time since last cast " << currentTime - gLastCastData.startTimeMs);

        // clear cooldown queue if we are casting a spell
        if (spellOnGcd && gCastData.cooldownNormalSpellQueued) {
            gCastData.cooldownNormalSpellQueued = false;
            TriggerSpellQueuedEvent(NORMAL_QUEUE_POPPED, gLastNormalCastParams.spellId);
        } else if (gCastData.cooldownNonGcdSpellQueued) {
            gCastData.cooldownNonGcdSpellQueued = false;
            // pop the params
            auto nonGcdParams = gNonGcdCastQueue.pop();
            TriggerSpellQueuedEvent(NON_GCD_QUEUE_POPPED, nonGcdParams.spellId);
        }

        // on swing spells are independent of cast bar / gcd, handle them separately
        if (spellIsOnSwing) {
            SaveCastParams(&gLastOnSwingCastParams, playerUnit, spellId, item, guid, spell->StartRecoveryCategory,
                           castTime,
                           currentTime, ON_SWING, 0);

            gCastHistory.pushFront({gNextCastId, playerUnit, spellId, item, guid,
                                    spell->StartRecoveryCategory,
                                    castTime,
                                    currentTime,
                                    ON_SWING,
                                    gCastData.numRetries,
                                    CastResult::WAITING_FOR_CAST});
            gNextCastId++;

            // try to cast the spell
            auto const castSpell = detour->GetTrampolineT<CastSpellT>();
            auto ret = castSpell(playerUnit, spellId, item, guid);

            TriggerSpellCastEvent(ret, spellId, ON_SWING, guid, itemId);

            if (ret) {
                gCastData.pendingOnSwingCast = true;
                gCastData.onSwingSpellId = spellId;
            }

            if (!ret && gUserSettings.queueOnSwingSpells && !gNoQueueCast) {
                // if not in cooldown window
                if (currentTime - gLastCastData.onSwingStartTimeMs > gUserSettings.onSwingBufferCooldownMs) {
                    DEBUG_LOG("Queuing on swing spell " << spellName);
                    TriggerSpellQueuedEvent(ON_SWING_QUEUED, spellId);
                    gCastData.onSwingQueued = true;
                }
            } else {
                gLastCastData.onSwingStartTimeMs = GetTime();
                DEBUG_LOG("Successful on swing spell " << spellName);
            }

            return ret;
        }

        auto const castSpell = detour->GetTrampolineT<CastSpellT>();

        auto effectiveCastEndMs = EffectiveCastEndMs();
        auto remainingEffectiveCastTime = (effectiveCastEndMs > currentTime) ? effectiveCastEndMs - currentTime : 0;
        auto remainingGcd = (gCastData.gcdEndMs > currentTime) ? gCastData.gcdEndMs - currentTime : 0;

        auto remainingCD = (remainingEffectiveCastTime > remainingGcd) ? remainingEffectiveCastTime : remainingGcd;

        auto inSpellQueueWindow = InSpellQueueWindow(remainingEffectiveCastTime, remainingGcd, spellIsTargeting);

        // don't queue trade skills or enchants
        if (isSpecialSpell) {
            inSpellQueueWindow = false;
        }

        if (spellOnGcd) {
            auto castType = NORMAL;
            if (spellIsTargeting) {
                castType = TARGETING;
            } else if (spellIsChanneling) {
                castType = CHANNEL;
            }

            SaveCastParams(&gLastNormalCastParams, playerUnit, spellId, item, guid,
                           spell->StartRecoveryCategory,
                           castTime,
                           currentTime, castType, 0);
        }

        // skip queueing if gNoQueueCast is set
        // skip queueing if spellIsChanneling and gUserSettings.queueChannelingSpells is false
        if (!gNoQueueCast && (!spellIsChanneling || gUserSettings.queueChannelingSpells)) {
            if (spellIsTargeting) {
                if (gUserSettings.queueTargetingSpells) {
                    if (castTime > 0 && inSpellQueueWindow) {
                        if (gUserSettings.queueCastTimeSpells) {
                            DEBUG_LOG("Queuing targeting for after cast/gcd: " << remainingCD << "ms " << spellName);
                            TriggerSpellQueuedEvent(NORMAL_QUEUED, spellId);
                            gCastData.normalSpellQueued = true;
                            return false;
                        }
                    } else if (inSpellQueueWindow) {
                        if (gUserSettings.queueInstantSpells) {
                            if (spellOnGcd) {
                                DEBUG_LOG("Queuing instant cast targeting for after cast/gcd: " << remainingCD << "ms "
                                                                                                << spellName);
                                TriggerSpellQueuedEvent(NORMAL_QUEUED, spellId);
                                gCastData.normalSpellQueued = true;
                                return false;
                            } else if (remainingEffectiveCastTime > 0) {
                                auto castParams = gNonGcdCastQueue.findSpellId(spellId);
                                if (castParams) {
                                    DEBUG_LOG("Updating instant cast non GCD targeting params for " << spellName);
                                    castParams->guid = guid;
                                    return false;
                                } else {
                                    DEBUG_LOG("Queuing instant cast non GCD targeting for after cast/gcd: "
                                                      << remainingEffectiveCastTime << "ms " << spellName
                                                      << " gcd category "
                                                      << spell->StartRecoveryCategory);

                                    gNonGcdCastQueue.push({0, playerUnit, spellId, item, guid,
                                                           spell->StartRecoveryCategory,
                                                           castTime,
                                                           0,
                                                           ::NON_GCD,
                                                           false}, gUserSettings.replaceMatchingNonGcdCategory);
                                    TriggerSpellQueuedEvent(NON_GCD_QUEUED, spellId);
                                    gCastData.nonGcdSpellQueued = true;
                                    return false;
                                }
                            }
                        }
                    }
                }
            } else if (castTime > 0 && inSpellQueueWindow) {
                if (gUserSettings.queueCastTimeSpells) {
                    if (spellOnGcd) {
                        DEBUG_LOG("Queuing for after cast/gcd: " << remainingCD << "ms " << spellName);
                        TriggerSpellQueuedEvent(NORMAL_QUEUED, spellId);
                        gCastData.normalSpellQueued = true;
                        return false;
                    } else if (remainingEffectiveCastTime > 0) {
                        auto castParams = gNonGcdCastQueue.findSpellId(spellId);
                        if (castParams) {
                            DEBUG_LOG("Updating non GCD params for " << spellName);
                            castParams->guid = guid;
                            return false;
                        } else {
                            DEBUG_LOG("Queuing non GCD for after cast/gcd: "
                                              << remainingEffectiveCastTime << "ms " << spellName << " gcd category "
                                              << spell->StartRecoveryCategory);

                            gNonGcdCastQueue.push({0, playerUnit, spellId, item, guid,
                                                   spell->StartRecoveryCategory,
                                                   castTime,
                                                   0,
                                                   ::NON_GCD,
                                                   false}, gUserSettings.replaceMatchingNonGcdCategory);
                            TriggerSpellQueuedEvent(NON_GCD_QUEUED, spellId);
                            gCastData.nonGcdSpellQueued = true;
                            return false;
                        }
                    }
                }
            } else if (inSpellQueueWindow) {
                if ((spellIsChanneling && gUserSettings.queueChannelingSpells) ||
                    (!spellIsChanneling && gUserSettings.queueInstantSpells)) {

                    auto desc = "instant cast";
                    if (spellIsChanneling) {
                        desc = "channeling";
                    }

                    if (spellOnGcd) {
                        DEBUG_LOG("Queuing " << desc << " for after cast/gcd: " << remainingCD << "ms " << spellName);
                        TriggerSpellQueuedEvent(NORMAL_QUEUED, spellId);
                        gCastData.normalSpellQueued = true;
                        return false;
                    } else if (remainingEffectiveCastTime > 0) {
                        auto castParams = gNonGcdCastQueue.findSpellId(spellId);
                        if (castParams) {
                            DEBUG_LOG("Updating " << desc << " non GCD params for " << spellName);
                            castParams->guid = guid;
                            return false;
                        } else {
                            DEBUG_LOG("Queuing " << desc << " non GCD for after cast/gcd: "
                                                 << remainingEffectiveCastTime << "ms " << spellName << " gcd category "
                                                 << spell->StartRecoveryCategory);

                            gNonGcdCastQueue.push({0, playerUnit, spellId, item, guid,
                                                   spell->StartRecoveryCategory,
                                                   castTime,
                                                   0,
                                                   ::NON_GCD,
                                                   false}, gUserSettings.replaceMatchingNonGcdCategory);
                            TriggerSpellQueuedEvent(NON_GCD_QUEUED, spellId);
                            gCastData.nonGcdSpellQueued = true;
                            return false;
                        }
                    }
                }
            }
        }

        if (!isSpecialSpell) {
            // is there a cast? (ignore for on swing spells)
            if (remainingEffectiveCastTime) {
                DEBUG_LOG("Cast or delay active " << remainingEffectiveCastTime << "ms remaining");
                return false;
            } else {
                gCastData.castEndMs = 0;
            }

            // is there a Gcd?
            if (spellOnGcd && remainingGcd) {
                DEBUG_LOG("Gcd active " << remainingGcd << "ms remaining");
                return false;
            } else {
                gCastData.gcdEndMs = 0;
            }
        }

        // prevent casting instant cast spells and spells with SPELL_ATTR_DISABLED_WHILE_ACTIVE
        // if cast in the last second and still waiting for server result or succeeded
        // otherwise can break cooldown in the client and cause unnecessary errors
        if (castTime == 0 || spell->Attributes & game::SPELL_ATTR_DISABLED_WHILE_ACTIVE) {
            auto castParams = gCastHistory.findNewestWaitingForServerSpellId(spellId);
            if (castParams &&
                currentTime - castParams->castStartTimeMs < 750) {
                DEBUG_LOG("Ignoring " << spellName
                                      << " cast still waiting for server result for the same spell");
                return false;
            } else {
                castParams = gCastHistory.findNewestSuccessfulSpellId(spellId);
                if (castParams &&
                    currentTime - castParams->castStartTimeMs < 750) {
                    DEBUG_LOG("Ignoring " << spellName
                                          << " cast recently succeeded for the same spell");
                    return false;
                }
            }
        }

        // try clearing current casting spell id if
        // not using tradeskill or enchant
        // no on swing spell queued (will interrupt them)
        if (!isSpecialSpell && !gCastData.pendingOnSwingCast) {
            clearCastingSpell();
        }

        // add to cast history
        auto castType = CastType::NORMAL;
        if (spellIsChanneling) {
            castType = CastType::CHANNEL;
        } else if (spellIsTargeting) {
            if (spellOnGcd) {
                castType = CastType::TARGETING;
            } else {
                castType = CastType::TARGETING_NON_GCD;
            }
        } else if (!spellOnGcd) {
            castType = CastType::NON_GCD;
        }

        gCastHistory.pushFront({gNextCastId, playerUnit, spellId, item, guid,
                                spell->StartRecoveryCategory,
                                castTime,
                                currentTime,
                                castType,
                                gCastData.numRetries,
                                CastResult::WAITING_FOR_CAST});
        gNextCastId++;

        auto ret = castSpell(playerUnit, spellId, item, guid);

        // if this is a trade skill or item enchant, do nothing further
        if (isSpecialSpell) {
            TriggerSpellCastEvent(ret, spellId, castType, guid, itemId);
            return ret;
        }

        // haven't gotten spell result from the previous cast yet, probably due to latency.
        // simulate a cancel to clear the cast bar but only when there should be a cast time
        // mining/herbing have cast time but aren't on Gcd, don't cancel them
        if (!ret && gLastCastData.castTimeMs > 0 && gLastCastData.wasOnGcd) {
            if (*reinterpret_cast<int *>(Offsets::SpellIsTargeting) == 0 && !gCastData.pendingOnSwingCast &&
                !IsSpellOnCooldown(spellId)) {
                DEBUG_LOG("Canceling spell cast due to previous spell having cast time of "
                                  << gLastCastData.castTimeMs);

                //JT: Suggest replacing CancelSpell with InterruptSpell (the API called when moving during casting).
                // The address of InterruptSpell needs to be dug out. It could possibly fix the sometimes broken animations.
                gCastData.cancellingSpell = true;

                auto const cancelSpell = reinterpret_cast<CancelSpellT>(Offsets::CancelSpell);
                cancelSpell(false, false, game::SPELL_FAILED_ERROR);

                gCastData.cancellingSpell = false;

                clearCastingSpell();

                // try again now that cast bar is gone
                ret = castSpell(playerUnit, spellId, item, guid);

                auto const cursorMode = *reinterpret_cast<int *>(Offsets::CursorMode);
                if (!ret && !(spell->Attributes & game::SPELL_ATTR_RANGED) && cursorMode != 2) {
                    DEBUG_LOG("Retry cast after cancel still failed");
                }
            } else {
                DEBUG_LOG("Initial cast failed but not canceling spell cast");
            }
        }

        TriggerSpellCastEvent(ret, spellId, castType, guid, itemId);

        return ret;
    }

    void
    SpellGoHook(hadesmem::PatchDetourBase *detour, uint64_t *casterGUID, uint64_t *targetGUID, uint32_t spellId,
                CDataStore *spellData) {
        auto const spellGo = detour->GetTrampolineT<SpellGoT>();
        spellGo(casterGUID, targetGUID, spellId, spellData);

        auto const castByActivePlayer = game::ClntObjMgrGetActivePlayerGuid() == *casterGUID;

        if (castByActivePlayer) {
            auto const currentTime = GetTime();
            // only care about our own casts
            if (!gCastData.channeling) {
                // check if spell is on swing
                auto const spell = game::GetSpellInfo(spellId);
                if (spell->Attributes & game::SPELL_ATTR_ON_NEXT_SWING_1) {
                    gLastCastData.onSwingStartTimeMs = currentTime;

                    gCastData.pendingOnSwingCast = false;

                    if (gCastData.onSwingQueued) {
                        DEBUG_LOG("On swing spell " << game::GetSpellName(spellId) <<
                                                    " resolved, casting queued on swing spell "
                                                    << game::GetSpellName(gLastOnSwingCastParams.spellId));


                        TriggerSpellQueuedEvent(ON_SWING_QUEUE_POPPED, gLastOnSwingCastParams.spellId);
                        Spell_C_CastSpellHook(castSpellDetour, gLastOnSwingCastParams.playerUnit,
                                              gLastOnSwingCastParams.spellId,
                                              gLastOnSwingCastParams.item, gLastOnSwingCastParams.guid);
                        gCastData.onSwingQueued = false;
                    }
                }
            }
        }
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

            if (spell->Targets == game::SpellTarget::TARGET_LOCATION_UNIT_POSITION &&
                spell->Effect[0] != game::SPELL_EFFECT_SUMMON_GUARDIAN) {
                // if quickcast is on instantly trigger all casts
                // otherwise if this is a queued cast, trigger it instant cast
                if (gUserSettings.quickcastTargetingSpells ||
                    (gUserSettings.queueTargetingSpells && gCastData.castingQueuedSpell)) {
                    DEBUG_LOG("Quickcasting terrain spell " << spellName
                                                            << " quickcast: "
                                                            << gUserSettings.quickcastTargetingSpells
                                                            << " queuetrigger: " << gCastData.castingQueuedSpell);

                    // store the current target
                    auto const targetGuid = game::GetCurrentTargetGuid();

                    LuaCall("CameraOrSelectOrMoveStart()");
                    LuaCall("CameraOrSelectOrMoveStop()");


                    // check if target changed
                    if (targetGuid != game::GetCurrentTargetGuid()) {
                        DEBUG_LOG("Target changed during quick cast, restoring previous target " << targetGuid);
                        auto const targetUnit = reinterpret_cast<CGGameUI_TargetT>(Offsets::CGGameUI_Target);
                        targetUnit(targetGuid);
                    }

                }
            }
        }
        return result;
    }

    void
    CancelSpellHook(hadesmem::PatchDetourBase *detour, bool failed, bool notifyServer,
                    game::SpellCastResult reason) {
        // triggered by us, reset the cast bar
        if (notifyServer) {
            ResetCastFlags();
        } else if (failed) {
            DEBUG_LOG("Cancel spell cast failed:" << failed <<
                                                  " notifyServer:" << notifyServer << " reason:" << int(reason));
        }

        auto const cancelSpell = detour->GetTrampolineT<CancelSpellT>();
        return cancelSpell(failed, notifyServer, reason);
    }

    void SendCastHook(hadesmem::PatchDetourBase *detour, game::SpellCast *cast, char unk) {
        auto const sendCast = detour->GetTrampolineT<SendCastT>();
        sendCast(cast, unk);

        auto const spell = game::GetSpellInfo(cast->spellId);
        BeginCast(gCastData.attemptedCastTimeMs, spell, cast);
    }

}