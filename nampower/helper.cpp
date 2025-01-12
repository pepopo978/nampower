//
// Created by pmacc on 9/21/2024.
//

#include "helper.hpp"
#include "offsets.hpp"
#include "main.hpp"

namespace Nampower {
    bool SpellIsOnGcd(const game::SpellRec *spell) {
        return spell->StartRecoveryCategory == 133;
    }

    bool SpellIsChanneling(const game::SpellRec *spell) {
        return spell->AttributesEx & game::SPELL_ATTR_EX_IS_CHANNELED ||
               spell->AttributesEx & game::SPELL_ATTR_EX_IS_SELF_CHANNELED;
    }

    bool SpellIsTargeting(const game::SpellRec *spell) {
        return spell->Targets == game::SpellTarget::TARGET_LOCATION_UNIT_POSITION;
    }

    bool SpellIsOnSwing(const game::SpellRec *spell) {
        return spell->Attributes & game::SPELL_ATTR_ON_NEXT_SWING_1;
    }

    bool SpellIsTradeskillOrEnchant(const game::SpellRec *spell) {
        return (
                spell->Attributes & game::SpellAttributes::SPELL_ATTR_TRADESPELL ||
                spell->Effect[0] == game::SpellEffects::SPELL_EFFECT_TRADE_SKILL ||
                spell->Effect[0] == game::SpellEffects::SPELL_EFFECT_ENCHANT_ITEM ||
                spell->Effect[0] == game::SpellEffects::SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY ||
                spell->Effect[0] == game::SpellEffects::SPELL_EFFECT_CREATE_ITEM ||
                spell->Effect[0] == game::SpellEffects::SPELL_EFFECT_OPEN_LOCK ||
                spell->Effect[0] == game::SpellEffects::SPELL_EFFECT_OPEN_LOCK_ITEM);
    }

    // if the spell is off cooldown, this will return the gcd, otherwise the cooldown
    uint32_t GetGcdOrCooldownForSpell(uint32_t spellId) {
        uint32_t duration;
        uint64_t startTime;
        uint32_t enable;

        auto const getSpellCooldown = reinterpret_cast<Spell_C_GetSpellCooldownT>(Offsets::Spell_C_GetSpellCooldown);
        getSpellCooldown(spellId, 0, &duration, &startTime, &enable);

        return duration;
    }

    uint32_t GetRemainingGcdOrCooldownForSpell(uint32_t spellId) {
        uint32_t duration;
        uint64_t startTime;
        uint32_t enable;

        auto const getSpellCooldown = reinterpret_cast<Spell_C_GetSpellCooldownT>(Offsets::Spell_C_GetSpellCooldown);
        getSpellCooldown(spellId, 0, &duration, &startTime, &enable);
        startTime = startTime & 0XFFFFFFFF; // only look at same bits that lua does

        if (startTime != 0) {
            auto currentLuaTime = GetWowTimeMs() & 0XFFFFFFFF;
            auto remaining = (startTime + duration) - currentLuaTime;
            return uint32_t(remaining);
        }

        return 0;
    }

    uint32_t GetRemainingCooldownForSpell(uint32_t spellId) {
        uint32_t duration;
        uint64_t startTime;
        uint32_t enable;

        auto const getSpellCooldown = reinterpret_cast<Spell_C_GetSpellCooldownT>(Offsets::Spell_C_GetSpellCooldown);
        getSpellCooldown(spellId, 0, &duration, &startTime, &enable);

        startTime = startTime & 0XFFFFFFFF; // only look at same bits that lua does

        // ignore gcd cooldown by looking for duration > 1.5
        if (startTime != 0 && duration > 1.5) {
            auto currentLuaTime = GetWowTimeMs() & 0XFFFFFFFF;
            auto remaining = (startTime + duration) - currentLuaTime;
            return uint32_t(remaining);
        }

        return 0;
    }

    bool IsSpellOnCooldown(uint32_t spellId) {
        uint32_t duration;
        uint64_t startTime;
        uint32_t enable;

        auto const getSpellCooldown = reinterpret_cast<Spell_C_GetSpellCooldownT>(Offsets::Spell_C_GetSpellCooldown);
        getSpellCooldown(spellId, 0, &duration, &startTime, &enable);

        return startTime != 0 && duration > 1.5;
    }

    char *ConvertGuidToString(uint64_t guid){
        char *guidStr = new char[21]; // 2 for 0x prefix, 18 for the number, and 1 for '\0'
        std::snprintf(guidStr, 21, "0x%016llX", static_cast<unsigned long long>(guid));
        return guidStr;
    }
}