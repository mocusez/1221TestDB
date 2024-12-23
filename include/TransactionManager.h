#pragma once
#include "Transaction.h"
#include <unordered_map>
#include <mutex>
#include <vector>

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
        txn->setState(TxnState::COMMITTED);
        std::lock_guard<std::mutex> lock(mutex_);
        active_txns_.erase(txn->getTxnId());
        delete txn;
    }

    void rollback(Transaction* txn,std::vector<MVCCRow> &data) {
        std::lock_guard<std::mutex> lock(mutex_);
        txn->setState(TxnState::ABORTED);

        // Restore original values from undo log
        for (const auto& entry : txn->getUndoLog().getEntries()) {
            data.at(entry.row_index).setValue(entry.original_value);
            data.at(entry.row_index).setEndTs(entry.original_end_ts);
        }

        auto txnId = txn->getTxnId();
        data.erase(
            std::remove_if(data.begin(), data.end(),
                [txnId](const MVCCRow& row) {
                    return row.getTxnId() == txnId;
                }
            ),
            data.end()
        );
        active_txns_.erase(txnId);
        delete txn;
    }

private:
    TransactionManager() = default;
    std::mutex mutex_;
    std::unordered_map<uint64_t, Transaction*> active_txns_;
};