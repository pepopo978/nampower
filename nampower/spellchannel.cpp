//
// Created by pmacc on 9/21/2024.
//

#include "spellchannel.hpp"
#include "spellcast.hpp"

namespace Nampower {
    int SpellChannelStartHandlerHook(hadesmem::PatchDetourBase *detour, int channelStart, game::CDataStore *dataPtr) {
        gLastChannelStartTime = GetTime();
        DEBUG_LOG("Channel start " << channelStart);
        if (channelStart) {
            gChanneling = true;
            gChannelCastCount = 0;
        }

        auto const spellChannelStartHandler = detour->GetTrampolineT<SpellChannelStartHandlerT>();
        return spellChannelStartHandler(channelStart, dataPtr);
    }

    int SpellChannelUpdateHandlerHook(hadesmem::PatchDetourBase *detour, int channelUpdate, game::CDataStore *dataPtr) {
        auto const elapsed = 500 + GetTime() - gLastChannelStartTime;
        DEBUG_LOG("Channel update elapsed " << elapsed << " duration " << gChannelDuration);

        if (elapsed > gChannelDuration) {
            DEBUG_LOG("Channel done");
            gChanneling = false;
            gChannelCastCount = 0;

            if (gChannelQueued) {
                gChannelQueued = false;
                DEBUG_LOG("Channeling ended, triggering queued cast");
                CastSpellHook(lastDetour, lastUnit, lastSpellId, lastItem, lastGuid);
            }
        }

        auto const spellChannelUpdateHandler = detour->GetTrampolineT<SpellChannelUpdateHandlerT>();
        return spellChannelUpdateHandler(channelUpdate, dataPtr);
    }
}