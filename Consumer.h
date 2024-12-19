#ifndef CONSUMER_H
#define CONSUMER_H

#include <vector>
#include "ScanOperator.h"

class Consumer {
public:
    void consume(const Row& row);
    const std::vector<Row>& getResults() const;

private:
    std::vector<Row> results;
};

#endif // CONSUMER_H