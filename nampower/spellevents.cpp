//
// Created by pmacc on 9/21/2024.
//

#include "spellevents.hpp"
#include "offsets.hpp"
#include "logging.hpp"
#include "spellcast.hpp"
#include "helper.hpp"

namespace Nampower {
    uint32_t lastCastResultTimeMs;

    void SignalEventHook(hadesmem::PatchDetourBase *detour, game::Events eventId) {
        auto const signalEvent = detour->GetTrampolineT<SignalEventT>();
        signalEvent(eventId);
    }

    uint32_t Script_SpellTargetUnitHook(hadesmem::PatchDetourBase *detour, uintptr_t *luaState) {
        auto const spellTargetUnit = detour->GetTrampolineT<LuaScriptT>();

        // check if valid string
        auto const lua_isstring = reinterpret_cast<lua_isstringT>(Offsets::lua_isstring);
        if (lua_isstring(luaState, 1)) {
            auto const lua_tostring = reinterpret_cast<lua_tostringT>(Offsets::lua_tostring);
            auto const unitName = lua_tostring(luaState, 1);

            auto const getGUIDFromName = reinterpret_cast<GetGUIDFromNameT>(Offsets::GetGUIDFromName);
            auto const guid = getGUIDFromName(unitName);
            if (guid) {
                DEBUG_LOG("Spell target unit " << unitName << " guid " << guid);
                // update all cast params so we don't have to figure out which one to use
                gLastNormalCastParams.guid = guid;
                gLastOnSwingCastParams.guid = guid;

                auto nonGcdCastParams = gNonGcdCastQueue.peek();
                if (nonGcdCastParams) {
                    nonGcdCastParams->guid = guid;
                }
            }
        }

        return spellTargetUnit(luaState);
    }

    void Spell_C_SpellFailedHook(hadesmem::PatchDetourBase *detour, uint32_t spellId,
                                 game::SpellCastResult spellResult, int unk1, int unk2, char unk3) {
        auto const spellFailed = detour->GetTrampolineT<Spell_C_SpellFailedT>();
        spellFailed(spellId, spellResult, unk1, unk2, unk3);

        // ignore fake failure (used by Unleashed Potential and not sure what else)
        if (spellResult == game::SpellCastResult::SPELL_FAILED_DONT_REPORT) {
            return;
        }

        // ignore SPELL_FAILED_CANT_DO_THAT_YET for arcane surge gets sent all the time after success
        if (spellResult == game::SpellCastResult::SPELL_FAILED_CANT_DO_THAT_YET &&
            (spellId == 51933 ||
             spellId == 51934 ||
             spellId == 51935 ||
             spellId == 51936)
                ) {
            return;
        }

        ResetCastFlags();

        if (spellId == gCastData.onSwingSpellId) {
            ResetOnSwingFlags();
        }

        if ((spellResult == game::SpellCastResult::SPELL_FAILED_NOT_READY ||
             spellResult == game::SpellCastResult::SPELL_FAILED_ITEM_NOT_READY ||
             spellResult == game::SpellCastResult::SPELL_FAILED_SPELL_IN_PROGRESS)
                ) {
            auto const currentTime = GetTime();

            if (spellResult == game::SpellCastResult::SPELL_FAILED_NOT_READY) {
                // check if spell is just on cooldown still
                auto const spellCooldown = GetRemainingCooldownForSpell(spellId);
                if (spellCooldown > 0) {
                    auto castParams = gCastHistory.findSpellId(spellId);
                    if (castParams) {
                        // mark as failed so it can be cast again
                        castParams->castResult = CastResult::SERVER_FAILURE;
                    }

                    // check if we should do cooldown queuing
                    if (gUserSettings.queueSpellsOnCooldown && spellCooldown < gUserSettings.cooldownQueueWindowMs) {
                        if (castParams) {
                            if (castParams->castType == CastType::NON_GCD ||
                                castParams->castType == CastType::TARGETING_NON_GCD) {
                                DEBUG_LOG("Non gcd spell " << game::GetSpellName(spellId) << " is still on cooldown "
                                                           << spellCooldown
                                                           << " queuing retry");
                                gCastData.cooldownNonGcdSpellQueued = true;
                                gCastData.cooldownNonGcdEndMs = spellCooldown + currentTime;

                                TriggerSpellQueuedEvent(QueueEvents::NON_GCD_QUEUED, spellId);
                                gNonGcdCastQueue.push(*castParams, gUserSettings.replaceMatchingNonGcdCategory);
                            } else {
                                DEBUG_LOG("Spell " << game::GetSpellName(spellId) << " is still on cooldown "
                                                   << spellCooldown
                                                   << " queuing retry");
                                gCastData.cooldownNormalSpellQueued = true;
                                gCastData.cooldownNormalEndMs = spellCooldown + currentTime;

                                TriggerSpellQueuedEvent(QueueEvents::NORMAL_QUEUED, spellId);
                                gLastNormalCastParams = *castParams;
                            }
                            return;
                        }
                    } else {
                        DEBUG_LOG("Spell " << game::GetSpellName(spellId) << " is still on cooldown " << spellCooldown);
                        return;
                    }
                }
            }

            if (!gUserSettings.retryServerRejectedSpells) {
                DEBUG_LOG("Cast failed for " << game::GetSpellName(spellId)
                                             << " code " << int(spellResult)
                                             << " not queuing retry due to retryServerRejectedSpells=false");
                return;
            }
            gLastErrorTimeMs = currentTime;

            uint64_t lastCastId = 0;

            // otherwise see if we should retry the cast
            auto castParams = gCastHistory.findSpellId(spellId);
            if (castParams) {
                lastCastId = castParams->castId;
                // if we find non retried cast params and the original cast time is within the last 500ms, retry the cast
                if (castParams->castStartTimeMs > currentTime - 500) {
                    // allow 3 retries
                    if (castParams->numRetries < 3) {
                        castParams->numRetries++; // mark as retried

                        if (castParams->castType == CastType::NON_GCD ||
                            castParams->castType == CastType::TARGETING_NON_GCD) {

                            // see if we have a recent successful cast of this spell
                            auto successfulCastParams = gCastHistory.findNewestSuccessfulSpellId(spellId);
                            if (successfulCastParams && successfulCastParams->castStartTimeMs > currentTime - 1000) {
                                // we found a recent successful cast of this spell, ignore the failure that was the result
                                // of spamming the cast
                                DEBUG_LOG("Cast failed for #" << lastCastId << " " << game::GetSpellName(spellId) << " "
                                                              << " non gcd code " << int(spellResult)
                                                              << ", but a recent cast succeeded, not retrying");
                                return;
                            }

                            DEBUG_LOG("Cast failed for #" << lastCastId << " " << game::GetSpellName(spellId) << " "
                                                          << " non gcd code " << int(spellResult)
                                                          << ", retry " << castParams->numRetries
                                                          << " result " << castParams->castResult);
                            gCastData.delayEndMs = currentTime + gBufferTimeMs; // retry after buffer delay
                            TriggerSpellQueuedEvent(QueueEvents::NON_GCD_QUEUED, spellId);
                            gCastData.nonGcdSpellQueued = true;
                            gNonGcdCastQueue.push(*castParams, gUserSettings.replaceMatchingNonGcdCategory);
                        } else {
                            // if gcd is active, do nothing
                            if (gCastData.gcdEndMs > currentTime) {
                                DEBUG_LOG("Cast failed for #" << lastCastId << " " << game::GetSpellName(spellId) << " "
                                                              << " code " << int(spellResult)
                                                              << ", gcd active not retrying");
                            } else {
                                DEBUG_LOG("Cast failed for #" << lastCastId << " " << game::GetSpellName(spellId) << " "
                                                              << " code " << int(spellResult)
                                                              << ", retry " << castParams->numRetries
                                                              << " result " << castParams->castResult);

                                TriggerSpellQueuedEvent(QueueEvents::NORMAL_QUEUED, spellId);
                                gCastData.normalSpellQueued = true;
                                gLastNormalCastParams = *castParams;
                            }
                        }
                    } else {
                        DEBUG_LOG("Cast failed for #" << lastCastId << " " << game::GetSpellName(spellId) << " "
                                                      << " code " << int(spellResult)
                                                      << ", not retrying as it has already been retried 3 times");
                    }
                } else {
                    DEBUG_LOG("Cast failed for #" << lastCastId << " " << game::GetSpellName(spellId) << " "
                                                  << " code " << int(spellResult)
                                                  << ", no recent cast params found in history");
                }


                // check if we should increase the buffer time
                if (currentTime - gLastBufferIncreaseTimeMs > BUFFER_INCREASE_FREQUENCY) {
                    // check if gBufferTimeMs is already at max
                    if (gBufferTimeMs - gUserSettings.minBufferTimeMs < gUserSettings.maxBufferIncreaseMs) {
                        gBufferTimeMs += DYNAMIC_BUFFER_INCREMENT;
                        DEBUG_LOG("Increasing buffer to " << gBufferTimeMs);
                        gLastBufferIncreaseTimeMs = currentTime;
                    }
                }
            }
        } else if (gCastData.normalSpellQueued) {
            DEBUG_LOG("Cast failed for " << game::GetSpellName(spellId)
                                         << " spell queued + ignored code " << int(spellResult));
        } else {
            DEBUG_LOG("Cast failed for " << game::GetSpellName(spellId)
                                         << " ignored code " << int(spellResult));
        }
    }

    int SpellCooldownHandlerHook(hadesmem::PatchDetourBase *detour, uint32_t *opCode, CDataStore *packet) {
        auto const spellCooldownHandler = detour->GetTrampolineT<PacketHandlerT>();

        auto const rpos = packet->m_read;

        if (packet->m_size == 16) {
            uint64_t targetGuid;
            packet->Get(targetGuid);

            uint32_t spellId;
            packet->Get(spellId);

            uint32_t cooldown;
            packet->Get(cooldown);

            packet->m_read = rpos;

            DEBUG_LOG("Spell cooldown opcode:" << opCode << " for " << game::GetSpellName(spellId) << " cooldown "
                                               << cooldown);
        }

        return spellCooldownHandler(opCode, packet);
    }

    void Spell_C_CooldownEventTriggeredHook(hadesmem::PatchDetourBase *detour,
                                            uint32_t spellId,
                                            uint64_t *targetGUID,
                                            int param_3,
                                            int clearCooldowns) {
        DEBUG_LOG("Cooldown event triggered for " << game::GetSpellName(spellId) << " " <<
                                                  targetGUID << " " << param_3 << " " << clearCooldowns);

        auto const cooldownEventTriggered = detour->GetTrampolineT<Spell_C_CooldownEventTriggeredT>();
        cooldownEventTriggered(spellId, targetGUID, param_3, clearCooldowns);
    }

    int SpellDelayedHook(hadesmem::PatchDetourBase *detour, uint32_t *opCode, CDataStore *packet) {
        auto const spellDelayed = detour->GetTrampolineT<PacketHandlerT>();

        auto const rpos = packet->m_read;

        uint64_t guid;
        packet->Get(guid);

        uint32_t delay;
        packet->Get(delay);

        packet->m_read = rpos;

        auto const activePlayer = game::ClntObjMgrGetActivePlayerGuid();

        if (guid == activePlayer) {
            auto const currentTime = GetTime();

            // if we are casting a spell, and it was delayed, update our own state so we do not allow a cast too soon
            if (currentTime < gCastData.castEndMs) {
                gCastData.castEndMs += delay;

                auto lastCastParams = gCastHistory.peek();
                if (lastCastParams != nullptr) {
                    DEBUG_LOG("Spell delayed by " << delay << " for cast #" << lastCastParams->castId << " "
                                                  << game::GetSpellName(lastCastParams->spellId)
                                                  << " cast end " << gCastData.castEndMs);
                }
            }
        }

        return spellDelayed(opCode, packet);
    }


    int CastResultHandlerHook(hadesmem::PatchDetourBase *detour, uint32_t *opCode, CDataStore *packet) {
        auto const castResultHandler = detour->GetTrampolineT<PacketHandlerT>();

        auto const rpos = packet->m_read;

        uint32_t spellId;
        packet->Get(spellId);

        uint8_t status;
        packet->Get(status);

        uint8_t spellCastResult;

        if (status != 0) {
            packet->Get(spellCastResult);
        } else {
            spellCastResult = 0;
        }

        packet->Get(spellCastResult);

        packet->m_read = rpos;

        auto const currentLatency = GetLatencyMs();
        auto currentTime = GetTime();

        // Reset the server delay in case we aren't able to calculate it
        gLastServerSpellDelayMs = 0;

        // if the cast was successful and the spell cast result was successful, and we have a latency
        // attempt to calculate the server delay
        if (status == 0 && spellCastResult == 0 && currentLatency > 0) {
            // try to find the cast in the history
            auto maxStartTime = currentTime - currentLatency;
            auto castParams = gCastHistory.findOldestWaitingForServerSpellId(spellId);

            if (castParams) {
                // running spellResponseTimeMs average
                auto const lastCastTime = castParams->castStartTimeMs;
                auto const spellResponseTimeMs = int32_t(currentTime - lastCastTime);

                auto const serverDelay = spellResponseTimeMs - int32_t(currentLatency + castParams->castTimeMs);

                if (serverDelay > 0) {
                    gLastServerSpellDelayMs = serverDelay + 15;
                } else {
                    DEBUG_LOG("Negative server delay using 1 ms " << serverDelay);
                    gLastServerSpellDelayMs = 1;
                }
            }
        }

        uint64_t matchingCastId = 0;

        // update cast history
        if (status == 0) {
            auto castParams = gCastHistory.findOldestWaitingForServerSpellId(spellId);
            if (castParams) {
                // successes normally for the oldest cast
                castParams->castResult = CastResult::SERVER_SUCCESS;
                matchingCastId = castParams->castId;
            }
        } else {
            auto castParams = gCastHistory.findNewestWaitingForServerSpellId(spellId);
            if (castParams) {
                // failures normally caused by the latest cast
                castParams->castResult = CastResult::SERVER_FAILURE;
                matchingCastId = castParams->castId;
            }
        }

        if (gLastServerSpellDelayMs > 0) {
            DEBUG_LOG("Cast result for #" << matchingCastId << " "
                                          << game::GetSpellName(spellId)
                                          << "(" << spellId << ")"
                                          << " status " << int(status)
                                          << " result " << int(spellCastResult) << " latency " << currentLatency
                                          << " server delay " << gLastServerSpellDelayMs
                                          << " since last cast result " << currentTime - lastCastResultTimeMs);
        } else {
            DEBUG_LOG("Cast result for #" << matchingCastId << " "
                                          << game::GetSpellName(spellId)
                                          << "(" << spellId << ")"
                                          << " status " << int(status)
                                          << " result " << int(spellCastResult) << " latency " << currentLatency
                                          << " since last cast result " << currentTime - lastCastResultTimeMs);
        }

        lastCastResultTimeMs = currentTime;

        return castResultHandler(opCode, packet);
    }

    int SpellFailedHandlerHook(hadesmem::PatchDetourBase *detour, uint32_t *opCode, CDataStore *packet) {
        auto const spellFailedHandler = detour->GetTrampolineT<PacketHandlerT>();

        auto const rpos = packet->m_read;

        uint64_t guid;
        packet->Get(guid);

        uint32_t spellId;
        packet->Get(spellId);

        packet->m_read = rpos;

        DEBUG_LOG("Spell failed opcode:" << opCode << " for " << game::GetSpellName(spellId) << " guid " << guid);

        return spellFailedHandler(opCode, packet);
    }

    int SpellStartHandlerHook(hadesmem::PatchDetourBase *detour, uint32_t unk, uint32_t opCode, uint32_t unk2,
                              CDataStore *packet) {
        auto const spellStartHandler = detour->GetTrampolineT<FastCallPacketHandlerT>();

        auto const rpos = packet->m_read;

        uint32_t previousVisualSpellId = 0;

        if (opCode == 0x131) {
            // 8 + 8 + 4 + 2 + 4 but first 2 guids are packed
            uint64_t itemGuid;
            packet->GetPackedGuid(itemGuid);

            uint64_t casterGuid;
            packet->GetPackedGuid(casterGuid);

            if (casterGuid == game::ClntObjMgrGetActivePlayerGuid()) {
                uint32_t spellId;
                packet->Get(spellId);

                uint16_t castFlags;
                packet->Get(castFlags);

                uint32_t castTime;
                packet->Get(castTime);

                bool isAutoRepeat = false;

                auto spellInfo = game::GetSpellInfo(spellId);
                if (spellInfo) {
                    isAutoRepeat = spellInfo->AttributesEx2 & game::SpellAttributesEx2::SPELL_ATTR_EX2_AUTO_REPEAT;
                }

                auto castParams = gCastHistory.findSpellId(spellId);

                // check if cast time differed from what we expected, ignore haste rounding errors
                if (castParams && castParams->spellId == spellId) {
                    if (!isAutoRepeat && castParams->castTimeMs < castTime) {
                        // server cast time increased
                        auto castTimeDifference = castTime - castParams->castTimeMs;

                        if (castTimeDifference > 5) {
                            gCastData.castEndMs = castParams->castStartTimeMs + castTime;
                            if (castTime > 0) {
                                gCastData.castEndMs += gBufferTimeMs;
                            }

                            castParams->castTimeMs = castTime;
                            DEBUG_LOG("Server cast time for " << game::GetSpellName(spellId) << " increased by "
                                                              << castTimeDifference << "ms.  Updated cast end time to "
                                                              << gCastData.castEndMs);
                        }
                    } else if (!isAutoRepeat && castParams->castTimeMs > castTime) {
                        // server cast time decreased
                        auto castTimeDifference = castParams->castTimeMs - castTime;

                        if (castTimeDifference > 5) {
                            auto gcdTime = GetGcdOrCooldownForSpell(spellId);
                            if (gcdTime > 1500) {
                                gcdTime = 1500; // items with spells on gcd will return their item gcd, make sure not to use that
                            }

                            if (gcdTime > castTime) {
                                DEBUG_LOG("Server cast time " << castTime << " was reduced below gcd of " << gcdTime
                                                              << " using gcd instead");
                                castTime = gcdTime;
                                gCastData.castEndMs = castParams->castStartTimeMs + gcdTime;
                            } else {
                                gCastData.castEndMs = castParams->castStartTimeMs + castTime + gBufferTimeMs;

                            }

                            castParams->castTimeMs = castTime;
                            DEBUG_LOG("Server cast time for " << game::GetSpellName(spellId) << " decreased by "
                                                              << castTimeDifference << "ms.  Updated cast end time to "
                                                              << gCastData.castEndMs);
                        }
                    }

                    // spell start successful, reset castParams->numRetries
                    castParams->numRetries = 0;
                }

                // spell start successful, reset gLastNormalCastParams.numRetries
                gLastNormalCastParams.numRetries = 0;

                // avoid clearing visual spell id as much as possible as it can cause weird sound/animation issues
                // only do it for normal gcd spells with a cast time
                if (castTime > 0 && gLastCastData.wasQueued && gLastCastData.wasOnGcd && !gLastCastData.wasItem) {
                    auto visualSpellId = reinterpret_cast<uint32_t *>(Offsets::VisualSpellId);
                    // only clear if the current visual spell id is the same as the spell we are casting
                    // as that seems to be when cast animation breaks
                    if (*visualSpellId == spellId) {
                        *visualSpellId = 0;

                        previousVisualSpellId = spellId;
                    }
                }
            }
        }

        packet->m_read = rpos;

        auto result = spellStartHandler(unk, opCode, unk2, packet);

        if (previousVisualSpellId > 0) {
            auto currentSpellId = reinterpret_cast<uint32_t *>(Offsets::VisualSpellId);
            *currentSpellId = previousVisualSpellId;
        }

        return result;
    }

    void
    TriggerSpellDamageEvent(uint64_t targetGuid,
                            uint64_t casterGuid,
                            uint32_t spellId,
                            uint32_t amount,
                            uint32_t spellSchool,
                            uint32_t absorb,
                            uint32_t blocked,
                            int32_t resist,
                            uint32_t auraType,
                            uint32_t hitInfo) {
        char format[] = "%s%s%d%d%s%d%d%s";

        char *targetGuidStr = ConvertGuidToString(targetGuid);

        char *casterGuidStr = ConvertGuidToString(casterGuid);

        auto event = game::SPELL_DAMAGE_EVENT_OTHER;

        if (casterGuid == game::ClntObjMgrGetActivePlayerGuid()) {
            event = game::SPELL_DAMAGE_EVENT_SELF;

            if (gCastData.channeling && gCastData.channelSpellId == spellId) {
                gCastData.channelNumTicks++;
            }
        }

        auto spell = game::GetSpellInfo(spellId);

        std::ostringstream mitigationStream;
        mitigationStream << absorb << "," << blocked << "," << resist;

        std::ostringstream effectsAuraTypeStream;
        if (spell) {
            effectsAuraTypeStream << spell->Effect[0] << "," << spell->Effect[1] << "," << spell->Effect[2];
        } else {
            effectsAuraTypeStream << "0,0,0";
        }
        // add aura type to the end
        effectsAuraTypeStream << "," << auraType;

        ((int (__cdecl *)(int eventCode,
                          char *format,
                          char *targetGuid,
                          char *casterGuid,
                          uint32_t spellId,
                          uint32_t amount,
                          const char *mitigationStr,
                          uint32_t hitInfo,
                          uint32_t spellSchool,
                          const char *effectAuraStr)) Offsets::SignalEventParam)(
                event,
                format,
                targetGuidStr,
                casterGuidStr,
                spellId,
                amount,
                mitigationStream.str().c_str(),
                hitInfo,
                spellSchool,
                effectsAuraTypeStream.str().c_str());
    }

    int PeriodicAuraLogHandlerHook(hadesmem::PatchDetourBase *detour, uint32_t unk, uint32_t opCode, uint32_t unk2,
                                   CDataStore *packet) {
        auto const rpos = packet->m_read;

        uint64_t targetGuid;
        packet->GetPackedGuid(targetGuid);

        uint64_t casterGuid;
        packet->GetPackedGuid(casterGuid);

        uint32_t spellId;
        packet->Get(spellId);

        uint32_t count;
        packet->Get(count);

        uint32_t auraType;
        packet->Get(auraType);

        uint32_t amount;
        uint32_t powerType;

        switch (auraType) {
            case 3: // SPELL_AURA_PERIODIC_DAMAGE
            case 89: // SPELL_AURA_PERIODIC_DAMAGE_PERCENT
                packet->Get(amount);  // damage amount

                uint32_t spellSchool;
                packet->Get(spellSchool);

                uint32_t absorb;
                packet->Get(absorb);

                int32_t resist;
                packet->Get(resist);

                TriggerSpellDamageEvent(targetGuid, casterGuid, spellId, amount, spellSchool, absorb, 0, resist,
                                        auraType,
                                        0);
                break;
            case 8: // SPELL_AURA_PERIODIC_HEAL
            case 20: // SPELL_AURA_OBS_MOD_HEALTH
                packet->Get(amount);  // heal amount

                // No custom event for these for now
                break;
            case 21: // SPELL_AURA_OBS_MOD_MANA
            case 24: // SPELL_AURA_PERIODIC_ENERGIZE
                packet->Get(powerType);

                packet->Get(amount);  // power type amount

                // No custom event for these for now
                break;
            case 64: // SPELL_AURA_PERIODIC_MANA_LEECH
                packet->Get(powerType);
                packet->Get(amount);  // power type amount

                uint32_t multiplier;
                packet->Get(multiplier);  // gain multiplier

                // No custom event for these for now
                break;
            default:
                break;
        }

        packet->m_read = rpos;

        auto const periodicAuraLogHandler = detour->GetTrampolineT<FastCallPacketHandlerT>();
        return periodicAuraLogHandler(unk, opCode, unk2, packet);
    }

    int SpellNonMeleeDmgLogHandlerHook(hadesmem::PatchDetourBase *detour, uint32_t unk, uint32_t opCode, uint32_t unk2,
                                       CDataStore *packet) {
        auto const rpos = packet->m_read;

        uint64_t targetGuid;
        packet->GetPackedGuid(targetGuid);

        uint64_t casterGuid;
        packet->GetPackedGuid(casterGuid);

        uint32_t spellId;
        packet->Get(spellId);

        uint32_t damage;
        packet->Get(damage);

        uint8_t school;
        packet->Get(school);

        uint32_t absorb;
        packet->Get(absorb);

        int32_t resist;
        packet->Get(resist);

        uint8_t periodicLog;
        packet->Get(periodicLog);

        uint8_t unused;
        packet->Get(unused);

        uint32_t blocked;
        packet->Get(blocked);

        uint32_t hitInfo;
        packet->Get(hitInfo);

        uint8_t extendData;
        packet->Get(extendData);

        packet->m_read = rpos;

        TriggerSpellDamageEvent(targetGuid, casterGuid, spellId, damage, school, absorb, blocked, resist, 0, hitInfo);

        auto const spellNonMeleeDmgLogHandler = detour->GetTrampolineT<FastCallPacketHandlerT>();
        return spellNonMeleeDmgLogHandler(unk, opCode, unk2, packet);
    }

    int PlaySpellVisualHandlerHook(hadesmem::PatchDetourBase *detour, uint32_t *opCode, CDataStore *packet) {
        auto const playSpellVisualHandler = detour->GetTrampolineT<PacketHandlerT>();

        auto const rpos = packet->m_read;

        uint64_t targetGuid;
        packet->GetPackedGuid(targetGuid);

        uint32_t spellVisualKitIndex;
        packet->Get(spellVisualKitIndex);

        packet->m_read = rpos;

        DEBUG_LOG("Play spell visual id " << spellVisualKitIndex << " for guid " << targetGuid);

//        return playSpellVisualHandler(opCode, packet);

        return 1;
    }
}