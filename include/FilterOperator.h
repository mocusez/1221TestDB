#pragma once
#include "Operator.h"
#include <functional>

template<typename T>
class FilterOperator : public Operator<T> {
public:
    explicit FilterOperator(std::function<bool(const T&)> predicate) 
        : predicate_(predicate), nextOperator_(nullptr) {}

    void setNextOperator(Operator<T>* next) { nextOperator_ = next; }

    void consume(const T& row) override {
        if (predicate_(row) && nextOperator_) {
            nextOperator_->consume(row);
        }
    }

private:
    std::function<bool(const T&)> predicate_;
    Operator<T>* nextOperator_;
};