//
// Created by pmacc on 9/21/2024.
//

#pragma once

#include "main.hpp"

namespace Nampower {

    void SignalEventHook(hadesmem::PatchDetourBase *detour, game::Events eventId);

    int SpellDelayedHook(hadesmem::PatchDetourBase *detour, int opCode, game::CDataStore *packet);

    bool SpellTargetUnitHook(hadesmem::PatchDetourBase *detour, uintptr_t *unitStr);

    void Spell_C_SpellFailedHook(hadesmem::PatchDetourBase *detour, uint32_t spellId,
                                 game::SpellCastResult spellResult, int unk1, int unk2, char unk3);

}