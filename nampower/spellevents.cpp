//
// Created by pmacc on 9/21/2024.
//

#include "spellevents.hpp"
#include "offsets.hpp"

namespace Nampower {
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


        if (eventId == game::Events::SPELLCAST_STOP) {
            // if this is from the server, but it is happening too early, it is for one of two reasons.
            // 1) it is for the last cast, in which case we can ignore it
            // 2) it is for our current cast and the server decided to cast sooner than we expected
            //    this can happen from mage 8/8 t2 proc or presence of mind
            DEBUG_LOG("SPELLCAST_STOP occurred");
        }
            // SPELLCAST_FAILED means the attempt was rejected by either the client or the server,
            // depending on the value of gNotifyServer.  if it was rejected by the server, this
            // could mean that our latency has decreased since the previous cast.  when that happens
            // the server perceives too little time as having passed to allow another cast.  i dont
            // think there is anything we can do about this except to honor the servers request to
            // abort the cast.  reset our cooldown and allow
        else if (eventId == game::Events::SPELLCAST_FAILED ||
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
        if (lua_isstring(unitStr, 1)) {
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

    void Spell_C_SpellFailedHook(hadesmem::PatchDetourBase *detour, int spellId,
                                 game::SpellCastResult spellResult, int unk1, int unk2, char unk3) {
        auto const spellFailed = detour->GetTrampolineT<Spell_C_SpellFailedT>();
        spellFailed(spellId, spellResult, unk1, unk2, unk3);

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
                // check if gBufferTimeMs is already at max
                if (gBufferTimeMs - gMinBufferTimeMs < gMaxBufferIncrease) {
                    gBufferTimeMs += DYNAMIC_BUFFER_INCREMENT;
                    DEBUG_LOG("Increasing buffer to " << gBufferTimeMs);
                    gLastBufferIncreaseTimeMs = currentTime;
                }
            }
        } else if (lastDetour) {
            DEBUG_LOG("Cast failed code " << int(spellResult) << " ignored");
        }
    }
}