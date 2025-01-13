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
    bool queueSpellsOnCooldown;

    bool interruptChannelsOutsideQueueWindow;
    bool retryServerRejectedSpells;
    bool quickcastTargetingSpells;
    bool replaceMatchingNonGcdCategory;
    bool optimizeBufferUsingPacketTimings;

    bool preventRightClickTargetChange;

    bool doubleCastToEndChannelEarly;

    uint32_t spellQueueWindowMs;
    uint32_t onSwingBufferCooldownMs;
    uint32_t channelQueueWindowMs;
    uint32_t targetingQueueWindowMs;
    uint32_t cooldownQueueWindowMs;

    uint32_t minBufferTimeMs;
    uint32_t maxBufferIncreaseMs;
    uint32_t nonGcdBufferTimeMs;

    int32_t channelLatencyReductionPercentage;
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

enum CastResult {
    WAITING_FOR_CAST,
    WAITING_FOR_SERVER,
    SERVER_SUCCESS,
    SERVER_FAILURE
};

struct CastSpellParams {
    /* Original cast spell function arguments */
    uint32_t *playerUnit;
    uint32_t spellId;
    uintptr_t *item;
    uint64_t guid;
    /* *********************** */

    /* Additional data */
    uint32_t gcDCategory; // comes from spell->StartRecoveryCategory
    uint32_t castTimeMs; // spell's cast time in ms
    uint32_t castStartTimeMs; // event time in ms
    CastType castType;
    uint32_t numRetries;
    CastResult castResult;
};

struct LastCastData {
    uint32_t attemptTimeMs;  // last cast attempt time in ms
    uint32_t attemptSpellId; // last cast attempt spell id

    uint32_t castTimeMs;  // spell's cast time in ms

    uint32_t startTimeMs;  // event time in ms
    uint32_t channelStartTimeMs; // event time in ms
    uint32_t onSwingStartTimeMs; // event time in ms

    bool wasItem;
    bool wasOnGcd;
    bool wasQueued;
};

struct CastData {
    uint32_t delayEndMs; // can't be reset by looking at CastingSpellId
    uint32_t castEndMs;
    uint32_t gcdEndMs;
    uint32_t attemptedCastTimeMs; // this ignoring on swing spells as they are independent

    bool onSwingQueued;
    bool pendingOnSwingCast;
    uint32_t onSwingSpellId;

    uint32_t cooldownNormalEndMs;
    uint32_t cooldownNonGcdEndMs;
    bool cooldownNormalSpellQueued;
    bool cooldownNonGcdSpellQueued;

    bool normalSpellQueued;
    bool nonGcdSpellQueued;

    bool castingQueuedSpell;
    uint32_t numRetries;

    bool cancellingSpell;

    bool channeling;
    bool cancelChannelNextTick;
    uint32_t channelStartMs;
    uint32_t channelEndMs;
    uint32_t channelTickTimeMs;
    uint32_t channelNumTicks;
    uint32_t channelSpellId;
    uint32_t channelDuration;
};

