//
// Created by pmacc on 9/21/2024.
//

#pragma once

#include "main.hpp"

namespace Nampower {
    int SpellChannelStartHandlerHook(hadesmem::PatchDetourBase *detour, uint32_t currentTime, CDataStore *packet);

    int SpellChannelUpdateHandlerHook(hadesmem::PatchDetourBase *detour, uint32_t currentTime, CDataStore *packet);
}