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

    std::vector<Row> data2 = {
        {1, 10, 1, 0, 0}, 
        {2, 20, 1, 0, 0},
        {4, 30, 1, 0, 0},
        {5, 50, 1, 0, 0},
        {6, 60, 1, 0, 0},
        {7, 70, 1, 0, 0},
        {8, 80, 1, 0, 0},
        {9, 100, 1, 0, 0},
        {10, 0, 1, 0, 0},
    };
    std::vector<int> values2(data2.size());
    std::transform(data2.begin(), data2.end(), values2.begin(),
                  [](const Row& row) { return row.value; });

    auto& txnManager = TransactionManager::getInstance();
    auto txn1 = txnManager.begin();

    std::cout << "SUM: " << sum_simd(values) << '\n';
    std::cout << "AVG: " << avg_simd(values) << '\n';
    std::cout << "COUNT: " << count_nonzero_simd(values) << '\n';

    auto joined = inner_join_simd(values, values2);
    std::cout << "Inner Join Result: ";
    for (int num : joined) {
        std::cout << num << " ";
    }
    std::cout << '\n';

    std::vector<std::pair<int, int>> joined_left = join_simd(values, values2, JoinType::LEFT);
    std::cout << "Left Join Result:\n";
    for (const auto& p : joined_left) {
        std::cout << "(" << p.first << ", " << (p.second == -1 ? "NULL" : std::to_string(p.second)) << ")\n";
    }

    // // Call the SIMD-accelerated right join function and print result
    std::vector<std::pair<int, int>> joined_right = join_simd(values, values2, JoinType::RIGHT);
    std::cout << "\nRight Join Result:\n";
    for (const auto& p : joined_right) {
        std::cout << "(" << (p.first == -1 ? "NULL" : std::to_string(p.first)) << ", " << p.second << ")\n";
    }
    
    txnManager.commit(txn1);
    return 0;
}
