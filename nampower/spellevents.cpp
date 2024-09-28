//
// Created by pmacc on 9/21/2024.
//

#include "spellevents.hpp"
#include "offsets.hpp"

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
                nonGcdCastParams->guid = guid;
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
            else if (currentTime - gLastErrorTimeMs < 100) {
                DEBUG_LOG("Cast failed for " << game::GetSpellName(spellId)
                                             << " code " << int(spellResult)
                                             << ", not queuing retry due to recent error at " << gLastErrorTimeMs);
                return;
            }
            gLastErrorTimeMs = currentTime;

            // try to find the cast params for the spellId that failureRetry in spellhistory
            auto castParams = gCastHistory.find(spellId);
            // if we find non retried cast params and the original cast time is within the last 500ms, retry the cast
            if (castParams && castParams->castTimeMs > currentTime - 500) {
                if(!castParams->failureRetry) {
                    if (castParams->castType == CastType::NON_GCD ||
                        castParams->castType == CastType::TARGETING_NON_GCD) {
                        DEBUG_LOG("Cast failed for non gcd " << game::GetSpellName(spellId)
                                                             << " code " << int(spellResult)
                                                             << ", retrying");
                        gCastData.delayEndMs = currentTime + gBufferTimeMs; // retry after buffer delay
                        gCastData.nonGcdSpellQueued = true;
                        castParams->failureRetry = true; // mark as retried
                        gNonGcdCastQueue.push(*castParams);
                    } else {
                        DEBUG_LOG("Cast failed for " << game::GetSpellName(spellId)
                                                     << " code " << int(spellResult)
                                                     << ", retrying");
                        gCastData.normalSpellQueued = true;
                        gLastNormalCastParams = *castParams;
                    }
                } else {
                    DEBUG_LOG("Cast failed for " << game::GetSpellName(spellId)
                                                 << " code " << int(spellResult)
                                                 << ", not retrying as it has already been retried");
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

    int SpellDelayedHook(hadesmem::PatchDetourBase *detour, uint32_t *opCode, game::CDataStore *packet) {
        auto const spellDelayed = detour->GetTrampolineT<PacketHandlerT>();

        auto const rpos = packet->m_read;

        auto const guid = packet->Get<std::uint64_t>();
        auto const delay = packet->Get<std::uint32_t>();

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


    int CastResultHandlerHook(hadesmem::PatchDetourBase *detour, uint32_t *opCode, game::CDataStore *packet) {
        auto const castResultHandler = detour->GetTrampolineT<PacketHandlerT>();
        return castResultHandler(opCode, packet);
    }
}