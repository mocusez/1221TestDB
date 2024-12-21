#pragma once
#include "Row.h"
#include <mutex>
#include <algorithm>

struct Morsel {
    size_t start;
    size_t end;
};

class MorselQueue {
public:
    MorselQueue(size_t dataSize, size_t morselSize) 
        : dataSize_(dataSize), morselSize_(morselSize), nextIndex_(0) {}

    bool getNextMorsel(Morsel& morsel) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (nextIndex_ >= dataSize_) return false;
        morsel.start = nextIndex_;
        morsel.end = std::min(nextIndex_ + morselSize_, dataSize_);
        nextIndex_ = morsel.end;
        return true;
    }

private:
    size_t dataSize_;
    size_t morselSize_;
    size_t nextIndex_;
    std::mutex mutex_;
};