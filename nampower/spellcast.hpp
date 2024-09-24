//
// Created by pmacc on 9/21/2024.
//

#pragma once

#include "game.hpp"
#include <Windows.h>
#include "main.hpp"

namespace Nampower {
    void BeginCast(DWORD currentTime, std::uint32_t castTime, const game::SpellRec *spell);

    bool CastSpellHook(hadesmem::PatchDetourBase *detour, void *unit, uint32_t spellId, void *item, std::uint64_t guid);

    int
    CancelSpellHook(hadesmem::PatchDetourBase *detour, bool failed, bool notifyServer, game::SpellCastResult reason);

    void SpellGoHook(hadesmem::PatchDetourBase *detour, uint64_t *casterGUID, uint64_t *targetGUID, uint32_t spellId,
                     game::CDataStore *spellData);

    bool Spell_C_TargetSpellHook(hadesmem::PatchDetourBase *detour,
                                 uint32_t *player,
                                 uint32_t *spellId,
                                 uint32_t unk3,
                                 float unk4);

    void SendCastHook(hadesmem::PatchDetourBase *detour, game::SpellCast *cast);
}