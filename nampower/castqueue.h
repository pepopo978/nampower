#pragma once

#include "types.h"
#include "logging.hpp"
#include <vector>

namespace Nampower {
    class CastQueue {
    private:
        int maxSize;
        std::vector<CastSpellParams> queue;
        int front;
        int rear;
        int size;

    public:
        explicit CastQueue(int maxSize)
                : maxSize(maxSize), queue(maxSize), front(0), rear(-1), size(0) {}

        bool isFull() const {
            return size == maxSize;
        }

        bool isEmpty() const {
            return size == 0;
        }

        void clear() {
            front = 0;
            rear = -1;
            size = 0;
        }

        void pushFront(const CastSpellParams &params) {
            if (isFull()) {
                // Shift all elements one position to the right
                for (int i = size - 1; i > 0; --i) {
                    queue[(front + i) % maxSize] = queue[(front + i - 1) % maxSize];
                }
                queue[front] = params;
            } else {
                front = (front - 1 + maxSize) % maxSize;
                queue[front] = params;
                if (size == 0) {
                    rear = front;
                }
                size++;
            }
        }

        void push(const CastSpellParams &params, bool replaceMatchingNonGcdCategory) {
            if (replaceMatchingNonGcdCategory && params.castType == CastType::NON_GCD && params.gcDCategory != 0) {
                auto nonGcdParams = findGcdCategory(params.gcDCategory);
                if (nonGcdParams) {
                    DEBUG_LOG("Replacing queued nonGcd spell " << game::GetSpellName(nonGcdParams->spellId) << " with "
                                                               << game::GetSpellName(params.spellId)
                                                               << " for gcdCategory " << params.gcDCategory);
                    *nonGcdParams = params;
                    return;
                }
            }
            if (isFull()) {
                front = (front + 1) % maxSize;
            } else {
                size++;
            }
            rear = (rear + 1) % maxSize;
            queue[rear] = params;
        }

        CastSpellParams pop() {
            if (isEmpty()) {
                return CastSpellParams{};
            }
            CastSpellParams result = queue[front];
            front = (front + 1) % maxSize;
            size--;
            return result;
        }

        CastSpellParams *peek() {
            if (isEmpty()) {
                return nullptr;
            }
            return &queue[front];
        }

        CastSpellParams *findSpellIdWithMaxStartTime(uint32_t spellId, uint32_t maxStartTimeMs) {
            for (int i = 0; i < size; i++) {
                int index = (front + i) % maxSize;
                if (queue[index].spellId == spellId && queue[index].castStartTimeMs < maxStartTimeMs) {
                    return &queue[index];
                }
            }
            return nullptr;
        }

        CastSpellParams *findSpellId(uint32_t spellId) {
            for (int i = 0; i < size; i++) {
                int index = (front + i) % maxSize;
                if (queue[index].spellId == spellId) {
                    return &queue[index];
                }
            }
            return nullptr;
        }

        CastSpellParams *findOldestWaitingForServerSpellId(uint32_t spellId) {
            for (int i = size - 1; i >= 0; i--) {
                int index = (front + i) % maxSize;
                if (queue[index].spellId == spellId && queue[index].castResult == CastResult::WAITING_FOR_SERVER) {
                    return &queue[index];
                }
            }
            return nullptr;
        }

        CastSpellParams *findNewestWaitingForServerSpellId(uint32_t spellId) {
            for (int i = 0; i < size; i++) {
                int index = (front + i) % maxSize;
                if (queue[index].spellId == spellId && queue[index].castResult == CastResult::WAITING_FOR_SERVER) {
                    return &queue[index];
                }
            }
            return nullptr;
        }

        CastSpellParams *findNewestSuccessfulSpellId(uint32_t spellId) {
            for (int i = 0; i < size; i++) {
                int index = (front + i) % maxSize;
                if (queue[index].spellId == spellId && queue[index].castResult == CastResult::SERVER_SUCCESS) {
                    return &queue[index];
                }
            }
            return nullptr;
        }

        CastSpellParams *findGcdCategory(uint32_t gcdCategory) {
            for (int i = 0; i < size; i++) {
                int index = (front + i) % maxSize;
                if (queue[index].gcDCategory == gcdCategory) {
                    return &queue[index];
                }
            }
            return nullptr;
        }

        void logHistory() {
            for (int i = 0; i < size; i++) {
                int index = (front + i) % maxSize;
                DEBUG_LOG("Cast history " << i << ": " << game::GetSpellName(queue[index].spellId) << " result "
                                          << queue[index].castResult);
            }
        }

        int getSize() const {
            return size;
        }

        int getMaxSize() const {
            return maxSize;
        }
    };
}
