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
    StartSpellVisualCheck = 0X006E79B5,

    GetObjectPtr = 0x464870,
    GetActivePlayer = 0x468550,
    GetDurationObject = 0x00C0D828,
    GetCastingTimeIndex = 0x2D,
    Language = 0xC0E080,
    SpellDb = 0xC0D788,
    CursorMode = 0xBE2C4C,
    SpellIsTargeting = 0xCECAC0,
    CastingSpellId = 0xCECA88,
    VisualSpellId = 0X00CEAC58,
    CastResultCDataStore = 0x0019FDDC,

    CancelSpell = 0x6E4940,
    CancelAutoRepeatSpell = 0X006EA080,
    SpellDelayed = 0x6E74F0,
    SignalEvent = 0x703E50,
    SignalEventParam = 0x703F50,
    ISceneEndPtr = 0x5A17A0,
    SpellStart = 0X006E7700,
    SpellGo = 0x006E7A70,
    CooldownEvent = 0x006E9670,
    SendCast = 0x6E54F0,
    CreateCastbar = 0x6E7A53,
    CheckAndReportSpellInhibitFlags = 0x006094f0,

    SpellVisualsHandleCastStart = 0X006EC220,

    SpellChannelStartHandler = 0x006E7550,
    SpellChannelUpdateHandler = 0x006E75F0,
    SpellFailedHandler = 0x006E8D80,
    SpellFailedOtherHandler = 0X006E8E40,
    CastResultHandler = 0x006E7330,
    SpellStartHandler = 0x006E7640,
    SpellCooldownHandler = 0X006E9460,

    Spell_C_CastSpell = 0x6E4B60,
    Spell_C_CooldownEventTriggered = 0X006E3050,
    Spell_C_SpellFailed = 0x006E1A00,
    Spell_C_CastSpellByID = 0x6E5A90,
    Spell_C_GetAutoRepeatingSpell = 0x006E9FD0,
    Spell_C_HandleSpriteClick = 0x006E5B10,
    Spell_C_TargetSpell = 0x006E5250,

    CVarLookup = 0x0063DEC0,
    RegisterCVar = 0X0063DB90,

    ClientServicesSendMessagePtr = 0X00C28128,

    Script_SpellTargetUnit = 0x006E6D90,
    Script_GetGUIDFromName = 0X00515970,
    Script_SetCVar = 0x00488C10,

    lua_isstring = 0x006F3510,
    lua_tostring = 0x006F3690,
    lua_call = 0x00704CD0,
    lua_pcall = 0x006F41A0,

    SpellVisualsInitialize = 0x006ec0e0,
};
