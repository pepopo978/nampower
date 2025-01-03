//
// Created by pmacc on 9/21/2024.
//

#include "spellchannel.hpp"
#include "spellcast.hpp"
#include "logging.hpp"
#include "offsets.hpp"

namespace Nampower {
    int SpellChannelStartHandlerHook(hadesmem::PatchDetourBase *detour, uint32_t currentTime, CDataStore *packet) {
        auto const rpos = packet->m_read;

        uint32_t spellId;
        packet->Get(spellId);

        uint32_t duration;
        packet->Get(duration);

        packet->m_read = rpos;

        DEBUG_LOG("Channel start: " << game::GetSpellName(spellId) << " duration " << duration);

        if (duration > 0) {
            // check if spell has "Far sight" flag which is used on mind control style abilities
            auto spell = game::GetSpellInfo(spellId);
            if (spell && !(spell->AttributesEx & game::SPELL_ATTR_EX_TOGGLE_FARSIGHT ||
                           spellId == 19832 || // bwl orb spell
                           spellId == 23014 || // bwl orb spell
                           spellId == 13180) // mind control cap
                    ) {
                auto currentTimeMs = GetTime();
                gCastData.channeling = true;
                gCastData.channelEndMs = currentTimeMs + duration;
                gCastData.channelSpellId = spellId;
                gCastData.channelDuration = duration;
                gCastData.channelCastCount = 0;
                gCastData.channelLastCastTimeMs = 0;

                gLastCastData.channelStartTimeMs = currentTimeMs;
            } else {
                DEBUG_LOG("Ignoring channel start for " << game::GetSpellName(spellId));
            }

        } else {
            ResetChannelingFlags();
        }

        auto const spellChannelStartHandler = detour->GetTrampolineT<SpellChannelStartHandlerT>();
        return spellChannelStartHandler(currentTime, packet);
    }

    // this gets called on damage and when channel ends with the end time of the channel
    int SpellChannelUpdateHandlerHook(hadesmem::PatchDetourBase *detour, uint32_t currentTime, CDataStore *packet) {
        auto const rpos = packet->m_read;

        uint32_t channelRemainingTime;
        packet->Get(channelRemainingTime);

        packet->m_read = rpos;

        if (channelRemainingTime <= 0) {
            DEBUG_LOG("Channel done: " << game::GetSpellName(gCastData.channelSpellId)
                                       << " elapsed " << (GetTime() - gLastCastData.channelStartTimeMs)
                                       << " original duration " << gCastData.channelDuration);
            ResetChannelingFlags();
        } else {
            DEBUG_LOG("Channel update: " << game::GetSpellName(gCastData.channelSpellId) << " remaining "
                                         << channelRemainingTime);
            gCastData.channelEndMs = GetTime() + channelRemainingTime;
        }

        auto const spellChannelUpdateHandler = detour->GetTrampolineT<SpellChannelUpdateHandlerT>();
        return spellChannelUpdateHandler(currentTime, packet);
    }
}