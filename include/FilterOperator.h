#pragma once
#include "Operator.h"
#include <functional>

class FilterOperator : public Operator {
public:
    explicit FilterOperator(std::function<bool(const Row&)> predicate) 
        : predicate_(predicate), nextOperator_(nullptr) {}

    void setNextOperator(Operator* next) { nextOperator_ = next; }

    void consume(const Row& row) override {
        if (predicate_(row) && nextOperator_) {
            nextOperator_->consume(row);
        }
    }

private:
    std::function<bool(const Row&)> predicate_;
    Operator* nextOperator_;
};