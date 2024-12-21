#include <iostream>
#include <vector>
#include "ScanOperator.h"
#include "FilterOperator.h"
#include "Consumer.h"
#include "TransactionManager.h"

int main() {
    // Sample data
    std::vector<Row> data = {
        {1, 10, 1, 0, 0},  // id, value, begin_ts, end_ts, txn_id
        {2, 20, 1, 0, 0},
        {3, 30, 1, 0, 0}
    };

    auto& txnManager = TransactionManager::getInstance();
    auto txn = txnManager.begin();

    auto predicate = [](const Row& row) {
        return row.value > 15;
    };

    ScanOperator scan(data, 2, txn);
    FilterOperator filter(predicate);
    Consumer consumer;

    scan.setNextOperator(&filter);
    filter.setNextOperator(&consumer);
    scan.execute(2);

    txnManager.commit(txn);

    const auto& results = consumer.getResults();
    for (const auto& row : results) {
        std::cout << "Row: { id: " << row.id << ", value: " << row.value << " }\n";
    }

    return 0;
}