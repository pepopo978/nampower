//
// Created by pmacc on 9/21/2024.
//

#include "helper.hpp"

namespace Nampower {
    bool SpellIsOnGCD(const game::SpellRec *spell) {
        return spell->StartRecoveryCategory == 133;
    }

    bool SpellIsChanneling(const game::SpellRec *spell) {
        return spell->AttributesEx & game::SPELL_ATTR_EX_IS_CHANNELED ||
               spell->AttributesEx & game::SPELL_ATTR_EX_IS_SELF_CHANNELED;
    }

    bool SpellIsOnSwing(const game::SpellRec *spell) {
        return spell->Attributes & game::SPELL_ATTR_ON_NEXT_SWING_1;
    }
}