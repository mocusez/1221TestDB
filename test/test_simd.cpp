#include <vector>
#include <iostream>
#include <algorithm>
#include "Row.h"
#include "TransactionManager.h"
#include "Simd.h"

int main() {
    std::vector<Row> data = {
        {1, 10, 1, 0, 0}, 
        {2, 20, 1, 0, 0},
        {4, 40, 1, 0, 0},
        {5, 50, 1, 0, 0},
        {6, 60, 1, 0, 0},
        {7, 70, 1, 0, 0},
        {8, 80, 1, 0, 0},
        {9, 90, 1, 0, 0},
        {10, 0, 1, 0, 0},
    };
    std::vector<int> values(data.size());
    std::transform(data.begin(), data.end(), values.begin(),
                  [](const Row& row) { return row.value; });

    auto& txnManager = TransactionManager::getInstance();
    auto txn1 = txnManager.begin();

    std::cout << "SUM: " << sum_simd(values) << '\n';
    std::cout << "AVG: " << avg_simd(values) << '\n';
    std::cout << "COUNT: " << count_nonzero_simd(values) << '\n';
    
    txnManager.commit(txn1);

    // Output the result
    return 0;
}
