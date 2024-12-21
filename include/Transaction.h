#pragma once
#include <atomic>

class Transaction {
public:
    static uint64_t getNextTxnId() {
        static std::atomic<uint64_t> next_txn_id{1};
        return next_txn_id.fetch_add(1);
    }

    Transaction() : txn_id_(getNextTxnId()), begin_ts_(getNextTxnId()) {}
    
    uint64_t getTxnId() const { return txn_id_; }
    uint64_t getBeginTs() const { return begin_ts_; }

private:
    uint64_t txn_id_;
    uint64_t begin_ts_;
};