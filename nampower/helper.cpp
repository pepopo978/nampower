//
// Created by pmacc on 9/21/2024.
//

#include "helper.hpp"

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
        return (spell->Effect[0] == game::SpellEffects::SPELL_EFFECT_TRADE_SKILL ||
                spell->Effect[0] == game::SpellEffects::SPELL_EFFECT_ENCHANT_ITEM ||
                spell->Effect[0] == game::SpellEffects::SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY ||
                spell->Effect[0] == game::SpellEffects::SPELL_EFFECT_CREATE_ITEM);
    }
}