/*
    Copyright (c) 2017-2023, namreeb (legal@namreeb.org)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
    ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    The views and conclusions contained in the software and documentation are those
    of the authors and should not be interpreted as representing official policies,
    either expressed or implied, of the FreeBSD Project.
*/

#include "game.hpp"
#include "offsets.hpp"

#include <hadesmem/detail/alias_cast.hpp>

#include <cstdint>

namespace game {
    uintptr_t *GetObjectPtr(std::uint64_t guid) {
        uintptr_t *(__stdcall *getObjectPtr)(std::uint64_t) = hadesmem::detail::AliasCast<decltype(getObjectPtr)>(
                Offsets::GetObjectPtr);

        return getObjectPtr(guid);
    }

    uintptr_t *ClntObjMgrObjectPtr(TypeMask typeMask, std::uint64_t guid) {
        using ClntObjMgrObjectPtrT = uintptr_t* (__fastcall *)(TypeMask typeMask, const char *debugMessage, unsigned __int64 guid, int debugCode);
        auto const clntObjMgrObjectPtr = reinterpret_cast<ClntObjMgrObjectPtrT>(Offsets::ClntObjMgrObjectPtr);

        return clntObjMgrObjectPtr(typeMask, nullptr, guid, 0);
    }


    std::uint32_t GetCastTime(void *unit, uint32_t spellId) {
        auto const vmt = *reinterpret_cast<std::uint8_t **>(unit);
        int
        (__thiscall *getSpellCastingTime)(void *, uint32_t) = *reinterpret_cast<decltype(&getSpellCastingTime)>(vmt +
                                                                                                                4 *
                                                                                                                static_cast<std::uint32_t>(Offsets::GetCastingTimeIndex));

        return getSpellCastingTime(unit, spellId);
    }

    CDuration *GetDurationObject(uint32_t durationIndex) {
        auto const durationListPtr = *reinterpret_cast<std::uint32_t *>(Offsets::GetDurationObject);
        if (durationListPtr) {
            auto const durationObjectPtr = *reinterpret_cast<std::uint32_t *>(durationListPtr + durationIndex*4);
            if (durationObjectPtr) {
                return reinterpret_cast<CDuration *>(durationObjectPtr);
            }
        }
        return nullptr;
    }

    int GetSpellDuration(const SpellRec *spellRec, bool ignoreModifiers) {
        using Spell_C_GetDurationT = int (__fastcall *)(const SpellRec *spellRec, int unknownFlag, char ignoreModifiers);

        auto const getDuration = reinterpret_cast<Spell_C_GetDurationT>(Offsets::Spell_C_GetDuration);
        if (ignoreModifiers){
            return getDuration(spellRec, 1, 1);
        } else {
            return getDuration(spellRec, 1, 0);
        }
    }

    int GetSpellModifier(const SpellRec *spellRec, SpellModOp spellMod) {
        using Spell_C_GetSpellModifiersT = void (__fastcall *)(const SpellRec *spellRec, int *returnVal, SpellModOp modOp);

        auto const getModifiers = reinterpret_cast<Spell_C_GetSpellModifiersT>(Offsets::Spell_C_GetSpellModifiers);
        auto modificationPercentage = 0;

        getModifiers(spellRec, &modificationPercentage, spellMod);

        return modificationPercentage;
    }

    const SpellRec *GetSpellInfo(uint32_t spellId) {
        auto const spellDb = reinterpret_cast<WowClientDB<SpellRec> *>(Offsets::SpellDb);

        if (spellId > spellDb->m_maxId)
            return nullptr;

        return spellDb->m_recordsById[spellId];
    }

    uint32_t GetItemId(CGItem_C *item) {
        uintptr_t *itemInfo = item->m_itemInfo;

        return *reinterpret_cast<uint32_t *>(itemInfo + 3); // item id offset
    }

    const char *GetSpellName(uint32_t spellId) {
        auto const spell = GetSpellInfo(spellId);

        if (!spell || spell->AttributesEx3 & SPELL_ATTR_EX3_NO_CASTING_BAR_TEXT)
            return "";

        auto const language = *reinterpret_cast<std::uint32_t *>(Offsets::Language);

        return spell->SpellName[language];
    }

    std::uint64_t ClntObjMgrGetActivePlayerGuid() {
        auto const getActivePlayer = hadesmem::detail::AliasCast<decltype(&ClntObjMgrGetActivePlayerGuid)>(
                Offsets::GetActivePlayer);

        return getActivePlayer();
    }

    std::uint64_t GetCurrentTargetGuid() {
        return *reinterpret_cast<uint64_t *>(Offsets::LockedTargetGuid);
    }

    uint64_t UnitGetGuid(uintptr_t *unit) {
        if (!unit) {
            return 0;
        }

        uint64_t guid = *reinterpret_cast<uint64_t *>(unit + 12);
        return guid;
    }

    uint64_t UnitGetTargetGuid(uintptr_t *unit) {
        if (!unit) {
            return 0;
        }

        auto *unitFields = *reinterpret_cast<UnitFields **>(unit + 68);

        if (unitFields == nullptr) {
            return 0;
        }

        return unitFields->target;
    }
}