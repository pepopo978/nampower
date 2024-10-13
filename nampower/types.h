//
// Created by pmacc on 9/25/2024.
//

#pragma once

#include <minwindef.h>
#include <cstdint>

struct UserSettings {
    bool queueCastTimeSpells;
    bool queueInstantSpells;
    bool queueOnSwingSpells;
    bool queueChannelingSpells;
    bool queueTargetingSpells;

    bool retryServerRejectedSpells;
    bool quickcastTargetingSpells;
    bool replaceMatchingNonGcdCategory;

    uint32_t spellQueueWindowMs;
    uint32_t onSwingBufferCooldownMs;
    uint32_t channelQueueWindowMs;
    uint32_t targetingQueueWindowMs;

    uint32_t minBufferTimeMs;
    uint32_t maxBufferIncreaseMs;
    uint32_t nonGcdBufferTimeMs;

};

enum CastType {
    NORMAL,
    NON_GCD,
    ON_SWING,
    CHANNEL,
    TARGETING,
    TARGETING_NON_GCD
};

enum QueueEvents {
    ON_SWING_QUEUED,
    ON_SWING_QUEUE_POPPED,
    NORMAL_QUEUED,
    NORMAL_QUEUE_POPPED,
    NON_GCD_QUEUED,
    NON_GCD_QUEUE_POPPED,
    QUEUE_EVENT_COUNT // Keep track of the number of events
};

struct CastSpellParams {
    /* Original cast spell function arguments */
    uint32_t *playerUnit;
    uint32_t spellId;
    void *item;
    uint64_t guid;
    /* *********************** */

    /* Additional data */
    uint32_t gcDCategory; // comes from spell->StartRecoveryCategory
    uint32_t castTimeMs;
    uint32_t castStartTimeMs;
    CastType castType;
    uint32_t numRetries;
};

struct LastCastData {
    uint32_t castTimeMs;  // spell's cast time in ms

    uint32_t startTimeMs;  // event time in ms
    uint32_t channelStartTimeMs; // event time in ms
    uint32_t onSwingStartTimeMs; // event time in ms

    bool wasOnGcd;
    bool wasQueued;
};

struct CastData {
    uint32_t delayEndMs; // can't be reset by looking at CastingSpellId
    uint32_t castEndMs;
    uint32_t gcdEndMs;
    uint32_t attemptedCastTimeMs; // this ignoring on swing spells as they are independent

    bool onSwingQueued;

    bool normalSpellQueued;
    bool nonGcdSpellQueued;

    bool castingQueuedSpell;
    uint32_t numRetries;

    bool cancellingSpell;
    bool channeling;
    uint32_t channelDuration;
    uint32_t channelCastCount;
};

