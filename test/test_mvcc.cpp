#include "ScanOperator.h"
#include "FilterOperator.h"
#include "Consumer.h"
#include "TransactionManager.h"
#include <iostream>

void printResults(const std::vector<MVCCRow>& results, const char* prefix) {
    std::cout << prefix << " Results:\n";
    for (const auto& row : results) {
        std::cout << "id=" << row.getId() << ", value=" << row.getValue() 
                << " (begin_ts=" << row.getBeginTs() 
                << ", end_ts=" << row.getEndTs() 
                << ", txn_id=" << row.getTxnId() << ")\n";
    }
    std::cout << "-------------------\n";
}

int testNormalMVCC(){
    // Initial data
    std::vector<MVCCRow> data = {
        {1, 10},  // id, value
        {2, 20},
        {3, 30}
    };

    auto& txnManager = TransactionManager::getInstance();

    // Transaction 1: Read initial state
    auto txn1 = txnManager.begin();
    auto predicate1 = [](const Row& row) { return true; };
    
    ScanOperator<MVCCRow> scan1(data, 2, txn1);
    FilterOperator<MVCCRow> filter1(predicate1);
    Consumer<MVCCRow> consumer1;
    
    scan1.setNextOperator(&filter1);
    filter1.setNextOperator(&consumer1);
    scan1.execute(2);
    
    // Transaction 2: Update some values
    auto txn2 = txnManager.begin();
    data[1].setValue(25);    // Update row 2
    data[1].setEndTs(txn2->getBeginTs());
    
    // Add new version
    data.push_back({2, 25, txn2->getBeginTs(), 0, txn2->getTxnId()});
    
    // Read from Transaction 1 (should see old values)
    printResults(consumer1.getResults(), "Txn1 (before Txn2 commit)");
    
    // Commit Transaction 2
    txnManager.commit(txn2);
    
    // New transaction to see updated values
    auto txn3 = txnManager.begin();
    auto predicate3 = [](const Row& row) { return true; };
    
    ScanOperator<MVCCRow> scan3(data, 2, txn3);
    FilterOperator<MVCCRow> filter3(predicate3);
    Consumer<MVCCRow> consumer3;
    
    scan3.setNextOperator(&filter3);
    filter3.setNextOperator(&consumer3);
    scan3.execute(2);
    
    printResults(consumer3.getResults(), "Txn3 (after Txn2 commit)");
    
    // Cleanup
    txnManager.commit(txn1);
    txnManager.commit(txn3);
    
    return 0;
}

void testMVCCRollback() {
    std::vector<MVCCRow> data = {
        {1, 10},
        {2, 20},
        {3, 30}
    };

    auto& txnManager = TransactionManager::getInstance();

    // T1: Initial read
    auto txn1 = txnManager.begin();
    auto predicate1 = [](const Row& row) { return true; };
    ScanOperator<MVCCRow> scan1(data, 2, txn1);
    FilterOperator<MVCCRow> filter1(predicate1);
    Consumer<MVCCRow> consumer1;
    
    scan1.setNextOperator(&filter1);
    filter1.setNextOperator(&consumer1);
    scan1.execute(2);
    printResults(consumer1.getResults(), "Initial state");

    // T2: Update and rollback
    auto txn2 = txnManager.begin();
    txn2->recordUpdate(1, &data[1]);

    data[1].setEndTs(txn2->getBeginTs());

    auto newVersion = MVCCRow{2, 25, txn2->getBeginTs(), 0, txn2->getTxnId()};
    data.push_back(newVersion);
    printResults(data, "After T2 Update (Before Rollback)");
    
    // Rollback T2
    txnManager.rollback(txn2, data);

    auto txn3 = txnManager.begin();
    auto predicate3 = [](const Row& row) { return true; };
    ScanOperator<MVCCRow> scan3(data, 2, txn3);
    FilterOperator<MVCCRow> filter3(predicate3);
    Consumer<MVCCRow> consumer3;
    
    scan3.setNextOperator(&filter3);
    filter3.setNextOperator(&consumer3);
    scan3.execute(2);
    printResults(consumer3.getResults(), "After rollback");

    txnManager.commit(txn1);
    txnManager.commit(txn3);
}

int main() {
    testMVCCRollback();
    testNormalMVCC();
    return 0;
}