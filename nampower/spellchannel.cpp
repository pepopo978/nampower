//
// Created by pmacc on 9/21/2024.
//

#include "spellchannel.hpp"
#include "spellcast.hpp"
#include "logging.hpp"
#include "offsets.hpp"

namespace Nampower {
    int SpellChannelStartHandlerHook(hadesmem::PatchDetourBase *detour, uint32_t *opCode, CDataStore *packet) {
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
                gCastData.channelStartMs = currentTimeMs;
                gCastData.channelEndMs = currentTimeMs + duration;
                gCastData.channelSpellId = spellId;
                gCastData.channelDuration = duration;

                auto originalDuration = game::GetDurationObject(spell->DurationIndex)->m_Duration;

                float durationReduction = 1.0f;

                if (originalDuration > 0 && originalDuration < 1000000) {
                    durationReduction = float(duration) / float(originalDuration);
                } else {
                    DEBUG_LOG("Invalid originalDuration of " << originalDuration << " for " << game::GetSpellName(spellId));
                }


                gCastData.channelTickTimeMs = uint32_t(float(spell->EffectAmplitude[0]) *
                                                       durationReduction);  // the base tick time should be the first effect, scale it based on the duration reduction due to haste mechanics

                DEBUG_LOG("Original tick time " << spell->EffectAmplitude[0] << " scaled tick time "
                                                << gCastData.channelTickTimeMs);

                if (gCastData.channelTickTimeMs <= 0 || gCastData.channelTickTimeMs > duration) {
                    DEBUG_LOG("Invalid channel tick time for " << game::GetSpellName(spellId)
                                                               << ", defaulting to duration");
                    gCastData.channelTickTimeMs = duration;
                }

                gCastData.channelNumTicks = 0;

                gLastCastData.channelStartTimeMs = currentTimeMs;
            } else {
                DEBUG_LOG("Ignoring/resetting channeling for " << game::GetSpellName(spellId));
                ResetChannelingFlags();
            }
        } else {
            ResetChannelingFlags();
        }

        auto const spellChannelStartHandler = detour->GetTrampolineT<PacketHandlerT>();
        return spellChannelStartHandler(opCode, packet);
    }

    // this gets called on damage and when channel ends with the end time of the channel
    int SpellChannelUpdateHandlerHook(hadesmem::PatchDetourBase *detour, uint32_t *opCode, CDataStore *packet) {
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

        auto const spellChannelUpdateHandler = detour->GetTrampolineT<PacketHandlerT>();
        return spellChannelUpdateHandler(opCode, packet);
    }
}