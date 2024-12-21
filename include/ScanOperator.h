#ifndef SCAN_OPERATOR_H
#define SCAN_OPERATOR_H

#include <vector>
#include <functional>

struct Row {
    int id;
    int value;
};

class FilterOperator;

class ScanOperator {
public:
    ScanOperator(const std::vector<Row>& data);
    void setConsumer(FilterOperator* consumer);
    void open();
    void next();
    void close();

private:
    const std::vector<Row>& data;
    std::size_t index;
    FilterOperator* consumer;
};

#endif // SCAN_OPERATOR_H