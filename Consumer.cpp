#include "Consumer.h"

void Consumer::consume(const Row& row) {
    results.push_back(row);
}

const std::vector<Row>& Consumer::getResults() const {
    return results;
}