#include "FilterOperator.h"
#include "Consumer.h"

FilterOperator::FilterOperator(std::function<bool(const Row&)> predicate) : predicate(predicate), consumer(nullptr) {}

void FilterOperator::setConsumer(Consumer* consumer) {
    this->consumer = consumer;
}

void FilterOperator::consume(const Row& row) {
    if (predicate(row)) {
        if (consumer) {
            consumer->consume(row);
        }
    }
}