//
// Created by pmacc on 1/8/2025.
//

#include "scripts.hpp"
#include "offsets.hpp"

namespace Nampower {
    auto const lua_error = reinterpret_cast<lua_errorT>(Offsets::lua_error);

    auto const lua_isstring = reinterpret_cast<lua_isstringT>(Offsets::lua_isstring);
    auto const lua_isnumber = reinterpret_cast<lua_isnumberT>(Offsets::lua_isnumber);

    auto const lua_tostring = reinterpret_cast<lua_tostringT>(Offsets::lua_tostring);
    auto const lua_tonumber = reinterpret_cast<lua_tonumberT>(Offsets::lua_tonumber);

    auto const lua_pushnumber = reinterpret_cast<lua_pushnumberT>(Offsets::lua_pushnumber);
    auto const lua_pushstring = reinterpret_cast<lua_pushstringT>(Offsets::lua_pushstring);

    bool gScriptQueued;
    int gScriptPriority = 1;
    char *queuedScript;

    uint32_t GetSpellSlotAndTypeForName(const char *spellName, uint32_t *spellType) {
        auto const getSpellSlotAndType = reinterpret_cast<GetSpellSlotAndTypeT>(Offsets::GetSpellSlotAndType);
        return getSpellSlotAndType(spellName, spellType);
    }


    uint32_t GetSpellIdFromSpellName(const char *spellName) {
        auto const GetSpellSlotAndBookTypeFromSpellName = reinterpret_cast<GetSpellSlotAndBookTypeFromSpellNameT>(Offsets::GetSpellIdFromSpellName);
        uint32_t bookType;
        uint32_t spellSlot = GetSpellSlotAndBookTypeFromSpellName(spellName, &bookType);

        uint32_t spellId = 0;
        if (spellSlot < 1024) {
            if (bookType == 0) {
                spellId = *reinterpret_cast<uint32_t *>(uint32_t(Offsets::CGSpellBook_mKnownSpells) +
                                                        spellSlot * 4);
            } else {
                spellId = *reinterpret_cast<uint32_t *>(uint32_t(Offsets::CGSpellBook_mKnownPetSpells) +
                                                        spellSlot * 4);
            }
        }

        return spellId;
    }

    uint32_t Script_CastSpellByNameNoQueue(hadesmem::PatchDetourBase *detour, uintptr_t *luaState) {
        DEBUG_LOG("Casting next spell without queuing");
        // turn on forceQueue and then call regular CastSpellByName
        gNoQueueCast = true;
        auto const Script_CastSpellByName = reinterpret_cast<LuaScriptT>(Offsets::Script_CastSpellByName);
        auto result = Script_CastSpellByName(luaState);
        gNoQueueCast = false;

        return result;
    }

    uint32_t Script_QueueSpellByName(hadesmem::PatchDetourBase *detour, uintptr_t *luaState) {
        DEBUG_LOG("Force queuing next cast spell");
        // turn on forceQueue and then call regular CastSpellByName
        gForceQueueCast = true;
        auto const Script_CastSpellByName = reinterpret_cast<LuaScriptT>(Offsets::Script_CastSpellByName);
        auto result = Script_CastSpellByName(luaState);
        gForceQueueCast = false;

        return result;
    }

    uint32_t Script_SpellStopCastingHook(hadesmem::PatchDetourBase *detour, uintptr_t *luaState) {
        DEBUG_LOG("SpellStopCasting called");

        ClearQueuedSpells();
        ResetCastFlags();
        ResetChannelingFlags();

        auto const spellStopCasting = detour->GetTrampolineT<LuaScriptT>();
        return spellStopCasting(luaState);
    }

    uint32_t Script_IsSpellInRange(hadesmem::PatchDetourBase *detour, uintptr_t *luaState) {
        auto param1IsString = lua_isstring(luaState, 1);
        auto param1IsNumber = lua_isnumber(luaState, 1);
        if (param1IsString || param1IsNumber) {
            uint32_t spellId = 0;

            if (param1IsNumber) {
                spellId = uint32_t(lua_tonumber(luaState, 1));

                if (spellId == 0) {
                    lua_error(luaState, "Unable to parse spell id");
                    return 0;
                }
            } else {
                auto const spellName = lua_tostring(luaState, 1);

                spellId = GetSpellIdFromSpellName(spellName);
                if (spellId == 0) {
                    lua_error(luaState,
                              "Unable to determine spell id from spell name, possibly because it isn't in your spell book.  Try IsSpellInRange(SPELL_ID) instead");
                    return 0;
                }
            }

            auto spell = game::GetSpellInfo(spellId);
            if (spell) {
                std::set<uint32_t> validTargetTypes = {5, 6, 21, 25};
                if (sizeof spell->EffectImplicitTargetA == 0 ||
                    validTargetTypes.count(spell->EffectImplicitTargetA[0]) == 0) {
                    lua_pushnumber(luaState, -1.0);
                    return 1;
                }

                char *target;

                if (lua_isstring(luaState, 2)) {
                    target = lua_tostring(luaState, 2);
                } else {
                    char defaultTarget[] = "target";
                    target = defaultTarget;
                }

                uint64_t targetGUID;
                if (strncmp(target, "0x", 2) == 0 || strncmp(target, "0X", 2) == 0) {
                    // already a guid
                    targetGUID = std::stoull(target, nullptr, 16);
                } else {
                    auto const getGUIDFromName = reinterpret_cast<GetGUIDFromNameT>(Offsets::GetGUIDFromName);
                    targetGUID = getGUIDFromName(target);
                }

                auto playerUnit = game::GetObjectPtr(game::ClntObjMgrGetActivePlayerGuid());

                auto const RangeCheckSelected = reinterpret_cast<RangeCheckSelectedT>(Offsets::RangeCheckSelected);
                auto const result = RangeCheckSelected(playerUnit, spell, targetGUID, '\0');

                if (result != 0) {
                    lua_pushnumber(luaState, 1.0);
                } else {
                    lua_pushnumber(luaState, 0);
                }
                return 1;
            } else {
                lua_error(luaState, "Spell not found");
            }
        } else {
            lua_error(luaState, "Usage: IsSpellInRange(spellName)");
        }

        return 0;
    }

    uint32_t Script_IsSpellUsable(hadesmem::PatchDetourBase *detour, uintptr_t *luaState) {
        auto param1IsString = lua_isstring(luaState, 1);
        auto param1IsNumber = lua_isnumber(luaState, 1);
        if (param1IsString || param1IsNumber) {
            uint32_t spellId = 0;

            if (param1IsNumber) {
                spellId = uint32_t(lua_tonumber(luaState, 1));

                if (spellId == 0) {
                    lua_error(luaState, "Unable to parse spell id");
                    return 0;
                }
            } else {
                auto const spellName = lua_tostring(luaState, 1);

                spellId = GetSpellIdFromSpellName(spellName);
                if (spellId == 0) {
                    lua_error(luaState,
                              "Unable to determine spell id from spell name, possibly because it isn't in your spell book.  Try IsSpellUsable(SPELL_ID) instead");
                    return 0;
                }
            }

            auto spell = game::GetSpellInfo(spellId);
            if (spell) {
                auto const IsSpellUsable = reinterpret_cast<Spell_C_IsSpellUsableT>(Offsets::Spell_C_IsSpellUsable);

                uint32_t outOfMana = 0;
                auto const result = IsSpellUsable(spell, &outOfMana) & 0xFF;

                if (result != 0) {
                    lua_pushnumber(luaState, 1.0);
                } else {
                    lua_pushnumber(luaState, 0);
                }

                if (outOfMana) {
                    lua_pushnumber(luaState, 1.0);
                } else {
                    lua_pushnumber(luaState, 0);
                }

                return 2;
            } else {
                lua_error(luaState, "Spell not found");
            }
        } else {
            lua_error(luaState, "Usage: IsSpellUsable(spellName)");
        }

        return 0;
    }

    uint32_t Script_GetCurrentCastingInfo(hadesmem::PatchDetourBase *detour, uintptr_t *luaState) {
        auto const castingSpellId = reinterpret_cast<uint32_t *>(Offsets::CastingSpellId);
        lua_pushnumber(luaState, *castingSpellId);

        auto const isCasting = gCastData.castEndMs > GetTime();
        auto const isChanneling = gCastData.channeling;

        auto const visualSpellId = reinterpret_cast<uint32_t *>(Offsets::VisualSpellId);
        lua_pushnumber(luaState, *visualSpellId);

        auto const autoRepeatingSpellId = reinterpret_cast<uint32_t *>(Offsets::AutoRepeatingSpellId);
        lua_pushnumber(luaState, *autoRepeatingSpellId);

        auto playerUnit = game::GetObjectPtr(game::ClntObjMgrGetActivePlayerGuid());
        if (isCasting) {
            lua_pushnumber(luaState, 1);
        } else {
            lua_pushnumber(luaState, 0);
        }

        if (isChanneling) {
            lua_pushnumber(luaState, 1);
        } else {
            lua_pushnumber(luaState, 0);
        }

        if (gCastData.pendingOnSwingCast) {
            lua_pushnumber(luaState, 1);
        } else {
            lua_pushnumber(luaState, 0);
        }

        auto const attackPtr = playerUnit + 0x312; // auto attacking
        if (attackPtr && *reinterpret_cast<uint32_t *>(attackPtr) > 0) {
            lua_pushnumber(luaState, 1);
        } else {
            lua_pushnumber(luaState, 0);
        }

        return 7;
    }

    uint32_t Script_GetSpellIdForName(hadesmem::PatchDetourBase *detour, uintptr_t *luaState) {
        if (lua_isstring(luaState, 1)) {
            auto const spellName = lua_tostring(luaState, 1);
            auto const spellId = GetSpellIdFromSpellName(spellName);
            lua_pushnumber(luaState, spellId);
            return 1;
        } else {
            lua_error(luaState, "Usage: GetSpellIdForName(spellName)");
        }

        return 0;
    }

    uint32_t Script_GetSpellNameAndRankForId(hadesmem::PatchDetourBase *detour, uintptr_t *luaState) {
        if (lua_isnumber(luaState, 1)) {
            auto const spellId = uint32_t(lua_tonumber(luaState, 1));
            auto const spell = game::GetSpellInfo(spellId);

            if (spell) {
                auto const language = *reinterpret_cast<std::uint32_t *>(Offsets::Language);
                lua_pushstring(luaState, (char *) spell->SpellName[language]);
                lua_pushstring(luaState, (char *) spell->Rank[language]);
                return 2;
            } else {
                lua_error(luaState, "Spell not found");
            }
        } else {
            lua_error(luaState, "Usage: GetSpellNameAndRankForId(spellId)");
        }

        return 0;
    }

    uint32_t Script_GetSpellSlotTypeIdForName(hadesmem::PatchDetourBase *detour, uintptr_t *luaState) {
        if (lua_isstring(luaState, 1)) {
            auto const spellName = lua_tostring(luaState, 1);
            uint32_t bookType;
            auto spellSlot = GetSpellSlotAndTypeForName(spellName, &bookType);

            // returns large number if spell not found
            if (spellSlot > 100000) {
                lua_pushnumber(luaState, 0);
                spellSlot = 0;
                bookType = 999;

                lua_pushnumber(luaState, spellSlot);
            } else {
                lua_pushnumber(luaState, spellSlot + 1); // lua is 1 indexed
            }

            if (bookType == 0) {
                char spell[] = "spell";
                lua_pushstring(luaState, spell);
            } else if (bookType == 1) {
                char pet[] = "pet";
                lua_pushstring(luaState, pet);
            } else {
                char unknown[] = "unknown";
                lua_pushstring(luaState, unknown);
            }

            if (spellSlot > 0 && spellSlot < 1024) {
                uint32_t spellId = 0;
                if (bookType == 0) {
                    spellId = *reinterpret_cast<uint32_t *>(uint32_t(Offsets::CGSpellBook_mKnownSpells) +
                                                            spellSlot * 4);
                } else {
                    spellId = *reinterpret_cast<uint32_t *>(uint32_t(Offsets::CGSpellBook_mKnownPetSpells) +
                                                            spellSlot * 4);
                }
                lua_pushnumber(luaState, spellId);
            } else {
                lua_pushnumber(luaState, 0);
            }

            return 3;
        } else {
            lua_error(luaState, "Usage: GetSpellSlotTypeIdForName(spellName)");
        }

        return 0;
    }

    uint32_t Script_ChannelStopCastingNextTick(hadesmem::PatchDetourBase *detour, uintptr_t *luaState) {
        DEBUG_LOG("ChannelStopCastingNextTick called");
        if (gCastData.channeling) {
            gCastData.cancelChannelNextTick = true;
        }

        return 0;
    }

    uint32_t Script_GetNampowerVersion(hadesmem::PatchDetourBase *detour, uintptr_t *luaState) {
        lua_pushnumber(luaState, MAJOR_VERSION);
        lua_pushnumber(luaState, MINOR_VERSION);
        lua_pushnumber(luaState, PATCH_VERSION);

        return 3;
    }

    uint32_t Script_GetItemLevel(hadesmem::PatchDetourBase *detour, uintptr_t *luaState) {
        if (lua_isnumber(luaState, 1)) {
            auto const itemId = static_cast<uint32_t>(lua_tonumber(luaState, 1));

            // Pointer to ItemDBCache
            void *itemDbCache = reinterpret_cast<void *>(Offsets::ItemDBCache);

            // Parameters for the DBCache<>::GetRecord function
            int **param2 = nullptr;
            int *param3 = nullptr;
            int *param4 = nullptr;
            char param5 = 0;

            // Call the DBCache<>::GetRecord function
            auto getRecord = reinterpret_cast<uintptr_t *(__thiscall *)(void *, uint32_t, int **, int *, int *, char)>(
                    Offsets::DBCacheGetRecord
            );
            uintptr_t *itemObject = getRecord(itemDbCache, itemId, param2, param3, param4, param5);

            uint32_t itemLevel = *reinterpret_cast<uint32_t *>(itemObject + 14);
            lua_pushnumber(luaState, itemLevel);
            return 1;
        } else {
            lua_error(luaState, "Usage: GetItemLevel(itemId)");
        }

        return 0;
    }

    bool Script_QueueScript(hadesmem::PatchDetourBase *detour, uintptr_t *luaState) {
        DEBUG_LOG("Trying to queue script");

        auto const currentTime = GetTime();
        auto effectiveCastEndMs = EffectiveCastEndMs();
        auto remainingEffectiveCastTime = (effectiveCastEndMs > currentTime) ? effectiveCastEndMs - currentTime : 0;
        auto remainingGcd = (gCastData.gcdEndMs > currentTime) ? gCastData.gcdEndMs - currentTime : 0;
        auto inSpellQueueWindow = InSpellQueueWindow(remainingEffectiveCastTime, remainingGcd, false);

        if (inSpellQueueWindow) {
            // check if valid string
            if (lua_isstring(luaState, 1)) {
                auto script = lua_tostring(luaState, 1);

                if (script != nullptr && strlen(script) > 0) {
                    // save the script to be run later
                    queuedScript = script;
                    gScriptQueued = true;

                    // check if priority is set
                    if (lua_isnumber(luaState, 2)) {
                        gScriptPriority = (int) lua_tonumber(luaState, 2);

                        DEBUG_LOG("Queuing script priority " << gScriptPriority << ": " << script);
                    } else {
                        DEBUG_LOG("Queuing script: " << script);
                    }
                }
            } else {
                DEBUG_LOG("Invalid script");
                lua_error(luaState, "Usage: QueueScript(\"script\", (optional)priority)");
            }
        } else {
            // just call regular runscript
            auto const runScript = reinterpret_cast<LuaScriptT >(Offsets::Script_RunScript);
            return runScript(luaState);
        }

        return false;
    }

    bool RunQueuedScript(int priority) {
        if (gScriptQueued && gScriptPriority == priority) {
            auto currentTime = GetTime();

            auto effectiveCastEndMs = EffectiveCastEndMs();
            // get max of cooldown and gcd
            auto delay = effectiveCastEndMs > gCastData.gcdEndMs ? effectiveCastEndMs : gCastData.gcdEndMs;

            if (delay <= currentTime) {
                DEBUG_LOG("Running queued script priority " << gScriptPriority << ": " << queuedScript);
                LuaCall(queuedScript);
                gScriptQueued = false;
                gScriptPriority = 1;
                return true;
            }
        }

        return false;
    }
}