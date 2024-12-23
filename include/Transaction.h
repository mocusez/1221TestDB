#pragma once
#include <atomic>
#include "UndoLog.h"

enum class TxnState {
    ACTIVE,
    COMMITTED,
    ABORTED
};

class Transaction {
public:
    static uint64_t getNextTxnId() {
        static std::atomic<uint64_t> next_txn_id{1};
        return next_txn_id.fetch_add(1);
    }

    Transaction() : txn_id_(getNextTxnId()), begin_ts_(getNextTxnId()), state_(TxnState::ACTIVE) {}
    
    void setState(TxnState state) { state_ = state; }

    void recordUpdate(size_t index, MVCCRow* row) {
        undoLog.recordUpdate(index,row);
    }

    bool isAborted() const { return this->state_ == TxnState::ABORTED; }

    UndoLog& getUndoLog() { return undoLog; }
    TxnState getState() const { return state_; }
    uint64_t getTxnId() const { return txn_id_; }
    uint64_t getBeginTs() const { return begin_ts_; }

private:
    uint64_t txn_id_;
    uint64_t begin_ts_;
    TxnState state_;
    UndoLog undoLog;
};