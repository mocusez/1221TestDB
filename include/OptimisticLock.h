#pragma once
#include <map>
#include <vector>
#include <mutex>
#include "Row.h"

class OptimisticLock {
private:    
    std::mutex mtx;

public:
    OptimisticLock() {}

    bool update(size_t key, int newValue, int expectedVersion,std::vector<OptimisticLockRow>& data) {
        std::lock_guard<std::mutex> lock(mtx);
        
        data.at(key).setValue(newValue);
        data.at(key).updateVersion();
        return true;
    }

    std::pair<int, int> get(size_t key,std::vector<OptimisticLockRow>& data) {
        return {data.at(key).getValue(), data.at(key).getVersion()};
    }
};