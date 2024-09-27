//
// Created by pmacc on 9/26/2024.
//

#pragma once


#include "types.h"

class CastQueue {
private:
    static const int MAX_SIZE = 5;
    CastSpellParams queue[MAX_SIZE];
    int front;
    int rear;
    int size;

public:
    CastQueue() : front(0), rear(-1), size(0) {}

    // Check if the queue is full
    bool isFull() const {
        return size == MAX_SIZE;
    }

    // Check if the queue is empty
    bool isEmpty() const {
        return size == 0;
    }

    // Add a CastSpellParams to the queue
    bool push(const CastSpellParams& params) {
        if (isFull()) {
            return false;
        }
        rear = (rear + 1) % MAX_SIZE;
        queue[rear] = params;
        size++;
        return true;
    }

    // Remove a CastSpellParams from the queue
    bool pop() {
        if (isEmpty()) {
            return false;
        }
        front = (front + 1) % MAX_SIZE;
        size--;
        return true;
    }

    // Get the front element of the queue
    CastSpellParams* peek() {
        if (isEmpty()) {
            return nullptr;
        }
        return &queue[front];
    }

    // Get the current size of the queue
    int getSize() const {
        return size;
    }
};