//
// Created by pmacc on 1/8/2025.
//

#pragma once

#include <Windows.h>
#include "main.hpp"

namespace Nampower {
    uint32_t Script_CastSpellByNameNoQueue(hadesmem::PatchDetourBase *detour, uintptr_t *luaState);

    uint32_t Script_QueueSpellByName(hadesmem::PatchDetourBase *detour, uintptr_t *luaState);

    uint32_t Script_IsSpellInRange(hadesmem::PatchDetourBase *detour, uintptr_t *luaState);

    uint32_t Script_IsSpellUsable(hadesmem::PatchDetourBase *detour, uintptr_t *luaState);

    uint32_t Script_SpellStopCastingHook(hadesmem::PatchDetourBase *detour, uintptr_t *luaState);

    uint32_t Script_GetCurrentCastingInfo(hadesmem::PatchDetourBase *detour, uintptr_t *luaState);

    uint32_t Script_GetSpellIdForName(hadesmem::PatchDetourBase *detour, uintptr_t *luaState);

    uint32_t Script_GetSpellNameAndRankForId(hadesmem::PatchDetourBase *detour, uintptr_t *luaState);

    uint32_t Script_GetSpellSlotTypeIdForName(hadesmem::PatchDetourBase *detour, uintptr_t *luaState);

    uint32_t Script_ChannelStopCastingNextTick(hadesmem::PatchDetourBase *detour, uintptr_t *luaState);

    bool Script_QueueScript(hadesmem::PatchDetourBase *detour, uintptr_t *luaState);

    bool RunQueuedScript(int priority);
}