#pragma once
#include "Operator.h"
#include <vector>
#include <mutex>

class Consumer : public Operator {
public:
    void consume(const Row& row) override {
        std::lock_guard<std::mutex> lock(mutex_);
        results_.push_back(row);
    }

    const std::vector<Row>& getResults() const { return results_; }

private:
    std::vector<Row> results_;
    std::mutex mutex_;
};