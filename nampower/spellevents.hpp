//
// Created by pmacc on 9/21/2024.
//

#pragma once

#include "main.hpp"

namespace Nampower {
    void SignalEventHook(hadesmem::PatchDetourBase *detour, game::Events eventId);

    int SpellDelayedHook(hadesmem::PatchDetourBase *detour, uint32_t *opCode, CDataStore *packet);

    uint32_t Script_SpellTargetUnitHook(hadesmem::PatchDetourBase *detour, uintptr_t *luaState);

    void Spell_C_SpellFailedHook(hadesmem::PatchDetourBase *detour, uint32_t spellId,
                                 game::SpellCastResult spellResult, int unk1, int unk2, char unk3);

    int CastResultHandlerHook(hadesmem::PatchDetourBase *detour, uint32_t *opCode, CDataStore *packet);

    int SpellFailedHandlerHook(hadesmem::PatchDetourBase *detour, uint32_t *opCode, CDataStore *packet);

    int SpellStartHandlerHook(hadesmem::PatchDetourBase *detour, uint32_t unk, uint32_t opCode, uint32_t unk2,
                              CDataStore *packet);

    void Spell_C_CooldownEventTriggeredHook(hadesmem::PatchDetourBase *detour, uint32_t spellId, uint64_t *targetGUID,
                                            int param_3, int clearCooldowns);

    int SpellCooldownHandlerHook(hadesmem::PatchDetourBase *detour, uint32_t *opCode, CDataStore *packet);

    int PeriodicAuraLogHandlerHook(hadesmem::PatchDetourBase *detour, uint32_t unk, uint32_t opCode, uint32_t unk2,
                                   CDataStore *packet);

    int SpellNonMeleeDmgLogHandlerHook(hadesmem::PatchDetourBase *detour, uint32_t unk, uint32_t opCode, uint32_t unk2,
                                       CDataStore *packet);

    int PlaySpellVisualHandlerHook(hadesmem::PatchDetourBase *detour, uint32_t *opCode, CDataStore *packet);
}