#include "ScanOperator.h"
#include "FilterOperator.h"
#include "Consumer.h"
#include "TransactionManager.h"
#include <iostream>

void printResults(const std::vector<Row>& results, const char* prefix) {
    std::cout << prefix << " Results:\n";
    for (const auto& row : results) {
        std::cout << "id=" << row.id << ", value=" << row.value 
                 << " (begin_ts=" << row.begin_ts 
                 << ", end_ts=" << row.end_ts 
                 << ", txn_id=" << row.txn_id << ")\n";
    }
    std::cout << "-------------------\n";
}

int main() {
    // Initial data
    std::vector<Row> data = {
        {1, 10, 1, 0, 0},  // id, value, begin_ts, end_ts, txn_id
        {2, 20, 1, 0, 0},
        {3, 30, 1, 0, 0}
    };

    auto& txnManager = TransactionManager::getInstance();

    // Transaction 1: Read initial state
    auto txn1 = txnManager.begin();
    auto predicate1 = [](const Row& row) { return true; };
    
    ScanOperator scan1(data, 2, txn1);
    FilterOperator filter1(predicate1);
    Consumer consumer1;
    
    scan1.setNextOperator(&filter1);
    filter1.setNextOperator(&consumer1);
    scan1.execute(2);
    
    // Transaction 2: Update some values
    auto txn2 = txnManager.begin();
    data[1].value = 25;    // Update row 2
    data[1].end_ts = txn2->getBeginTs();
    
    // Add new version
    data.push_back({2, 25, txn2->getBeginTs(), 0, txn2->getTxnId()});
    
    // Read from Transaction 1 (should see old values)
    printResults(consumer1.getResults(), "Txn1 (before Txn2 commit)");
    
    // Commit Transaction 2
    txnManager.commit(txn2);
    
    // New transaction to see updated values
    auto txn3 = txnManager.begin();
    auto predicate3 = [](const Row& row) { return true; };
    
    ScanOperator scan3(data, 2, txn3);
    FilterOperator filter3(predicate3);
    Consumer consumer3;
    
    scan3.setNextOperator(&filter3);
    filter3.setNextOperator(&consumer3);
    scan3.execute(2);
    
    printResults(consumer3.getResults(), "Txn3 (after Txn2 commit)");
    
    // Cleanup
    txnManager.commit(txn1);
    txnManager.commit(txn3);
    
    return 0;
}