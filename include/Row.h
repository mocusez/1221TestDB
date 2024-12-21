#pragma once
#include <cstdint>
struct Row {
    int id;
    int value;
    uint64_t begin_ts;  // Transaction start timestamp
    uint64_t end_ts;    // Transaction end timestamp
    uint64_t txn_id;    // Transaction ID that created this version
};