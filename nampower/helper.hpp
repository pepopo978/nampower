//
// Created by pmacc on 9/21/2024.
//

#pragma once

#include "game.hpp"

namespace Nampower {

    bool SpellIsOnGcd(const game::SpellRec *spell);

    bool SpellIsChanneling(const game::SpellRec *spell);

    bool SpellIsTargeting(const game::SpellRec *spell);

    bool SpellIsOnSwing(const game::SpellRec *spell);

    bool SpellIsTradeskillOrEnchant(const game::SpellRec *spell);
}
