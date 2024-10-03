//
// Created by pmacc on 9/21/2024.
//

#include "spellevents.hpp"
#include "offsets.hpp"
#include "logging.hpp"

namespace Nampower {
    bool SpellTargetUnitHook(hadesmem::PatchDetourBase *detour, uintptr_t *unitStr) {
        auto const spellTargetUnit = detour->GetTrampolineT<SpellTargetUnitT>();

        // check if valid string
        auto const lua_isstring = reinterpret_cast<lua_isstringT>(Offsets::lua_isstring);
        if (lua_isstring(unitStr, 1)) {
            auto const lua_tostring = reinterpret_cast<lua_tostringT>(Offsets::lua_tostring);
            auto const unitName = lua_tostring(unitStr, 1);

            auto const getGUIDFromName = reinterpret_cast<Script_GetGUIDFromNameT>(Offsets::Script_GetGUIDFromName);
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

        return spellTargetUnit(unitStr);
    }

    void Spell_C_SpellFailedHook(hadesmem::PatchDetourBase *detour, uint32_t spellId,
                                 game::SpellCastResult spellResult, int unk1, int unk2, char unk3) {
        auto const spellFailed = detour->GetTrampolineT<Spell_C_SpellFailedT>();
        spellFailed(spellId, spellResult, unk1, unk2, unk3);

        ResetCastFlags();

        if (gCastData.castingQueuedSpell || gCastData.cancellingSpell) {
            return;
        }

        if ((spellResult == game::SpellCastResult::SPELL_FAILED_NOT_READY ||
             spellResult == game::SpellCastResult::SPELL_FAILED_ITEM_NOT_READY ||
             spellResult == game::SpellCastResult::SPELL_FAILED_SPELL_IN_PROGRESS)
                ) {
            auto const currentTime = GetTime();

            if (!gUserSettings.retryServerRejectedSpells) {
                DEBUG_LOG("Cast failed for " << game::GetSpellName(spellId)
                                             << " code " << int(spellResult)
                                             << " not queuing retry due to retryServerRejectedSpells=false");
                return;
            }
                // ignore error spam
            else if (currentTime - gLastErrorTimeMs < 50) {
                DEBUG_LOG("Cast failed for " << game::GetSpellName(spellId)
                                             << " code " << int(spellResult)
                                             << ", not queuing retry due to recent error at " << gLastErrorTimeMs);
                return;
            }
            gLastErrorTimeMs = currentTime;

            // try to find the cast params for the spellId that numRetries in spellhistory
            auto castParams = gCastHistory.findSpellId(spellId);
            // if we find non retried cast params and the original cast time is within the last 500ms, retry the cast
            if (castParams && castParams->castStartTimeMs > currentTime - 500) {
                // allow 2 retries
                if (castParams->numRetries < 2) {
                    castParams->numRetries++; // mark as retried

                    if (castParams->castType == CastType::NON_GCD ||
                        castParams->castType == CastType::TARGETING_NON_GCD) {
                        DEBUG_LOG("Cast failed for non gcd " << game::GetSpellName(spellId)
                                                             << " code " << int(spellResult)
                                                             << ", retry " << castParams->numRetries);
                        gCastData.delayEndMs = currentTime + gBufferTimeMs; // retry after buffer delay
                        gCastData.nonGcdSpellQueued = true;
                        gNonGcdCastQueue.push(*castParams, gUserSettings.replaceMatchingNonGcdCategory);
                    } else {
                        DEBUG_LOG("Cast failed for " << game::GetSpellName(spellId)
                                                     << " code " << int(spellResult)
                                                     << ", retry " << castParams->numRetries);
                        gCastData.normalSpellQueued = true;
                        gLastNormalCastParams = *castParams;
                    }
                } else {
                    DEBUG_LOG("Cast failed for " << game::GetSpellName(spellId)
                                                 << " code " << int(spellResult)
                                                 << ", not retrying as it has already been retried twice");
                }
            } else {
                DEBUG_LOG("Cast failed for " << game::GetSpellName(spellId)
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
        } else if (gCastData.normalSpellQueued) {
            DEBUG_LOG("Cast failed code " << int(spellResult) << " ignored");
        }
    }

    int SpellCooldownHandlerHook(hadesmem::PatchDetourBase *detour, uint32_t *opCode, CDataStore *packet) {
        auto const spellCooldownHandler = detour->GetTrampolineT<PacketHandlerT>();

        auto const rpos = packet->m_read;

        if(packet->m_size == 16){
            uint64_t targetGuid;
            packet->Get(targetGuid);

            uint32_t spellId;
            packet->Get(spellId);

            uint32_t cooldown;
            packet->Get(cooldown);

            packet->m_read = rpos;

            DEBUG_LOG("Spell cooldown opcode:" << opCode << " for " << game::GetSpellName(spellId) << " cooldown " << cooldown);
        }

        return spellCooldownHandler(opCode, packet);
    }

    void Spell_C_CooldownEventTriggeredHook(hadesmem::PatchDetourBase *detour,
                                            uint32_t spellId,
                                            uint64_t *targetGUID,
                                            int param_3,
                                            int clearCooldowns) {
        DEBUG_LOG("Cooldown event triggered for " << game::GetSpellName(spellId) << " " << targetGUID << " " << param_3
                                                  << " " << clearCooldowns);

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

        auto const activePlayer = game::ClntObjMgrGetActivePlayer();

        if (guid == activePlayer) {
            auto const currentTime = GetTime();

            // if we are casting a spell, and it was delayed, update our own state so we do not allow a cast too soon
            if (currentTime < gCastData.castEndMs) {
                gCastData.castEndMs += delay;
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

        DEBUG_LOG("Cast result opcode:" << opCode << " for " << game::GetSpellName(spellId) << " status " << int(status)
                                        << " result " << int(spellCastResult));

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

            if (casterGuid == game::ClntObjMgrGetActivePlayer()) {
                uint32_t spellId;
                packet->Get(spellId);

                uint16_t castFlags;
                packet->Get(castFlags);

                uint32_t castTime;
                packet->Get(castTime);

                packet->m_read = rpos;

                // check if cast time differed from what we expected, ignore haste rounding errors
                if (gLastNormalCastParams.spellId == spellId && gLastNormalCastParams.castTimeMs - castTime > 5) {
                    if (castTime > gLastNormalCastParams.castTimeMs) {
                        auto castTimeDifference = castTime - gLastNormalCastParams.castTimeMs;
                        gCastData.castEndMs += castTimeDifference;
                        DEBUG_LOG("Server cast time for " << game::GetSpellName(spellId) << " increased by "
                                                          << castTimeDifference << "ms.  Updated cast end time to "
                                                          << gCastData.castEndMs);
                    } else {
                        auto castTimeDifference = gLastNormalCastParams.castTimeMs - castTime;

                        if(gCastData.castEndMs > castTimeDifference) {
                            gCastData.castEndMs -= castTimeDifference;
                            DEBUG_LOG("Server cast time for " << game::GetSpellName(spellId) << " decreased by "
                                                              << castTimeDifference << "ms.  Updated cast end time to "
                                                              << gCastData.castEndMs);
                        }
                    }
                }

                // spell start successful, reset gLastNormalCastParams.numRetries
                gLastNormalCastParams.numRetries = 0;

                if (castTime > 0) {
                    auto currentSpellId = reinterpret_cast<uint32_t *>(Offsets::VisualSpellId);
                    if (*currentSpellId == spellId) {
                        *currentSpellId = 0;

                        previousVisualSpellId = spellId;
                    }
                }
            }
        }

        auto result = spellStartHandler(unk, opCode, unk2, packet);

        if (previousVisualSpellId > 0) {
            auto currentSpellId = reinterpret_cast<uint32_t *>(Offsets::VisualSpellId);
            *currentSpellId = previousVisualSpellId;
        }

        return result;
    }
}