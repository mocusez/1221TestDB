#pragma once
#include "Operator.h"
#include "MorselQueue.h"
#include "Transaction.h"
#include <vector>
#include <thread>
#include <cstdio>

template<typename T>
class ScanOperator {
public:
    ScanOperator(const std::vector<T>& data, size_t morselSize, Transaction* txn)
        : data_(data), morselQueue_(data.size(), morselSize), 
          nextOperator_(nullptr), txn_(txn) {}

    void setNextOperator(Operator<T>* next) { nextOperator_ = next; }

    void execute(int numThreads = 4) {
        std::vector<std::thread> threads;
        for (int i = 0; i < numThreads; i++) {
            threads.emplace_back([this]() { processMorsels(); });
        }
        for (auto& thread : threads) {
            thread.join();
        }
    }

private:
    void processMorsels() {
        Morsel morsel;
        printf("Thread %lu processing\n", 
               std::hash<std::thread::id>{}(std::this_thread::get_id()));
        while (morselQueue_.getNextMorsel(morsel)) {
            for (size_t i = morsel.start; i < morsel.end; i++) {
                if (isVisible(data_[i]) && nextOperator_) {
                    nextOperator_->consume(data_[i]);
                }
            }
        }
    }
    bool isVisible(const MVCCRow& row) {
        return row.getBeginTs() <= txn_->getBeginTs() && 
            (row.getEndTs() == 0 || row.getEndTs() > txn_->getBeginTs());
    }

    const std::vector<T>& data_;
    MorselQueue morselQueue_;
    Operator<T>* nextOperator_;
    Transaction* txn_;
};