//
// Created by pmacc on 9/26/2024.
//

#pragma once


#include "types.h"
#include "logging.hpp"

namespace Nampower {
    class CastQueue {
    private:
        static const int MAX_SIZE = 5;
        CastSpellParams queue[MAX_SIZE]{};
        int front;
        int rear;
        int size;

    public:
        CastQueue() : front(0), rear(-1), size(0) {}

        bool isFull() const {
            return size == MAX_SIZE;
        }

        bool isEmpty() const {
            return size == 0;
        }

        void pushFront(const CastSpellParams &params) {
            if (isFull()) {
                // Shift all elements one position to the right
                for (int i = size - 1; i > 0; --i) {
                    queue[(front + i) % MAX_SIZE] = queue[(front + i - 1) % MAX_SIZE];
                }
                queue[front] = params;
                // rear doesn't change as we're overwriting the last element
            } else {
                front = (front - 1 + MAX_SIZE) % MAX_SIZE;
                queue[front] = params;
                if (size == 0) {
                    rear = front;
                }
                size++;
            }
        }

        void push(const CastSpellParams &params, bool replaceMatchingNonGcdCategory) {
            // if replaceMatchingNonGcdCategory is on
            // first check if this is nonGcd, and we already have a nonGcd spell in the queue with the same gcdCategory
            if (replaceMatchingNonGcdCategory && params.castType == CastType::NON_GCD) {
                auto nonGcdParams = findGcdCategory(params.gcDCategory);
                if (nonGcdParams) {
                    DEBUG_LOG("Replacing queued nonGcd spell " << game::GetSpellName(nonGcdParams->spellId) << " with "
                                                               << game::GetSpellName(params.spellId)
                                                               << " for gcdCategory " << params.gcDCategory);
                    *nonGcdParams = params;  // replace the existing nonGcd spell with the new one
                    return;
                }
            }
            if (isFull()) {
                // Replace the oldest element
                front = (front + 1) % MAX_SIZE;
                // Note: We don't need to decrease size here as it remains MAX_SIZE
            } else {
                size++;
            }
            rear = (rear + 1) % MAX_SIZE;
            queue[rear] = params;
        }

        CastSpellParams pop() {
            if (isEmpty()) {
                return CastSpellParams{};
            }
            CastSpellParams result = queue[front];
            front = (front + 1) % MAX_SIZE;
            size--;
            return result;
        }

        CastSpellParams *peek() {
            if (isEmpty()) {
                return nullptr;
            }
            return &queue[front];
        }

        CastSpellParams *findSpellId(uint32_t spellId) {
            for (int i = 0; i < size; i++) {
                int index = (front + i) % MAX_SIZE;
                if (queue[index].spellId == spellId) {
                    return &queue[index];
                }
            }
            return nullptr;
        }

        CastSpellParams *findGcdCategory(uint32_t gcdCategory) {
            for (int i = 0; i < size; i++) {
                int index = (front + i) % MAX_SIZE;
                if (queue[index].gcDCategory == gcdCategory) {
                    return &queue[index];
                }
            }
            return nullptr;
        }

        int getSize() const {
            return size;
        }
    };
}