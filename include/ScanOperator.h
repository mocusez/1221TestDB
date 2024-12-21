#pragma once
#include "Operator.h"
#include "MorselQueue.h"
#include <vector>
#include <thread>

class ScanOperator {
public:
    ScanOperator(const std::vector<Row>& data, size_t morselSize = 1)
        : data_(data), morselQueue_(data.size(), morselSize), nextOperator_(nullptr) {}
    
    
    void setNextOperator(Operator* next) { nextOperator_ = next; }

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
                if (nextOperator_) {
                    nextOperator_->consume(data_[i]);
                }
            }
        }
    }

    const std::vector<Row>& data_;
    MorselQueue morselQueue_;
    Operator* nextOperator_;
};