#pragma once
#include "Row.h"
#include <vector>

struct UndoEntry {
    size_t row_index; 
    int original_value;
    uint64_t original_end_ts;
};

class UndoLog {
public:
    void recordUpdate(size_t index,MVCCRow* row) {
        undoEntries.push_back({
            index,
            row->getValue(),
            row->getEndTs()
        });
    }

    const std::vector<UndoEntry>& getEntries() const {
        return undoEntries;
    }

private:
    std::vector<UndoEntry> undoEntries;
};