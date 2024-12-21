#ifndef FILTER_OPERATOR_H
#define FILTER_OPERATOR_H

#include <functional>
#include "ScanOperator.h"

class Consumer;

class FilterOperator {
public:
    FilterOperator(std::function<bool(const Row&)> predicate);
    void setConsumer(Consumer* consumer);
    void consume(const Row& row);

private:
    std::function<bool(const Row&)> predicate;
    Consumer* consumer;
};

#endif // FILTER_OPERATOR_H