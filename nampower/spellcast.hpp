//
// Created by pmacc on 9/21/2024.
//

#pragma once

#include "game.hpp"
#include <Windows.h>
#include "main.hpp"

namespace Nampower {
    void BeginCast(uint32_t currentTime, std::uint32_t castTime, const game::SpellRec *spell);

    void CastQueuedNonGcdSpell();

    void CastQueuedNormalSpell();

    void CastQueuedSpells();

    bool Spell_C_CastSpellHook(hadesmem::PatchDetourBase *detour, uint32_t *playerUnit, uint32_t spellId, void *item, std::uint64_t guid);

    void
    CancelSpellHook(hadesmem::PatchDetourBase *detour, bool failed, bool notifyServer, game::SpellCastResult reason);

    void SpellGoHook(hadesmem::PatchDetourBase *detour, uint64_t *casterGUID, uint64_t *targetGUID, uint32_t spellId,
                     CDataStore *spellData);

    bool Spell_C_TargetSpellHook(hadesmem::PatchDetourBase *detour,
                                 uint32_t *player,
                                 uint32_t *spellId,
                                 uint32_t unk3,
                                 float unk4);

    void SendCastHook(hadesmem::PatchDetourBase *detour, game::SpellCast *cast, char unk);
}