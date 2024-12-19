#include "ScanOperator.h"
#include "FilterOperator.h"

ScanOperator::ScanOperator(const std::vector<Row>& data) : data(data), index(0), consumer(nullptr) {}

void ScanOperator::setConsumer(FilterOperator* consumer) {
    this->consumer = consumer;
}

void ScanOperator::open() {
    index = 0;
}

void ScanOperator::next() {
    while (index < data.size()) {
        Row row = data[index++];
        if (consumer) {
            consumer->consume(row);
        }
    }
}

void ScanOperator::close() {
    index = 0;
}