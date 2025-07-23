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

namespace game {
#pragma pack(push, 1)
    struct SpellRec {
        unsigned int Id;
        unsigned int School;
        unsigned int Category;
        unsigned int castUI;
        unsigned int Dispel;
        unsigned int Mechanic;
        unsigned int Attributes;
        unsigned int AttributesEx;
        unsigned int AttributesEx2;
        unsigned int AttributesEx3;
        unsigned int AttributesEx4;
        unsigned int Stances;
        unsigned int StancesNot;
        unsigned int Targets;
        unsigned int TargetCreatureType;
        unsigned int RequiresSpellFocus;
        unsigned int CasterAuraState;
        unsigned int TargetAuraState;
        unsigned int CastingTimeIndex;
        unsigned int RecoveryTime;
        unsigned int CategoryRecoveryTime;
        unsigned int InterruptFlags;
        unsigned int AuraInterruptFlags;
        unsigned int ChannelInterruptFlags;
        unsigned int procFlags;
        unsigned int procChance;
        unsigned int procCharges;
        unsigned int maxLevel;
        unsigned int baseLevel;
        unsigned int spellLevel;
        unsigned int DurationIndex;
        unsigned int powerType;
        unsigned int manaCost;
        unsigned int manaCostPerlevel;
        unsigned int manaPerSecond;
        unsigned int manaPerSecondPerLevel;
        unsigned int rangeIndex;
        float speed;
        unsigned int modalNextSpell;
        unsigned int StackAmount;
        unsigned int Totem[2];
        int Reagent[8];
        unsigned int ReagentCount[8];
        int EquippedItemClass;
        int EquippedItemSubClassMask;
        int EquippedItemInventoryTypeMask;
        unsigned int Effect[3];
        int EffectDieSides[3];
        unsigned int EffectBaseDice[3];
        float EffectDicePerLevel[3];
        float EffectRealPointsPerLevel[3];
        int EffectBasePoints[3];
        unsigned int EffectMechanic[3];
        unsigned int EffectImplicitTargetA[3];
        unsigned int EffectImplicitTargetB[3];
        unsigned int EffectRadiusIndex[3];
        unsigned int EffectApplyAuraName[3];
        unsigned int EffectAmplitude[3];
        float EffectMultipleValue[3];
        unsigned int EffectChainTarget[3];
        unsigned int EffectItemType[3];
        int EffectMiscValue[3];
        unsigned int EffectTriggerSpell[3];
        float EffectPointsPerComboPoint[3];
        unsigned int SpellVisual;
        unsigned int SpellVisual2;
        unsigned int SpellIconID;
        unsigned int activeIconID;
        unsigned int spellPriority;
        const char *SpellName[8];
        unsigned int SpellNameFlag;
        unsigned int Rank[8];
        unsigned int RankFlags;
        unsigned int Description[8];
        unsigned int DescriptionFlags;
        unsigned int ToolTip[8];
        unsigned int ToolTipFlags;
        unsigned int ManaCostPercentage;
        unsigned int StartRecoveryCategory;
        unsigned int StartRecoveryTime;
        unsigned int MaxTargetLevel;
        unsigned int SpellFamilyName;
        unsigned __int64 SpellFamilyFlags;
        unsigned int MaxAffectedTargets;
        unsigned int DmgClass;
        unsigned int PreventionType;
        unsigned int StanceBarOrder;
        float DmgMultiplier[3];
        unsigned int MinFactionId;
        unsigned int MinReputation;
        unsigned int RequiredAuraVision;
    };

    template<typename T>
    struct WowClientDB {
        T **m_recordsById;
        uint32_t m_maxID;
        int Unk3;
        int Unk4;
        int Unk5;
    };

    struct SpellCast {
        unsigned __int64 casterUnit;
        unsigned __int64 caster;
        int spellId;
        unsigned __int16 targets;
        unsigned __int16 unk;
        unsigned __int64 unitTarget;
        unsigned __int64 itemTarget;
        unsigned int Fields2[18];
        char targetString[128];
    };

    struct CSpriteClickEvent {
        unsigned __int64 objectGUID;
        unsigned int button;
        unsigned int time;
        unsigned __int64 *pos;
    };

    enum ItemStatsFlags {
        ITEM_FLAG_NO_PICKUP = 1,
        ITEM_FLAG_CONJURED = 2,
        ITEM_FLAG_HAS_LOOT = 4,
        ITEM_FLAG_EXOTIC = 8,
        ITEM_FLAG_NUM = 14,
        ITEM_FLAG_DEPRECATED = 16,
        ITEM_FLAG_OBSOLETE = 32,
        ITEM_FLAG_PLAYERCAST = 64,
        ITEM_FLAG_NO_EQUIPCOOLDOWN = 128,
        ITEM_FLAG_INTBONUSINSTEAD = 256,
        ITEM_FLAG_IS_WRAPPER = 512,
        ITEM_FLAG_USES_RESOURCES = 1024,
        ITEM_FLAG_MULTI_DROP = 2048,
        ITEM_FLAG_BRIEFSPELLEFFECTS = 4096,
        ITEM_FLAG_PETITION = 8192,
        MAX_ITEM_FLAG = 32768
    };

    struct CGItem_C {
        uint32_t m_unk;
        uint32_t m_flags;
        uintptr_t *m_itemInfo;
        uint32_t m_expirationTime;
        uint32_t m_enchantmentExpiration[5];
        uintptr_t *m_soundsRec;
    };

    struct __declspec(align(4)) ItemStats_C {
        int m_class;
        int m_subclass;
        char *m_displayName[4];
        int m_displayInfoID;
        int m_quality;
        ItemStatsFlags m_flags;
        int m_buyPrice;
        int m_sellPrice;
        int m_inventoryType;
        int m_allowableClass;
        int m_allowableRace;
        int m_itemLevel;
        int m_requiredLevel;
        int m_requiredSkill;
        int m_requiredSkillRank;
        int m_requiredSpell;
        int m_requiredHonorRank;
        int m_requiredCityRank;
        int m_requiredRep;
        int m_requiredRepRank;
        int m_maxCount;
        int m_stackable;
        int m_containerSlots;
        int m_bonusStat[10];
        int m_bonusAmount[10];
        int m_minDamage[5];
        float m_maxDamage[5];
        int m_damageType[5];
        int m_resistances[7];
        int m_delay;
        int m_ammoType;
        int m_rangedModRange;
        int m_spellID[5];
        int m_spellTrigger[5];
        int m_spellCharges[5];
        int m_spellCooldown[5];
        int m_spellCategory[5];
        int m_spellCategoryCooldown[5];
        int m_bonding;
        char *m_description;
        int m_pageText;
        int m_languageID;
        int m_pageMaterial;
        int m_startQuestID;
        int m_lockID;
        char *m_material;
        int m_sheatheType;
        int m_randomProperty;
        int m_block;
        int m_itemSet;
        int m_maxDurability;
        int m_area;
        int m_map;
        int m_duration;
        int m_bagFamily;
    };
#pragma pack(pop)

    enum SpellEffects {
        SPELL_EFFECT_NONE = 0,
        SPELL_EFFECT_INSTAKILL = 1,
        SPELL_EFFECT_SCHOOL_DAMAGE = 2,
        SPELL_EFFECT_DUMMY = 3,
        SPELL_EFFECT_PORTAL_TELEPORT = 4,
        SPELL_EFFECT_TELEPORT_UNITS = 5,
        SPELL_EFFECT_APPLY_AURA = 6,
        SPELL_EFFECT_ENVIRONMENTAL_DAMAGE = 7,
        SPELL_EFFECT_POWER_DRAIN = 8,
        SPELL_EFFECT_HEALTH_LEECH = 9,
        SPELL_EFFECT_HEAL = 10,
        SPELL_EFFECT_BIND = 11,
        SPELL_EFFECT_PORTAL = 12,
        SPELL_EFFECT_RITUAL_BASE = 13,
        SPELL_EFFECT_RITUAL_SPECIALIZE = 14,
        SPELL_EFFECT_RITUAL_ACTIVATE_PORTAL = 15,
        SPELL_EFFECT_QUEST_COMPLETE = 16,
        SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL = 17,
        SPELL_EFFECT_RESURRECT = 18,
        SPELL_EFFECT_ADD_EXTRA_ATTACKS = 19,
        SPELL_EFFECT_DODGE = 20,
        SPELL_EFFECT_EVADE = 21,
        SPELL_EFFECT_PARRY = 22,
        SPELL_EFFECT_BLOCK = 23,
        SPELL_EFFECT_CREATE_ITEM = 24,
        SPELL_EFFECT_WEAPON = 25,
        SPELL_EFFECT_DEFENSE = 26,
        SPELL_EFFECT_PERSISTENT_AREA_AURA = 27,
        SPELL_EFFECT_SUMMON = 28,
        SPELL_EFFECT_LEAP = 29,
        SPELL_EFFECT_ENERGIZE = 30,
        SPELL_EFFECT_WEAPON_PERCENT_DAMAGE = 31,
        SPELL_EFFECT_TRIGGER_MISSILE = 32,
        SPELL_EFFECT_OPEN_LOCK = 33,
        SPELL_EFFECT_SUMMON_CHANGE_ITEM = 34,
        SPELL_EFFECT_APPLY_AREA_AURA_PARTY = 35,
        SPELL_EFFECT_LEARN_SPELL = 36,
        SPELL_EFFECT_SPELL_DEFENSE = 37,
        SPELL_EFFECT_DISPEL = 38,
        SPELL_EFFECT_LANGUAGE = 39,
        SPELL_EFFECT_DUAL_WIELD = 40,
        SPELL_EFFECT_SUMMON_WILD = 41,
        SPELL_EFFECT_SUMMON_GUARDIAN = 42,
        SPELL_EFFECT_TELEPORT_UNITS_FACE_CASTER = 43,
        SPELL_EFFECT_SKILL_STEP = 44,
        SPELL_EFFECT_ADD_HONOR = 45,
        SPELL_EFFECT_SPAWN = 46,
        SPELL_EFFECT_TRADE_SKILL = 47,
        SPELL_EFFECT_STEALTH = 48,
        SPELL_EFFECT_DETECT = 49,
        SPELL_EFFECT_TRANS_DOOR = 50,
        SPELL_EFFECT_FORCE_CRITICAL_HIT = 51,
        SPELL_EFFECT_GUARANTEE_HIT = 52,
        SPELL_EFFECT_ENCHANT_ITEM = 53,
        SPELL_EFFECT_ENCHANT_ITEM_TEMPORARY = 54,
        SPELL_EFFECT_TAMECREATURE = 55,
        SPELL_EFFECT_SUMMON_PET = 56,
        SPELL_EFFECT_LEARN_PET_SPELL = 57,
        SPELL_EFFECT_WEAPON_DAMAGE = 58,
        SPELL_EFFECT_OPEN_LOCK_ITEM = 59,
        SPELL_EFFECT_PROFICIENCY = 60,
        SPELL_EFFECT_SEND_EVENT = 61,
        SPELL_EFFECT_POWER_BURN = 62,
        SPELL_EFFECT_THREAT = 63,
        SPELL_EFFECT_TRIGGER_SPELL = 64,
        SPELL_EFFECT_HEALTH_FUNNEL = 65,
        SPELL_EFFECT_POWER_FUNNEL = 66,
        SPELL_EFFECT_HEAL_MAX_HEALTH = 67,
        SPELL_EFFECT_INTERRUPT_CAST = 68,
        SPELL_EFFECT_DISTRACT = 69,
        SPELL_EFFECT_PULL = 70,
        SPELL_EFFECT_PICKPOCKET = 71,
        SPELL_EFFECT_ADD_FARSIGHT = 72,
        SPELL_EFFECT_SUMMON_POSSESSED = 73,
        SPELL_EFFECT_SUMMON_TOTEM = 74,
        SPELL_EFFECT_HEAL_MECHANICAL = 75,
        SPELL_EFFECT_SUMMON_OBJECT_WILD = 76,
        SPELL_EFFECT_SCRIPT_EFFECT = 77,
        SPELL_EFFECT_ATTACK = 78,
        SPELL_EFFECT_SANCTUARY = 79,
        SPELL_EFFECT_ADD_COMBO_POINTS = 80,
        SPELL_EFFECT_CREATE_HOUSE = 81,
        SPELL_EFFECT_BIND_SIGHT = 82,
        SPELL_EFFECT_DUEL = 83,
        SPELL_EFFECT_STUCK = 84,
        SPELL_EFFECT_SUMMON_PLAYER = 85,
        SPELL_EFFECT_ACTIVATE_OBJECT = 86,
        SPELL_EFFECT_SUMMON_TOTEM_SLOT1 = 87,
        SPELL_EFFECT_SUMMON_TOTEM_SLOT2 = 88,
        SPELL_EFFECT_SUMMON_TOTEM_SLOT3 = 89,
        SPELL_EFFECT_SUMMON_TOTEM_SLOT4 = 90,
        SPELL_EFFECT_THREAT_ALL = 91,
        SPELL_EFFECT_ENCHANT_HELD_ITEM = 92,
        SPELL_EFFECT_SUMMON_PHANTASM = 93,
        SPELL_EFFECT_SELF_RESURRECT = 94,
        SPELL_EFFECT_SKINNING = 95,
        SPELL_EFFECT_CHARGE = 96,
        SPELL_EFFECT_SUMMON_CRITTER = 97,
        SPELL_EFFECT_KNOCK_BACK = 98,
        SPELL_EFFECT_DISENCHANT = 99,
        SPELL_EFFECT_INEBRIATE = 100,
        SPELL_EFFECT_FEED_PET = 101,
        SPELL_EFFECT_DISMISS_PET = 102,
        SPELL_EFFECT_REPUTATION = 103,
        SPELL_EFFECT_SUMMON_OBJECT_SLOT1 = 104,
        SPELL_EFFECT_SUMMON_OBJECT_SLOT2 = 105,
        SPELL_EFFECT_SUMMON_OBJECT_SLOT3 = 106,
        SPELL_EFFECT_SUMMON_OBJECT_SLOT4 = 107,
        SPELL_EFFECT_DISPEL_MECHANIC = 108,
        SPELL_EFFECT_SUMMON_DEAD_PET = 109,
        SPELL_EFFECT_DESTROY_ALL_TOTEMS = 110,
        SPELL_EFFECT_DURABILITY_DAMAGE = 111,
        SPELL_EFFECT_SUMMON_DEMON = 112,
        SPELL_EFFECT_RESURRECT_NEW = 113,
        SPELL_EFFECT_ATTACK_ME = 114,
        SPELL_EFFECT_DURABILITY_DAMAGE_PCT = 115,
        SPELL_EFFECT_SKIN_PLAYER_CORPSE = 116,
        SPELL_EFFECT_SPIRIT_HEAL = 117,
        SPELL_EFFECT_SKILL = 118,
        SPELL_EFFECT_APPLY_AREA_AURA_PET = 119,
        SPELL_EFFECT_TELEPORT_GRAVEYARD = 120,
        SPELL_EFFECT_NORMALIZED_WEAPON_DMG = 121,
        SPELL_EFFECT_122 = 122,
        SPELL_EFFECT_SEND_TAXI = 123,
        SPELL_EFFECT_PLAYER_PULL = 124,
        SPELL_EFFECT_MODIFY_THREAT_PERCENT = 125,
        SPELL_EFFECT_126 = 126,
        SPELL_EFFECT_127 = 127,
        // Effets "backportes" depuis MaNGOS BC+.
        SPELL_EFFECT_APPLY_AREA_AURA_FRIEND = 128,
        SPELL_EFFECT_APPLY_AREA_AURA_ENEMY = 129,
        // Custom
        SPELL_EFFECT_DESPAWN_OBJECT = 130,
        SPELL_EFFECT_NOSTALRIUS = 131,
        SPELL_EFFECT_APPLY_AREA_AURA_RAID = 132,
        SPELL_EFFECT_APPLY_AREA_AURA_OWNER = 133,
        TOTAL_SPELL_EFFECTS = 134
    };


    enum SpellCastResult : std::uint8_t {
        SPELL_FAILED_AFFECTING_COMBAT = 0,           // 0x0
        SPELL_FAILED_ALREADY_AT_FULL_HEALTH = 1,     // 0x1
        SPELL_FAILED_ALREADY_AT_FULL_MANA = 2,       // 0x2
        SPELL_FAILED_ALREADY_BEING_TAMED = 3,        // 0x3
        SPELL_FAILED_ALREADY_HAVE_CHARM = 4,         // 0x4
        SPELL_FAILED_ALREADY_HAVE_SUMMON = 5,        // 0x5
        SPELL_FAILED_ALREADY_OPEN = 6,               // 0x6
        SPELL_FAILED_MORE_POWERFUL_SPELL_ACTIVE = 7, // 0x7
        SPELL_FAILED_BAD_IMPLICIT_TARGETS = 9,       // 0x9
        SPELL_FAILED_BAD_TARGETS = 10,               // 0xA
        SPELL_FAILED_CANT_BE_CHARMED = 11,           // 0xB
        SPELL_FAILED_CANT_BE_DISENCHANTED = 12,      // 0xC
        SPELL_FAILED_CANT_BE_PROSPECTED = 13,        // 0xD
        SPELL_FAILED_CANT_CAST_ON_TAPPED = 14,       // 0xE
        SPELL_FAILED_CANT_DUEL_WHILE_INVISIBLE = 15, // 0xF
        SPELL_FAILED_CANT_DUEL_WHILE_STEALTHED = 16, // 0x10
        SPELL_FAILED_CANT_TOO_CLOSE_TO_ENEMY = 17,   // 0x11
        SPELL_FAILED_CANT_DO_THAT_YET = 18,          // 0x12
        SPELL_FAILED_CASTER_DEAD = 19,               // 0x13
        SPELL_FAILED_CHARMED = 20,                   // 0x14
        SPELL_FAILED_CHEST_IN_USE = 21,              // 0x15
        SPELL_FAILED_CONFUSED = 22,                  // 0x16
        SPELL_FAILED_DONT_REPORT = 23,               // 0x17
        SPELL_FAILED_EQUIPPED_ITEM = 24,             // 0x18
        SPELL_FAILED_EQUIPPED_ITEM_CLASS = 25,       // 0x19
        SPELL_FAILED_EQUIPPED_ITEM_CLASS_MAINHAND = 26, // 0x1A
        SPELL_FAILED_EQUIPPED_ITEM_CLASS_OFFHAND = 27,  // 0x1B
        SPELL_FAILED_ERROR = 28,                     // 0x1C
        SPELL_FAILED_FIZZLE = 29,                    // 0x1D
        SPELL_FAILED_FLEEING = 30,                   // 0x1E
        SPELL_FAILED_FOOD_LOWLEVEL = 31,             // 0x1F
        SPELL_FAILED_HIGHLEVEL = 32,                 // 0x20
        SPELL_FAILED_IMMUNE = 34,                    // 0x22
        SPELL_FAILED_INTERRUPTED = 35,               // 0x23
        SPELL_FAILED_INTERRUPTED_COMBAT = 36,        // 0x24
        SPELL_FAILED_ITEM_ALREADY_ENCHANTED = 37,    // 0x25
        SPELL_FAILED_ITEM_GONE = 38,                 // 0x26
        SPELL_FAILED_ENCHANT_NOT_EXISTING_ITEM = 39, // 0x27
        SPELL_FAILED_ITEM_NOT_READY = 40,            // 0x28
        SPELL_FAILED_LEVEL_REQUIREMENT = 41,         // 0x29
        SPELL_FAILED_LINE_OF_SIGHT = 42,             // 0x2A
        SPELL_FAILED_LOWLEVEL = 43,                  // 0x2B
        SPELL_FAILED_SKILL_NOT_HIGH_ENOUGH = 44,     // 0x2C
        SPELL_FAILED_MAINHAND_EMPTY = 45,            // 0x2D
        SPELL_FAILED_MOVING = 46,                    // 0x2E
        SPELL_FAILED_NEED_AMMO = 47,                 // 0x2F
        SPELL_FAILED_NEED_REQUIRES_SOMETHING = 48,   // 0x30
        SPELL_FAILED_NEED_EXOTIC_AMMO = 49,          // 0x31
        SPELL_FAILED_NOPATH = 50,                    // 0x32
        SPELL_FAILED_NOT_BEHIND = 51,                // 0x33
        SPELL_FAILED_NOT_FISHABLE = 52,              // 0x34
        SPELL_FAILED_NOT_HERE = 53,                  // 0x35
        SPELL_FAILED_NOT_INFRONT = 54,               // 0x36
        SPELL_FAILED_NOT_IN_CONTROL = 55,            // 0x37
        SPELL_FAILED_NOT_KNOWN = 56,                 // 0x38
        SPELL_FAILED_NOT_MOUNTED = 57,               // 0x39
        SPELL_FAILED_NOT_ON_TAXI = 58,               // 0x3A
        SPELL_FAILED_NOT_ON_TRANSPORT = 59,          // 0x3B
        SPELL_FAILED_NOT_READY = 60,                 // 0x3C
        SPELL_FAILED_NOT_SHAPESHIFT = 61,            // 0x3D
        SPELL_FAILED_NOT_STANDING = 62,              // 0x3E
        SPELL_FAILED_NOT_TRADEABLE = 63,             // 0x3F
        SPELL_FAILED_NOT_TRADING = 64,               // 0x40
        SPELL_FAILED_NOT_UNSHEATHED = 65,            // 0x41
        SPELL_FAILED_NOT_WHILE_GHOST = 66,           // 0x42
        SPELL_FAILED_NO_AMMO = 67,                   // 0x43
        SPELL_FAILED_NO_CHARGES_REMAIN = 68,         // 0x44
        SPELL_FAILED_NO_CHAMPION = 69,               // 0x45
        SPELL_FAILED_NO_COMBO_POINTS = 70,           // 0x46
        SPELL_FAILED_NO_DUELING = 71,                // 0x47
        SPELL_FAILED_NO_ENDURANCE = 72,              // 0x48
        SPELL_FAILED_NO_FISH = 73,                   // 0x49
        SPELL_FAILED_NO_ITEMS_WHILE_SHAPESHIFTED = 74, // 0x4A
        SPELL_FAILED_NO_MOUNTS_ALLOWED = 75,         // 0x4B
        SPELL_FAILED_NO_PET = 76,                    // 0x4C
        SPELL_FAILED_NO_POWER = 77,                  // 0x4D
        SPELL_FAILED_NOTHING_TO_DISPEL = 78,         // 0x4E
        SPELL_FAILED_NOTHING_TO_STEAL = 79,          // 0x4F
        SPELL_FAILED_ONLY_ABOVEWATER = 80,           // 0x50
        SPELL_FAILED_ONLY_DAYTIME = 81,              // 0x51
        SPELL_FAILED_ONLY_INDOORS = 82,              // 0x52
        SPELL_FAILED_ONLY_MOUNTED = 83,              // 0x53
        SPELL_FAILED_ONLY_NIGHTTIME = 84,            // 0x54
        SPELL_FAILED_ONLY_OUTDOORS = 85,             // 0x55
        SPELL_FAILED_ONLY_SHAPESHIFT = 86,           // 0x56
        SPELL_FAILED_ONLY_STEALTHED = 87,            // 0x57
        SPELL_FAILED_ONLY_UNDERWATER = 88,           // 0x58
        SPELL_FAILED_OUT_OF_RANGE = 89,              // 0x59
        SPELL_FAILED_PACIFIED = 90,                  // 0x5A
        SPELL_FAILED_POSSESSED = 91,                 // 0x5B
        SPELL_FAILED_REQUIRES_AREA = 93,             // 0x5D
        SPELL_FAILED_REQUIRES_SPELL_FOCUS = 94,      // 0x5E
        SPELL_FAILED_ROOTED = 95,                    // 0x5F
        SPELL_FAILED_SILENCED = 96,                  // 0x60
        SPELL_FAILED_SPELL_IN_PROGRESS = 97,         // 0x61
        SPELL_FAILED_SPELL_LEARNED = 98,             // 0x62
        SPELL_FAILED_SPELL_UNAVAILABLE = 99,         // 0x63
        SPELL_FAILED_STUNNED = 100,                  // 0x64
        SPELL_FAILED_TARGETS_DEAD = 101,             // 0x65
        SPELL_FAILED_TARGET_AFFECTING_COMBAT = 102,  // 0x66
        SPELL_FAILED_TARGET_AURASTATE = 103,         // 0x67
        SPELL_FAILED_TARGET_DUELING = 104,           // 0x68
        SPELL_FAILED_TARGET_ENEMY = 105,             // 0x69
        SPELL_FAILED_TARGET_ENRAGED = 106,           // 0x6A
        SPELL_FAILED_TARGET_FRIENDLY = 107,          // 0x6B
        SPELL_FAILED_TARGET_IN_COMBAT = 108,         // 0x6C
        SPELL_FAILED_TARGET_IS_PLAYER = 109,         // 0x6D
        SPELL_FAILED_TARGET_NOT_DEAD = 110,          // 0x6E
        SPELL_FAILED_TARGET_NOT_IN_PARTY = 111,      // 0x6F
        SPELL_FAILED_TARGET_NOT_LOOTED = 112,        // 0x70
        SPELL_FAILED_TARGET_NOT_PLAYER = 113,        // 0x71
        SPELL_FAILED_TARGET_NO_POCKETS = 114,        // 0x72
        SPELL_FAILED_TARGET_NO_WEAPONS = 115,        // 0x73
        SPELL_FAILED_TARGET_UNSKINNABLE = 116,       // 0x74
        SPELL_FAILED_THIRST_SATIATED = 117,          // 0x75
        SPELL_FAILED_TOO_CLOSE = 118
    };

    enum SpellAttributes {
        SPELL_ATTR_UNK0 = 0x1,
        SPELL_ATTR_RANGED = 0x2,
        SPELL_ATTR_ON_NEXT_SWING_1 = 0x4,
        SPELL_ATTR_UNK3 = 0x8,
        SPELL_ATTR_ABILITY = 0x10,
        SPELL_ATTR_TRADESPELL = 0x20,
        SPELL_ATTR_PASSIVE = 0x40,
        SPELL_ATTR_HIDDEN_CLIENTSIDE = 0x80,
        SPELL_ATTR_HIDE_IN_COMBAT_LOG = 0x100,
        SPELL_ATTR_TARGET_MAINHAND_ITEM = 0x200,
        SPELL_ATTR_ON_NEXT_SWING_2 = 0x400,
        SPELL_ATTR_UNK11 = 0x800,
        SPELL_ATTR_DAYTIME_ONLY = 0x1000,
        SPELL_ATTR_NIGHT_ONLY = 0x2000,
        SPELL_ATTR_INDOORS_ONLY = 0x4000,
        SPELL_ATTR_OUTDOORS_ONLY = 0x8000,
        SPELL_ATTR_NOT_SHAPESHIFT = 0x10000,
        SPELL_ATTR_ONLY_STEALTHED = 0x20000,
        SPELL_ATTR_DONT_AFFECT_SHEATH_STATE = 0x40000,
        SPELL_ATTR_LEVEL_DAMAGE_CALCULATION = 0x80000,
        SPELL_ATTR_STOP_ATTACK_TARGET = 0x100000,
        SPELL_ATTR_IMPOSSIBLE_DODGE_PARRY_BLOCK = 0x200000,
        SPELL_ATTR_SET_TRACKING_TARGET = 0x400000,
        SPELL_ATTR_CASTABLE_WHILE_DEAD = 0x800000,
        SPELL_ATTR_CASTABLE_WHILE_MOUNTED = 0x1000000,
        SPELL_ATTR_DISABLED_WHILE_ACTIVE = 0x2000000,
        SPELL_ATTR_NEGATIVE = 0x4000000,
        SPELL_ATTR_CASTABLE_WHILE_SITTING = 0x8000000,
        SPELL_ATTR_CANT_USED_IN_COMBAT = 0x10000000,
        SPELL_ATTR_UNAFFECTED_BY_INVULNERABILITY = 0x20000000,
        SPELL_ATTR_UNK30 = 0x40000000,
        SPELL_ATTR_CANT_CANCEL = 0x80000000,
    };

    enum SpellAttributesEx {
        SPELL_ATTR_EX_DISMISS_PET_FIRST = 0x00000001,            // 0 For spells without this flag client doesn't allow to summon pet if caster has a pet
        SPELL_ATTR_EX_USE_ALL_MANA = 0x00000002,            // 1 Use all power (Only paladin Lay of Hands and Bunyanize)
        SPELL_ATTR_EX_IS_CHANNELED = 0x00000004,            // 2
        SPELL_ATTR_EX_NO_REDIRECTION = 0x00000008,            // 3
        SPELL_ATTR_EX_NO_SKILL_INCREASE = 0x00000010,            // 4 Only assigned to stealth spells for some reason
        SPELL_ATTR_EX_ALLOW_WHILE_STEALTHED = 0x00000020,            // 5 Does not break stealth
        SPELL_ATTR_EX_IS_SELF_CHANNELED = 0x00000040,            // 6
        SPELL_ATTR_EX_NO_REFLECTION = 0x00000080,            // 7
        SPELL_ATTR_EX_ONLY_PEACEFUL_TARGETS = 0x00000100,            // 8 Target must not be in combat
        SPELL_ATTR_EX_INITIATES_COMBAT = 0x00000200,            // 9 Enables Auto-Attack
        SPELL_ATTR_EX_NO_THREAT = 0x00000400,            // 10
        SPELL_ATTR_EX_AURA_UNIQUE = 0x00000800,            // 11
        SPELL_ATTR_EX_FAILURE_BREAKS_STEALTH = 0x00001000,            // 12
        SPELL_ATTR_EX_TOGGLE_FARSIGHT = 0x00002000,            // 13
        SPELL_ATTR_EX_TRACK_TARGET_IN_CHANNEL = 0x00004000,            // 14 Client automatically forces player to face target when channeling
        SPELL_ATTR_EX_IMMUNITY_PURGES_EFFECT = 0x00008000,            // 15 Remove auras on immunity
        SPELL_ATTR_EX_IMMUNITY_TO_HOSTILE_AND_FRIENDLY_EFFECTS = 0x00010000, // 16 Aura that provides immunity prevents positive effects too
        SPELL_ATTR_EX_NO_AUTOCAST_AI = 0x00020000,            // 17
        SPELL_ATTR_EX_PREVENTS_ANIM = 0x00040000,            // 18 Stun, polymorph, daze, sleep
        SPELL_ATTR_EX_EXCLUDE_CASTER = 0x00080000,            // 19
        SPELL_ATTR_EX_FINISHING_MOVE_DAMAGE = 0x00100000,            // 20 Uses combo points
        SPELL_ATTR_EX_THREAT_ONLY_ON_MISS = 0x00200000,            // 21
        SPELL_ATTR_EX_FINISHING_MOVE_DURATION = 0x00400000,            // 22 Uses combo points (in 4.x not required combo point target selected)
        SPELL_ATTR_EX_IGNORE_CASTER_AND_TARGET_RESTRICTIONS = 0x00800000,    // 23 Skips all cast checks, moved to AttributesEx3 after 1.10 (100% correlation)
        SPELL_ATTR_EX_SPECIAL_SKILLUP = 0x01000000,            // 24 Only fishing spells
        SPELL_ATTR_EX_UNK25 = 0x02000000,            // 25 Different in vanilla
        SPELL_ATTR_EX_REQUIRE_ALL_TARGETS = 0x04000000,            // 26
        SPELL_ATTR_EX_DISCOUNT_POWER_ON_MISS = 0x08000000,            // 27 All these spells refund power on parry or deflect
        SPELL_ATTR_EX_NO_AURA_ICON = 0x10000000,            // 28 Client doesn't display these spells in aura bar
        SPELL_ATTR_EX_NAME_IN_CHANNEL_BAR = 0x20000000,            // 29 Spell name is displayed in cast bar instead of 'channeling' text
        SPELL_ATTR_EX_COMBO_ON_BLOCK = 0x40000000,            // 30 Overpower
        SPELL_ATTR_EX_CAST_WHEN_LEARNED = 0x80000000             // 31
    };

    enum SpellAttributesEx2 {
        SPELL_ATTR_EX2_ALLOW_DEAD_TARGET = 0x00000001,            // 0 Can target dead unit or corpse
        SPELL_ATTR_EX2_NO_SHAPESHIFT_UI = 0x00000002,            // 1
        SPELL_ATTR_EX2_IGNORE_LINE_OF_SIGHT = 0x00000004,            // 2
        SPELL_ATTR_EX2_ALLOW_LOW_LEVEL_BUFF = 0x00000008,            // 3
        SPELL_ATTR_EX2_USE_SHAPESHIFT_BAR = 0x00000010,            // 4 Client displays icon in stance bar when learned, even if not shapeshift
        SPELL_ATTR_EX2_AUTO_REPEAT = 0x00000020,            // 5
        SPELL_ATTR_EX2_CANNOT_CAST_ON_TAPPED = 0x00000040,            // 6 Target must be tapped by caster
        SPELL_ATTR_EX2_DO_NOT_REPORT_SPELL_FAILURE = 0x00000080,            // 7
        SPELL_ATTR_EX2_UNK8 = 0x00000100,            // 8 Unused
        SPELL_ATTR_EX2_UNK9 = 0x00000200,            // 9 Unused
        SPELL_ATTR_EX2_SPECIAL_TAMING_FLAG = 0x00000400,            // 10
        SPELL_ATTR_EX2_NO_TARGET_PER_SECOND_COSTS = 0x00000800,            // 11
        SPELL_ATTR_EX2_CHAIN_FROM_CASTER = 0x00001000,            // 12
        SPELL_ATTR_EX2_ENCHANT_OWN_ITEM_ONLY = 0x00002000,            // 13
        SPELL_ATTR_EX2_ALLOW_WHILE_INVISIBLE = 0x00004000,            // 14
        SPELL_ATTR_EX2_ENABLE_AFTER_PARRY = 0x00008000,            // 15 Deprecated in patch 1.8 and moved to CasterAuraState
        SPELL_ATTR_EX2_NO_ACTIVE_PETS = 0x00010000,            // 16
        SPELL_ATTR_EX2_DO_NOT_RESET_COMBAT_TIMERS = 0x00020000,            // 17 Don't reset timers for melee autoattacks (swings) or ranged autoattacks (autoshoots)
        SPELL_ATTR_EX2_REQ_DEAD_PET = 0x00040000,            // 18 Only Revive pet has it
        SPELL_ATTR_EX2_ALLOW_WHILE_NOT_SHAPESHIFTED = 0x00080000,            // 19 Does not necessary need shapeshift (pre-3.x not have passive spells with this attribute)
        SPELL_ATTR_EX2_INITIATE_COMBAT_POST_CAST = 0x00100000,            // 20 Client will send CMSG_ATTACK_SWING after SMSG_SPELL_GO
        SPELL_ATTR_EX2_FAIL_ON_ALL_TARGETS_IMMUNE = 0x00200000,            // 21 For ice blocks, pala immunity buffs, priest absorb shields
        SPELL_ATTR_EX2_NO_INITIAL_THREAT = 0x00400000,            // 22
        SPELL_ATTR_EX2_PROC_COOLDOWN_ON_FAILURE = 0x00800000,            // 23
        SPELL_ATTR_EX2_ITEM_CAST_WITH_OWNER_SKILL = 0x01000000,            // 24 NYI
        SPELL_ATTR_EX2_DONT_BLOCK_MANA_REGEN = 0x02000000,            // 25
        SPELL_ATTR_EX2_NO_SCHOOL_IMMUNITIES = 0x04000000,            // 26
        SPELL_ATTR_EX2_IGNORE_WEAPONSKILL = 0x08000000,            // 27 NYI (only fishing has it)
        SPELL_ATTR_EX2_NOT_AN_ACTION = 0x10000000,            // 28
        SPELL_ATTR_EX2_CANT_CRIT = 0x20000000,            // 29
        SPELL_ATTR_EX2_ACTIVE_THREAT = 0x40000000,            // 30 Caster is put in combat for 5.5 seconds on cast at enemy unit
        SPELL_ATTR_EX2_RETAIN_ITEM_CAST = 0x80000000             // 31 Food or Drink Buff (like Well Fed)
    };

    enum SpellAttributesEx3 {
        SPELL_ATTR_EX3_PVP_ENABLING = 0x00000001,            // 0 Spell landed counts as hostile action against enemy even if it doesn't trigger combat state, propagates PvP flags
        SPELL_ATTR_EX3_NO_PROC_EQUIP_REQUIREMENT = 0x00000002,            // 1
        SPELL_ATTR_EX3_NO_CASTING_BAR_TEXT = 0x00000004,            // 2
        SPELL_ATTR_EX3_COMPLETELY_BLOCKED = 0x00000008,            // 3 All effects prevented on block
        SPELL_ATTR_EX3_NO_RES_TIMER = 0x00000010,            // 4 Corpse reclaim delay does not apply to accepting resurrection (only Rebirth has it)
        SPELL_ATTR_EX3_NO_DURABILITY_LOSS = 0x00000020,            // 5
        SPELL_ATTR_EX3_NO_AVOIDANCE = 0x00000040,            // 6 Persistent Area Aura not removed on leaving radius
        SPELL_ATTR_EX3_DOT_STACKING_RULE = 0x00000080,            // 7 Create a separate (de)buff stack for each caster
        SPELL_ATTR_EX3_ONLY_ON_PLAYER = 0x00000100,            // 8 Can target only players
        SPELL_ATTR_EX3_NOT_A_PROC = 0x00000200,            // 9 Aura periodic trigger is not evaluated as triggered
        SPELL_ATTR_EX3_REQUIRES_MAIN_HAND_WEAPON = 0x00000400,            // 10
        SPELL_ATTR_EX3_ONLY_BATTLEGROUNDS = 0x00000800,            // 11
        SPELL_ATTR_EX3_ONLY_ON_GHOSTS = 0x00001000,            // 12
        SPELL_ATTR_EX3_HIDE_CHANNEL_BAR = 0x00002000,            // 13 Client will not display channeling bar
        SPELL_ATTR_EX3_HIDE_IN_RAID_FILTER = 0x00004000,            // 14 Only "Honorless Target" has this flag
        SPELL_ATTR_EX3_NORMAL_RANGED_ATTACK = 0x00008000,            // 15 Spells with this attribute are processed as ranged attacks in client
        SPELL_ATTR_EX3_SUPPRESS_CASTER_PROCS = 0x00010000,            // 16
        SPELL_ATTR_EX3_SUPPRESS_TARGET_PROCS = 0x00020000,            // 17
        SPELL_ATTR_EX3_ALWAYS_HIT = 0x00040000,            // 18 Spell should always hit its target
        SPELL_ATTR_EX3_INSTANT_TARGET_PROCS = 0x00080000,            // 19 Related to spell batching
        SPELL_ATTR_EX3_ALLOW_AURA_WHILE_DEAD = 0x00100000,            // 20 Death persistent spells
        SPELL_ATTR_EX3_ONLY_PROC_OUTDOORS = 0x00200000,            // 21
        SPELL_ATTR_EX3_CASTING_CANCELS_AUTOREPEAT = 0x00400000,            // 22 NYI (only Shoot with Wand has it)
        SPELL_ATTR_EX3_NO_DAMAGE_HISTORY = 0x00800000,            // 23 NYI
        SPELL_ATTR_EX3_REQUIRES_OFFHAND_WEAPON = 0x01000000,            // 24
        SPELL_ATTR_EX3_TREAT_AS_PERIODIC = 0x02000000,            // 25 Does not cause spell pushback
        SPELL_ATTR_EX3_CAN_PROC_FROM_PROCS = 0x04000000,            // 26 Auras with this attribute can proc off procced spells (periodic triggers etc)
        SPELL_ATTR_EX3_ONLY_PROC_ON_CASTER = 0x08000000,            // 27
        SPELL_ATTR_EX3_IGNORE_CASTER_AND_TARGET_RESTRICTIONS = 0x10000000,   // 28 Skips all cast checks, moved from AttributesEx after 1.10 (100% correlation)
        SPELL_ATTR_EX3_IGNORE_CASTER_MODIFIERS = 0x20000000,            // 29
        SPELL_ATTR_EX3_DO_NOT_DISPLAY_RANGE = 0x40000000,            // 30
        SPELL_ATTR_EX3_NOT_ON_AOE_IMMUNE = 0x80000000             // 31
    };

    enum SpellAttributesEx4 {
        SPELL_ATTR_EX4_IGNORE_RESISTANCES = 0x00000001,            // 0 From TC 3.3.5, but not present in 1.12 native DBCs. Add it with spell_mod to prevent a spell from being resisted.
        SPELL_ATTR_EX4_CLASS_TRIGGER_ONLY_ON_TARGET = 0x00000002,            // 1
        SPELL_ATTR_EX4_AURA_EXPIRES_OFFLINE = 0x00000004,            // 2 Aura continues to expire while player is offline
        SPELL_ATTR_EX4_NO_HELPFUL_THREAT = 0x00000008,            // 3
        SPELL_ATTR_EX4_NO_HARMFUL_THREAT = 0x00000010,            // 4
        SPELL_ATTR_EX4_ALLOW_CLIENT_TARGETING = 0x00000020,            // 5 NYI
        SPELL_ATTR_EX4_CANNOT_BE_STOLEN = 0x00000040,            // 6 Unused
        SPELL_ATTR_EX4_CAN_CAST_WHILE_CASTING = 0x00000080,            // 7 NYI (does not seem to work client side either)
        SPELL_ATTR_EX4_IGNORE_DAMAGE_TAKEN_MODIFIERS = 0x00000100,            // 8
        SPELL_ATTR_EX4_COMBAT_FEEDBACK_WHEN_USABLE = 0x00000200,            // 9 Initially disabled / Trigger activate from event (Execute, Riposte, Deep Freeze...)
    };

// Custom flags assigned in the db
    enum SpellAttributesCustom {
        SPELL_CUSTOM_NONE = 0x000,
        SPELL_CUSTOM_ALLOW_STACK_BETWEEN_CASTER = 0x001,     // For example 'Siphon Soul' must be able to stack between the warlocks on a mob
        SPELL_CUSTOM_NEGATIVE = 0x002,
        SPELL_CUSTOM_POSITIVE = 0x004,
        SPELL_CUSTOM_CHAN_NO_DIST_LIMIT = 0x008,
        SPELL_CUSTOM_FIXED_DAMAGE = 0x010,     // Not affected by damage/healing done bonus
        SPELL_CUSTOM_IGNORE_ARMOR = 0x020,
        SPELL_CUSTOM_BEHIND_TARGET = 0x040,     // For spells that require the caster to be behind the target
        SPELL_CUSTOM_FACE_TARGET = 0x080,     // For spells that require the target to be in front of the caster
        SPELL_CUSTOM_SINGLE_TARGET_AURA = 0x100,     // Aura applied by spell can only be on 1 target at a time
        SPELL_CUSTOM_AURA_APPLY_BREAKS_STEALTH = 0x200,     // Stealth is removed when this aura is applied
        SPELL_CUSTOM_NOT_REMOVED_ON_EVADE = 0x400,     // Aura persists after creature evades
        SPELL_CUSTOM_SEND_CHANNEL_VISUAL = 0x800,     // Will periodically send the channeling spell visual kit
        SPELL_CUSTOM_SEPARATE_AURA_PER_CASTER = 0x1000,    // Each caster has his own aura slot, instead of replacing others
    };
    enum SpellTarget {
        TARGET_NONE = 1,
        TARGET_UNIT_CASTER = 2,
        TARGET_UNIT_ENEMY_NEAR_CASTER = 3,
        TARGET_UNIT_FRIEND_NEAR_CASTER = 4,
        TARGET_UNIT_NEAR_CASTER = 5,
        TARGET_UNIT_CASTER_PET = 6,
        TARGET_UNIT_ENEMY = 7,
        TARGET_ENUM_UNITS_SCRIPT_AOE_AT_SRC_LOC = 8,
        TARGET_ENUM_UNITS_SCRIPT_AOE_AT_DEST_LOC = 9,
        TARGET_LOCATION_CASTER_HOME_BIND = 10,
        TARGET_LOCATION_CASTER_DIVINE_BIND_NYI = 11,
        TARGET_PLAYER_NYI = 12,
        TARGET_PLAYER_NEAR_CASTER_NYI = 13,
        TARGET_PLAYER_ENEMY_NYI = 14,
        TARGET_PLAYER_FRIEND_NYI = 15,
        TARGET_ENUM_UNITS_ENEMY_AOE_AT_SRC_LOC = 16,
        TARGET_ENUM_UNITS_ENEMY_AOE_AT_DEST_LOC = 17,
        TARGET_LOCATION_DATABASE = 18,
        TARGET_LOCATION_CASTER_DEST = 19,
        TARGET_UNK_19 = 20,
        TARGET_ENUM_UNITS_PARTY_WITHIN_CASTER_RANGE = 21,
        TARGET_UNIT_FRIEND = 22,
        TARGET_LOCATION_CASTER_SRC = 23,
        TARGET_GAMEOBJECT = 24,
        TARGET_ENUM_UNITS_ENEMY_IN_CONE_24 = 25,
        TARGET_UNIT = 26,
        TARGET_LOCKED = 27,
        TARGET_UNIT_CASTER_MASTER = 28,
        TARGET_ENUM_UNITS_ENEMY_AOE_AT_DYNOBJ_LOC = 29,
        TARGET_ENUM_UNITS_FRIEND_AOE_AT_DYNOBJ_LOC = 30,
        TARGET_ENUM_UNITS_FRIEND_AOE_AT_SRC_LOC = 31,
        TARGET_ENUM_UNITS_FRIEND_AOE_AT_DEST_LOC = 32,
        TARGET_LOCATION_UNIT_MINION_POSITION = 33,
        TARGET_ENUM_UNITS_PARTY_AOE_AT_SRC_LOC = 34,
        TARGET_ENUM_UNITS_PARTY_AOE_AT_DEST_LOC = 35,
        TARGET_UNIT_PARTY = 36,
        TARGET_ENUM_UNITS_ENEMY_WITHIN_CASTER_RANGE = 37, // TODO: only used with dest-effects - reinvestigate naming
        TARGET_UNIT_FRIEND_AND_PARTY = 38,
        TARGET_UNIT_SCRIPT_NEAR_CASTER = 39,
        TARGET_LOCATION_CASTER_FISHING_SPOT = 40,
        TARGET_GAMEOBJECT_SCRIPT_NEAR_CASTER = 41,
        TARGET_LOCATION_CASTER_FRONT_RIGHT = 42,
        TARGET_LOCATION_CASTER_BACK_RIGHT = 43,
        TARGET_LOCATION_CASTER_BACK_LEFT = 44,
        TARGET_LOCATION_CASTER_FRONT_LEFT = 45,
        TARGET_UNIT_FRIEND_CHAIN_HEAL = 46,
        TARGET_LOCATION_SCRIPT_NEAR_CASTER = 47,
        TARGET_LOCATION_CASTER_FRONT = 48,
        TARGET_LOCATION_CASTER_BACK = 49,
        TARGET_LOCATION_CASTER_LEFT = 50,
        TARGET_LOCATION_CASTER_RIGHT = 51,
        TARGET_ENUM_GAMEOBJECTS_SCRIPT_AOE_AT_SRC_LOC = 52,
        TARGET_ENUM_GAMEOBJECTS_SCRIPT_AOE_AT_DEST_LOC = 53,
        TARGET_LOCATION_CASTER_TARGET_POSITION = 54,
        TARGET_ENUM_UNITS_ENEMY_IN_CONE_54 = 55,
        TARGET_LOCATION_CASTER_FRONT_LEAP = 56,
        TARGET_ENUM_UNITS_RAID_WITHIN_CASTER_RANGE = 57,
        TARGET_UNIT_RAID = 58,
        TARGET_UNIT_RAID_NEAR_CASTER = 59,
        TARGET_ENUM_UNITS_FRIEND_IN_CONE = 60,
        TARGET_ENUM_UNITS_SCRIPT_IN_CONE_60 = 61,
        TARGET_UNIT_RAID_AND_CLASS = 62,
        TARGET_PLAYER_RAID_NYI = 63,
        TARGET_LOCATION_UNIT_POSITION = 64,
    };

    enum Events : std::uint32_t {
        UNIT_NAME_UPDATE = 183,
        UNIT_PORTRAIT_UPDATE = 184,
        UNIT_MODEL_CHANGED = 185,
        UNIT_INVENTORY_CHANGED = 186,
        UNIT_CLASSIFICATION_CHANGED = 187,
        ITEM_LOCK_CHANGED = 188,
        PLAYER_XP_UPDATE = 189,
        PLAYER_REGEN_DISABLED = 190,
        PLAYER_REGEN_ENABLED = 191,
        PLAYER_AURAS_CHANGED = 192,
        PLAYER_ENTER_COMBAT = 193,
        PLAYER_LEAVE_COMBAT = 194,
        PLAYER_TARGET_CHANGED = 195,
        PLAYER_CONTROL_LOST = 196,
        PLAYER_CONTROL_GAINED = 197,
        PLAYER_FARSIGHT_FOCUS_CHANGED = 198,
        PLAYER_LEVEL_UP = 199,
        PLAYER_MONEY = 200,
        PLAYER_DAMAGE_DONE_MODS = 201,
        PLAYER_COMBO_POINTS = 202,
        ZONE_CHANGED = 203,
        ZONE_CHANGED_INDOORS = 204,
        ZONE_CHANGED_NEW_AREA = 205,
        MINIMAP_ZONE_CHANGED = 206,
        MINIMAP_UPDATE_ZOOM = 207,
        SCREENSHOT_SUCCEEDED = 208,
        SCREENSHOT_FAILED = 209,
        ACTIONBAR_SHOWGRID = 210,
        ACTIONBAR_HIDEGRID = 211,
        ACTIONBAR_PAGE_CHANGED = 212,
        ACTIONBAR_SLOT_CHANGED = 213,
        ACTIONBAR_UPDATE_STATE = 214,
        ACTIONBAR_UPDATE_USABLE = 215,
        ACTIONBAR_UPDATE_COOLDOWN = 216,
        UPDATE_BONUS_ACTIONBAR = 217,
        PARTY_MEMBERS_CHANGED = 218,
        PARTY_LEADER_CHANGED = 219,
        PARTY_MEMBER_ENABLE = 220,
        PARTY_MEMBER_DISABLE = 221,
        PARTY_LOOT_METHOD_CHANGED = 222,
        SYSMSG = 223,
        UI_ERROR_MESSAGE = 224,
        UI_INFO_MESSAGE = 225,
        UPDATE_CHAT_COLOR = 226,
        CHAT_MSG_ADDON = 227,
        CHAT_MSG_SAY = 228,
        CHAT_MSG_PARTY = 229,
        CHAT_MSG_RAID = 230,
        CHAT_MSG_GUILD = 231,
        CHAT_MSG_OFFICER = 232,
        CHAT_MSG_YELL = 233,
        CHAT_MSG_WHISPER = 234,
        CHAT_MSG_WHISPER_INFORM = 235,
        CHAT_MSG_EMOTE = 236,
        CHAT_MSG_TEXT_EMOTE = 237,
        CHAT_MSG_SYSTEM = 238,
        CHAT_MSG_MONSTER_SAY = 239,
        CHAT_MSG_MONSTER_YELL = 240,
        CHAT_MSG_MONSTER_WHISPER = 241,
        CHAT_MSG_MONSTER_EMOTE = 242,
        CHAT_MSG_CHANNEL = 243,
        CHAT_MSG_CHANNEL_JOIN = 244,
        CHAT_MSG_CHANNEL_LEAVE = 245,
        CHAT_MSG_CHANNEL_LIST = 246,
        CHAT_MSG_CHANNEL_NOTICE = 247,
        CHAT_MSG_CHANNEL_NOTICE_USER = 248,
        CHAT_MSG_AFK = 249,
        CHAT_MSG_DND = 250,
        CHAT_MSG_COMBAT_LOG = 251,
        CHAT_MSG_IGNORED = 252,
        CHAT_MSG_SKILL = 253,
        CHAT_MSG_LOOT = 254,
        CHAT_MSG_MONEY = 255,
        CHAT_MSG_RAID_LEADER = 256,
        CHAT_MSG_RAID_WARNING = 257,
        LANGUAGE_LIST_CHANGED = 258,
        TIME_PLAYED_MSG = 259,
        SPELLS_CHANGED = 260,
        CURRENT_SPELL_CAST_CHANGED = 261,
        SPELL_UPDATE_COOLDOWN = 262,
        SPELL_UPDATE_USABLE = 263,
        CHARACTER_POINTS_CHANGED = 264,
        SKILL_LINES_CHANGED = 265,
        ITEM_PUSH = 266,
        LOOT_OPENED = 267,
        LOOT_SLOT_CLEARED = 268,
        LOOT_CLOSED = 269,
        PLAYER_LOGIN = 270,
        PLAYER_LOGOUT = 271,
        PLAYER_ENTERING_WORLD = 272,
        PLAYER_LEAVING_WORLD = 273,
        PLAYER_ALIVE = 274,
        PLAYER_DEAD = 275,
        PLAYER_CAMPING = 276,
        PLAYER_QUITING = 277,
        LOGOUT_CANCEL = 278,
        RESURRECT_REQUEST = 279,
        PARTY_INVITE_REQUEST = 280,
        PARTY_INVITE_CANCEL = 281,
        GUILD_INVITE_REQUEST = 282,
        GUILD_INVITE_CANCEL = 283,
        GUILD_MOTD = 284,
        TRADE_REQUEST = 285,
        TRADE_REQUEST_CANCEL = 286,
        LOOT_BIND_CONFIRM = 287,
        EQUIP_BIND_CONFIRM = 288,
        AUTOEQUIP_BIND_CONFIRM = 289,
        USE_BIND_CONFIRM = 290,
        DELETE_ITEM_CONFIRM = 291,
        CURSOR_UPDATE = 292,
        ITEM_TEXT_BEGIN = 293,
        ITEM_TEXT_TRANSLATION = 294,
        ITEM_TEXT_READY = 295,
        ITEM_TEXT_CLOSED = 296,
        GOSSIP_SHOW = 297,
        GOSSIP_ENTER_CODE = 298,
        GOSSIP_CLOSED = 299,
        QUEST_GREETING = 300,
        QUEST_DETAIL = 301,
        QUEST_PROGRESS = 302,
        QUEST_COMPLETE = 303,
        QUEST_FINISHED = 304,
        QUEST_ITEM_UPDATE = 305,
        TAXIMAP_OPENED = 306,
        TAXIMAP_CLOSED = 307,
        QUEST_LOG_UPDATE = 308,
        TRAINER_SHOW = 309,
        TRAINER_UPDATE = 310,
        TRAINER_CLOSED = 311,
        CVAR_UPDATE = 312,
        TRADE_SKILL_SHOW = 313,
        TRADE_SKILL_UPDATE = 314,
        TRADE_SKILL_CLOSE = 315,
        MERCHANT_SHOW = 316,
        MERCHANT_UPDATE = 317,
        MERCHANT_CLOSED = 318,
        TRADE_SHOW = 319,
        TRADE_CLOSED = 320,
        TRADE_UPDATE = 321,
        TRADE_ACCEPT_UPDATE = 322,
        TRADE_TARGET_ITEM_CHANGED = 323,
        TRADE_PLAYER_ITEM_CHANGED = 324,
        TRADE_MONEY_CHANGED = 325,
        PLAYER_TRADE_MONEY = 326,
        BAG_OPEN = 327,
        BAG_UPDATE = 328,
        BAG_CLOSED = 329,
        BAG_UPDATE_COOLDOWN = 330,
        LOCALPLAYER_PET_RENAMED = 331,
        UNIT_ATTACK = 332,
        UNIT_DEFENSE = 333,
        PET_ATTACK_START = 334,
        PET_ATTACK_STOP = 335,
        UPDATE_MOUSEOVER_UNIT = 336,
        SPELLCAST_START = 337,
        SPELLCAST_STOP = 338,
        SPELLCAST_FAILED = 339,
        SPELLCAST_INTERRUPTED = 340,
        SPELLCAST_DELAYED = 341,
        SPELLCAST_CHANNEL_START = 342,
        SPELLCAST_CHANNEL_UPDATE = 343,
        SPELLCAST_CHANNEL_STOP = 344,
        PLAYER_GUILD_UPDATE = 345,
        QUEST_ACCEPT_CONFIRM = 346,
        PLAYERBANKSLOTS_CHANGED = 347,
        BANKFRAME_OPENED = 348,
        BANKFRAME_CLOSED = 349,
        PLAYERBANKBAGSLOTS_CHANGED = 350,
        FRIENDLIST_UPDATE = 351,
        IGNORELIST_UPDATE = 352,
        PET_BAR_UPDATE_COOLDOWN = 354,
        PET_BAR_UPDATE = 353,
        PET_BAR_SHOWGRID = 355,
        PET_BAR_HIDEGRID = 356,
        MINIMAP_PING = 357,
        CHAT_MSG_COMBAT_MISC_INFO = 358,
        CRAFT_SHOW = 359,
        CRAFT_UPDATE = 360,
        CRAFT_CLOSE = 361,
        MIRROR_TIMER_START = 362,
        MIRROR_TIMER_PAUSE = 363,
        MIRROR_TIMER_STOP = 364,
        WORLD_MAP_UPDATE = 365,
        WORLD_MAP_NAME_UPDATE = 366,
        AUTOFOLLOW_BEGIN = 367,
        AUTOFOLLOW_END = 368,
        CINEMATIC_START = 370,
        CINEMATIC_STOP = 371,
        UPDATE_FACTION = 372,
        CLOSE_WORLD_MAP = 373,
        OPEN_TABARD_FRAME = 374,
        CLOSE_TABARD_FRAME = 375,
        SHOW_COMPARE_TOOLTIP = 377,
        TABARD_CANSAVE_CHANGED = 376,
        GUILD_REGISTRAR_SHOW = 378,
        GUILD_REGISTRAR_CLOSED = 379,
        DUEL_REQUESTED = 380,
        DUEL_OUTOFBOUNDS = 381,
        DUEL_INBOUNDS = 382,
        DUEL_FINISHED = 383,
        TUTORIAL_TRIGGER = 384,
        PET_DISMISS_START = 385,
        UPDATE_BINDINGS = 386,
        UPDATE_SHAPESHIFT_FORMS = 387,
        WHO_LIST_UPDATE = 388,
        UPDATE_LFG = 389,
        PETITION_SHOW = 390,
        PETITION_CLOSED = 391,
        EXECUTE_CHAT_LINE = 392,
        UPDATE_MACROS = 393,
        UPDATE_TICKET = 394,
        UPDATE_CHAT_WINDOWS = 395,
        CONFIRM_XP_LOSS = 396,
        CORPSE_IN_RANGE = 397,
        CORPSE_IN_INSTANCE = 398,
        CORPSE_OUT_OF_RANGE = 399,
        UPDATE_GM_STATUS = 400,
        PLAYER_UNGHOST = 401,
        BIND_ENCHANT = 402,
        REPLACE_ENCHANT = 403,
        TRADE_REPLACE_ENCHANT = 404,
        PLAYER_UPDATE_RESTING = 405,
        UPDATE_EXHAUSTION = 406,
        PLAYER_FLAGS_CHANGED = 407,
        GUILD_ROSTER_UPDATE = 408,
        GM_PLAYER_INFO = 409,
        MAIL_SHOW = 410,
        MAIL_CLOSED = 411,
        SEND_MAIL_MONEY_CHANGED = 412,
        SEND_MAIL_COD_CHANGED = 413,
        MAIL_SEND_INFO_UPDATE = 414,
        MAIL_SEND_SUCCESS = 415,
        MAIL_INBOX_UPDATE = 416,
        BATTLEFIELDS_SHOW = 417,
        BATTLEFIELDS_CLOSED = 418,
        UPDATE_BATTLEFIELD_STATUS = 419,
        UPDATE_BATTLEFIELD_SCORE = 420,
        AUCTION_HOUSE_SHOW = 421,
        AUCTION_HOUSE_CLOSED = 422,
        NEW_AUCTION_UPDATE = 423,
        AUCTION_ITEM_LIST_UPDATE = 424,
        AUCTION_OWNED_LIST_UPDATE = 425,
        AUCTION_BIDDER_LIST_UPDATE = 426,
        PET_UI_UPDATE = 427,
        PET_UI_CLOSE = 428,
        ADDON_LOADED = 429,
        VARIABLES_LOADED = 430,
        MACRO_ACTION_FORBIDDEN = 431,
        ADDON_ACTION_FORBIDDEN = 432,
        MEMORY_EXHAUSTED = 433,
        MEMORY_RECOVERED = 434,
        START_AUTOREPEAT_SPELL = 435,
        STOP_AUTOREPEAT_SPELL = 436,
        PET_STABLE_SHOW = 437,
        PET_STABLE_UPDATE = 438,
        PET_STABLE_UPDATE_PAPERDOLL = 439,
        PET_STABLE_CLOSED = 440,
        CHAT_MSG_COMBAT_SELF_HITS = 441,
        CHAT_MSG_COMBAT_SELF_MISSES = 442,
        CHAT_MSG_COMBAT_PET_HITS = 443,
        CHAT_MSG_COMBAT_PET_MISSES = 444,
        CHAT_MSG_COMBAT_PARTY_HITS = 445,
        CHAT_MSG_COMBAT_PARTY_MISSES = 446,
        CHAT_MSG_COMBAT_FRIENDLYPLAYER_HITS = 447,
        CHAT_MSG_COMBAT_FRIENDLYPLAYER_MISSES = 448,
        CHAT_MSG_COMBAT_HOSTILEPLAYER_HITS = 449,
        CHAT_MSG_COMBAT_HOSTILEPLAYER_MISSES = 450,
        CHAT_MSG_COMBAT_CREATURE_VS_SELF_HITS = 451,
        CHAT_MSG_COMBAT_CREATURE_VS_SELF_MISSES = 452,
        CHAT_MSG_COMBAT_CREATURE_VS_PARTY_HITS = 453,
        CHAT_MSG_COMBAT_CREATURE_VS_PARTY_MISSES = 454,
        CHAT_MSG_COMBAT_CREATURE_VS_CREATURE_HITS = 455,
        CHAT_MSG_COMBAT_CREATURE_VS_CREATURE_MISSES = 456,
        CHAT_MSG_COMBAT_FRIENDLY_DEATH = 457,
        CHAT_MSG_COMBAT_HOSTILE_DEATH = 458,
        CHAT_MSG_COMBAT_XP_GAIN = 459,
        CHAT_MSG_COMBAT_HONOR_GAIN = 460,
        CHAT_MSG_SPELL_SELF_DAMAGE = 461,
        CHAT_MSG_SPELL_SELF_BUFF = 462,
        CHAT_MSG_SPELL_PET_DAMAGE = 463,
        CHAT_MSG_SPELL_PET_BUFF = 464,
        CHAT_MSG_SPELL_PARTY_DAMAGE = 465,
        CHAT_MSG_SPELL_PARTY_BUFF = 466,
        CHAT_MSG_SPELL_FRIENDLYPLAYER_DAMAGE = 467,
        CHAT_MSG_SPELL_FRIENDLYPLAYER_BUFF = 468,
        CHAT_MSG_SPELL_HOSTILEPLAYER_DAMAGE = 469,
        CHAT_MSG_SPELL_HOSTILEPLAYER_BUFF = 470,
        CHAT_MSG_SPELL_CREATURE_VS_SELF_DAMAGE = 471,
        CHAT_MSG_SPELL_CREATURE_VS_SELF_BUFF = 472,
        CHAT_MSG_SPELL_CREATURE_VS_PARTY_DAMAGE = 473,
        CHAT_MSG_SPELL_CREATURE_VS_PARTY_BUFF = 474,
        CHAT_MSG_SPELL_CREATURE_VS_CREATURE_DAMAGE = 475,
        CHAT_MSG_SPELL_CREATURE_VS_CREATURE_BUFF = 476,
        CHAT_MSG_SPELL_TRADESKILLS = 477,
        CHAT_MSG_SPELL_DAMAGESHIELDS_ON_SELF = 478,
        CHAT_MSG_SPELL_DAMAGESHIELDS_ON_OTHERS = 479,
        CHAT_MSG_SPELL_AURA_GONE_SELF = 480,
        CHAT_MSG_SPELL_AURA_GONE_PARTY = 481,
        CHAT_MSG_SPELL_AURA_GONE_OTHER = 482,
        CHAT_MSG_SPELL_ITEM_ENCHANTMENTS = 483,
        CHAT_MSG_SPELL_BREAK_AURA = 484,
        CHAT_MSG_SPELL_PERIODIC_SELF_DAMAGE = 485,
        CHAT_MSG_SPELL_PERIODIC_SELF_BUFFS = 486,
        CHAT_MSG_SPELL_PERIODIC_PARTY_DAMAGE = 487,
        CHAT_MSG_SPELL_PERIODIC_PARTY_BUFFS = 488,
        CHAT_MSG_SPELL_PERIODIC_FRIENDLYPLAYER_DAMAGE = 489,
        CHAT_MSG_SPELL_PERIODIC_FRIENDLYPLAYER_BUFFS = 490,
        CHAT_MSG_SPELL_PERIODIC_HOSTILEPLAYER_DAMAGE = 491,
        CHAT_MSG_SPELL_PERIODIC_HOSTILEPLAYER_BUFFS = 492,
        CHAT_MSG_SPELL_PERIODIC_CREATURE_DAMAGE = 493,
        CHAT_MSG_SPELL_PERIODIC_CREATURE_BUFFS = 494,
        CHAT_MSG_SPELL_FAILED_LOCALPLAYER = 495,
        CHAT_MSG_BG_SYSTEM_NEUTRAL = 496,
        CHAT_MSG_BG_SYSTEM_ALLIANCE = 497,
        CHAT_MSG_BG_SYSTEM_HORDE = 498,
        RAID_ROSTER_UPDATE = 499,
        UPDATE_PENDING_MAIL = 500,
        UPDATE_INVENTORY_ALERTS = 501,
        UPDATE_TRADESKILL_RECAST = 502,
        OPEN_MASTER_LOOT_LIST = 503,
        UPDATE_MASTER_LOOT_LIST = 504,
        START_LOOT_ROLL = 505,
        CANCEL_LOOT_ROLL = 506,
        CONFIRM_LOOT_ROLL = 507,
        INSTANCE_BOOT_START = 508,
        INSTANCE_BOOT_STOP = 509,
        LEARNED_SPELL_IN_TAB = 510,
        DISPLAY_SIZE_CHANGED = 511,
        CONFIRM_TALENT_WIPE = 512,
        CONFIRM_BINDER = 513,
        MAIL_FAILED = 514,
        CLOSE_INBOX_ITEM = 515,
        CONFIRM_SUMMON = 516,
        BILLING_NAG_DIALOG = 517,
        IGR_BILLING_NAG_DIALOG = 518,
        MEETINGSTONE_CHANGED = 519,
        PLAYER_SKINNED = 520,
        TABARD_SAVE_PENDING = 521,
        UNIT_QUEST_LOG_CHANGED = 522,
        PLAYER_PVP_KILLS_CHANGED = 523,
        PLAYER_PVP_RANK_CHANGED = 524,
        INSPECT_HONOR_UPDATE = 525,
        UPDATE_WORLD_STATES = 526,
        AREA_SPIRIT_HEALER_IN_RANGE = 527,
        AREA_SPIRIT_HEALER_OUT_OF_RANGE = 528,
        CONFIRM_PET_UNLEARN = 529,
        PLAYTIME_CHANGED = 530,
        UPDATE_LFG_TYPES = 531,
        UPDATE_LFG_LIST = 532,
        CHAT_MSG_COMBAT_FACTION_CHANGE = 533,
        START_MINIGAME = 534,
        MINIGAME_UPDATE = 535,
        READY_CHECK = 536,
        RAID_TARGET_UPDATE = 537,
        GMSURVEY_DISPLAY = 538,
        UPDATE_INSTANCE_INFO = 539,
        CHAT_MSG_RAID_BOSS_EMOTE = 541,
        COMBAT_TEXT_UPDATE = 542,
        LOTTERY_SHOW = 543,
        CHAT_MSG_FILTERED = 544,
        QUEST_WATCH_UPDATE = 545,
        CHAT_MSG_BATTLEGROUND = 546,
        CHAT_MSG_BATTLEGROUND_LEADER = 547,
        LOTTERY_ITEM_UPDATE = 548,

        // ADDED EVENTS
        SPELL_QUEUE_EVENT = 369,
        SPELL_CAST_EVENT = 540,
        SPELL_DAMAGE_EVENT_SELF = 549,
        SPELL_DAMAGE_EVENT_OTHER = 550,
    };

    enum TypeMask {
        TYPEMASK_OBJECT = 0x1,
        TYPEMASK_ITEM = 0x2,
        TYPEMASK_CONTAINER = 0x4,
        TYPEMASK_UNIT = 0x8,
        TYPEMASK_PLAYER = 0x10,
        TYPEMASK_GAMEOBJECT = 0x20,
        TYPEMASK_DYNAMICOBJECT = 0x40,
        TYPEMASK_CORPSE = 0x80,
    };

    enum SpellModOp {
        SPELLMOD_DAMAGE = 0,
        SPELLMOD_DURATION = 1,
        SPELLMOD_THREAT = 2,
        SPELLMOD_ATTACK_POWER = 3,
        SPELLMOD_CHARGES = 4,
        SPELLMOD_RANGE = 5,
        SPELLMOD_RADIUS = 6,
        SPELLMOD_CRITICAL_CHANCE = 7,
        SPELLMOD_ALL_EFFECTS = 8,
        SPELLMOD_NOT_LOSE_CASTING_TIME = 9,
        SPELLMOD_CASTING_TIME = 10,
        SPELLMOD_COOLDOWN = 11,
        SPELLMOD_SPEED = 12,
        SPELLMOD_COST = 14,
        SPELLMOD_CRIT_DAMAGE_BONUS = 15,
        SPELLMOD_RESIST_MISS_CHANCE = 16,
        SPELLMOD_JUMP_TARGETS = 17,
        SPELLMOD_CHANCE_OF_SUCCESS = 18,                   // Only used with SPELL_AURA_ADD_FLAT_MODIFIER and affects proc spells
        SPELLMOD_ACTIVATION_TIME = 19,
        SPELLMOD_EFFECT_PAST_FIRST = 20,
        SPELLMOD_CASTING_TIME_OLD = 21,
        SPELLMOD_DOT = 22,
        SPELLMOD_HASTE = 23,
        SPELLMOD_SPELL_BONUS_DAMAGE = 24,
        SPELLMOD_MULTIPLE_VALUE = 27,
        SPELLMOD_RESIST_DISPEL_CHANCE = 28,
        MAX_SPELLMOD = 29,
    };


    enum OBJECT_TYPE_ID : __int32 {
        ID_OBJECT = 0x0,
        ID_ITEM = 0x1,
        ID_CONTAINER = 0x2,
        ID_UNIT = 0x3,
        ID_PLAYER = 0x4,
        ID_GAMEOBJECT = 0x5,
        ID_DYNAMICOBJECT = 0x6,
        ID_CORPSE = 0x7,
        NUM_CLIENT_OBJECT_TYPES = 0x8,
        ID_AIGROUP = 0x8,
        ID_AREATRIGGER = 0x9,
        NUM_OBJECT_TYPES = 0xA,
    };

    class CDuration {
    public:
        char m_DurationIndex;                //0x0000
        __int32 m_Duration;                    //0x0004
        char unknown[4];                    //0x0008
        __int32 m_Duration2;                    //0x000C

        __int32 GetDuration() {
            return ((m_Duration / 1000) / 60);
        }
    };//Size=0x0010

    class CSpellCastingTime {
    public:
        __int32 m_CastingTimeIndex;                //0x0000
        __int32 m_CastTime;                    //0x0004
        char m_0x0008[4];                    //0x0008
        __int32 m_CastTime2;                    //0x000C

    };//Size=0x0010

    uintptr_t *GetObjectPtr(std::uint64_t guid);

    std::uint32_t GetCastTime(void *unit, uint32_t spellId);

    CDuration *GetDurationObject(uint32_t durationIndex);

    int GetSpellDuration(const SpellRec *spellRec, bool ignoreModifiers);

    int GetSpellModifier(const SpellRec *spellRec, SpellModOp spellMod);

    const SpellRec *GetSpellInfo(uint32_t spellId);

    uint32_t GetItemId(CGItem_C *item);

    const char *GetSpellName(uint32_t spellId);

    std::uint64_t ClntObjMgrGetActivePlayerGuid();

    std::uint64_t GetCurrentTargetGuid();

    uintptr_t *ClntObjMgrObjectPtr(TypeMask typeMask, std::uint64_t guid);
}