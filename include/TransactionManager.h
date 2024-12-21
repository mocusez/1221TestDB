#pragma once
#include "Transaction.h"
#include <unordered_map>
#include <mutex>

class TransactionManager {
public:
    static TransactionManager& getInstance() {
        static TransactionManager instance;
        return instance;
    }

    Transaction* begin() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto txn = new Transaction();
        active_txns_[txn->getTxnId()] = txn;
        return txn;
    }

    void commit(Transaction* txn) {
        std::lock_guard<std::mutex> lock(mutex_);
        active_txns_.erase(txn->getTxnId());
        delete txn;
    }

private:
    TransactionManager() = default;
    std::mutex mutex_;
    std::unordered_map<uint64_t, Transaction*> active_txns_;
};