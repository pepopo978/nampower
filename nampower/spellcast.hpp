//
// Created by pmacc on 9/21/2024.
//

#pragma once

#include "game.hpp"
#include <Windows.h>

namespace Nampower {
    void BeginCast(DWORD currentTime, std::uint32_t castTime, const game::SpellRec *spell);


}