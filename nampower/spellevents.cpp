//
// Created by pmacc on 9/21/2024.
//

#include "spellevents.hpp"
#include "offsets.hpp"

namespace Nampower {
    // events without parameters go here
    void SignalEventHook(hadesmem::PatchDetourBase *detour, game::Events eventId) {
        // SPELLCAST_FAILED means the attempt was rejected by either the client or the server,
        // depending on the value of gNotifyServer.  if it was rejected by the server, this
        // could mean that our latency has decreased since the previous cast.  when that happens
        // the server perceives too little time as having passed to allow another cast.  i dont
        // think there is anything we can do about this except to honor the servers request to
        // abort the cast.  reset our cooldown and allow
        if (eventId == game::Events::SPELLCAST_FAILED ||
            eventId ==
            game::Events::SPELLCAST_INTERRUPTED) //JT: moving to cancel the spell sends "SPELLCAST_INTERRUPTED"
        {
            ResetCastFlags();
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
            if (currentTime < gCastData.castEndMs) {
                gCastData.castEndMs += delay;
            }
        }

        return spellDelayed(opCode, packet);
    }

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
                gLastNormalCastParams.guid = guid;
                gLastNonGcdCastParams.guid = guid;
            }
        }

        return spellTargetUnit(unitStr);
    }

    void Spell_C_SpellFailedHook(hadesmem::PatchDetourBase *detour, int spellId,
                                 game::SpellCastResult spellResult, int unk1, int unk2, char unk3) {
        auto const spellFailed = detour->GetTrampolineT<Spell_C_SpellFailedT>();
        spellFailed(spellId, spellResult, unk1, unk2, unk3);

        if ((spellResult == game::SpellCastResult::SPELL_FAILED_NOT_READY ||
             spellResult == game::SpellCastResult::SPELL_FAILED_ITEM_NOT_READY ||
             spellResult == game::SpellCastResult::SPELL_FAILED_SPELL_IN_PROGRESS)
                ) {
            auto const currentTime = GetTime();

            if(!gUserSettings.retryServerRejectedSpells) {
                DEBUG_LOG("Cast failed code " << int(spellResult) << " not queuing retry due to retryServerRejectedSpells=false");
                return;
            }
            // ignore error spam
            else if (currentTime - gLastErrorTimeMs < 200) {
                DEBUG_LOG("Cast failed code " << int(spellResult) << ", not queuing retry due to recent error at "
                                              << gLastErrorTimeMs);
                return;
            } else {
                DEBUG_LOG("Cast failed code " << int(spellResult) << ", queuing a retry");
            }
            gLastErrorTimeMs = currentTime;
            gCastData.normalSpellQueued = true;  // this is fine even for non GCD spells as nothing will be casting or queued

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
}