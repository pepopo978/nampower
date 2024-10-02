//
// Created by pmacc on 9/21/2024.
//

#pragma once

#include "main.hpp"

namespace Nampower {

    void SignalEventHook(hadesmem::PatchDetourBase *detour, game::Events eventId);

    int SpellDelayedHook(hadesmem::PatchDetourBase *detour,  uint32_t* opCode, CDataStore *packet);

    bool SpellTargetUnitHook(hadesmem::PatchDetourBase *detour, uintptr_t *unitStr);

    void Spell_C_SpellFailedHook(hadesmem::PatchDetourBase *detour, uint32_t spellId,
                                 game::SpellCastResult spellResult, int unk1, int unk2, char unk3);

    int CastResultHandlerHook(hadesmem::PatchDetourBase *detour,  uint32_t* opCode, CDataStore *packet);

    int SpellFailedHandlerHook(hadesmem::PatchDetourBase *detour, uint32_t* opCode, CDataStore *packet);

    int SpellStartHandlerHook(hadesmem::PatchDetourBase *detour,uint32_t unk, uint32_t opCode, uint32_t unk2, CDataStore *packet);

    void Spell_C_CooldownEventTriggeredHook(hadesmem::PatchDetourBase *detour, uint32_t spellId,int param_2,int param_3,int clearCooldowns);
}