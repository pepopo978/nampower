//
// Created by pmacc on 9/21/2024.
//

#include "spellchannel.hpp"
#include "spellcast.hpp"
#include "logging.hpp"

namespace Nampower {
    int SpellChannelStartHandlerHook(hadesmem::PatchDetourBase *detour, int channelStart, game::CDataStore *dataPtr) {
        gLastCastData.channelStartTimeMs = GetTime();
        DEBUG_LOG("Channel start " << channelStart);
        if (channelStart) {
            gCastData.channeling = true;
            gCastData.channelCastCount = 0;
        }

        auto const spellChannelStartHandler = detour->GetTrampolineT<SpellChannelStartHandlerT>();
        return spellChannelStartHandler(channelStart, dataPtr);
    }

    int SpellChannelUpdateHandlerHook(hadesmem::PatchDetourBase *detour, int channelUpdate, game::CDataStore *dataPtr) {
        auto const elapsed = 500 + GetTime() - gLastCastData.channelStartTimeMs;
        DEBUG_LOG("Channel update elapsed " << elapsed << " duration " << gCastData.channelDuration);

        if (elapsed > gCastData.channelDuration) {
            DEBUG_LOG("Channel done");
            gCastData.channeling = false;
            gCastData.channelCastCount = 0;

            DEBUG_LOG("Channel done queued:" << IsNonSwingSpellQueued());

            if (IsNonSwingSpellQueued()) {
                CastQueuedSpells();
            }
        }

        auto const spellChannelUpdateHandler = detour->GetTrampolineT<SpellChannelUpdateHandlerT>();
        return spellChannelUpdateHandler(channelUpdate, dataPtr);
    }
}