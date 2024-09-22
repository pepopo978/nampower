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

#pragma once

#include <cstdint>

enum class Offsets : std::uint32_t {
    GetObjectPtr = 0x464870,
    GetActivePlayer = 0x468550,
    GetDurationObject = 0x00C0D828,
    CastSpell = 0x6E4B60,
    CancelSpell = 0x6E4940,
    SpellDelayed = 0x6E74F0,
    SignalEvent = 0x703E50,
    SignalEventParam = 0x703F50,
    GetCastingTimeIndex = 0x2D,
    Language = 0xC0E080,
    SpellDb = 0xC0D788,
    CursorMode = 0xBE2C4C,
    SpellIsTargeting = 0xCECAC0,

    CastingSpellId = 0xCECA88,
    ISceneEndPtr = 0x5A17A0,
    SpellGo = 0x006E7A70,
    Spell_C_SpellFailed = 0x006E1A00,
    Spell_C_CastSpellByID = 0x6E5A90,
    Spell_C_GetAutoRepeatingSpell = 0x006E9FD0,
    SpellStartHandler = 0x006E7640,
    SpellChannelStartHandler = 0x006E7550,
    SpellChannelUpdateHandler = 0x006E75F0,
    SpellFailedHandler = 0x006E8D80,
    CastResultHandler = 0x006E7330,
    CooldownEvent = 0x006E9670,
    SendCast = 0x6E54F0,
    CreateCastbar = 0x6E7A53,
};
