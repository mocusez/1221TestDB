#pragma once
#include "Operator.h"
#include <vector>
#include <mutex>

template<typename T>
class Consumer : public Operator<T> {
public:
    void consume(const T& row) override {
        std::lock_guard<std::mutex> lock(mutex_);
        results_.push_back(row);
    }

    const std::vector<T>& getResults() const { return results_; }

private:
    std::vector<T> results_;
    std::mutex mutex_;
};