//
// Created by pmacc on 9/21/2024.
//

#pragma once

#include "main.hpp"

namespace Nampower {
    int SpellChannelStartHandlerHook(hadesmem::PatchDetourBase *detour, int channelStart, game::CDataStore *dataPtr);

    int SpellChannelUpdateHandlerHook(hadesmem::PatchDetourBase *detour, int channelUpdate, game::CDataStore *dataPtr);
}