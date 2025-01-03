#include "OptimisticLock.h"
#include "Row.h"
#include <thread>
#include <iostream>

int normalOptimisticLock(const std::vector<Row>& raw_data) {
    OptimisticLock lock;
    std::vector<OptimisticLockRow> data = OptimisticLockRow::convertRowToOptimisticLockRows(raw_data);

    auto [value, version] = lock.get(1,data);
    std::cout << "Initial value: " << value << ", version: " << version << std::endl;

    std::thread t1([&]() {
        bool success = lock.update(1, 15, 1, data);
        std::cout << "Thread 1 update: " << (success ? "success" : "failed") << std::endl;
    });

    std::thread t2([&]() {
        bool success = lock.update(1, 25, 1, data);
        std::cout << "Thread 2 update: " << (success ? "success" : "failed") << std::endl;
    });

    auto [newValue, newVersion] = lock.get(1,data);
    std::cout << "Running value: " << newValue << ", version: " << newVersion << std::endl;

    t1.join();
    t2.join();

    auto [newValue1, newVersion1] = lock.get(1,data);
    std::cout << "Final value: " << newValue1 << ", version: " << newVersion1 << std::endl;

    return 0;
}

int main(){
    std::vector<Row> data = {
        {1, 10},
        {2, 20},
        {3, 30}
    };
    normalOptimisticLock(data);
    return 0;
}

extern "C" {
    __attribute__((used))
    int jitMain(std::vector<Row> data){
        normalOptimisticLock(data);
        return 0;
    }
}
