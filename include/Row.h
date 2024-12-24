#pragma once
#include <cstdint>
#include <algorithm>
#include <vector>

class Row {
private:
    int id;
    int value;
public:
    Row() : id(0), value(0) {}
    Row(int i, int v) : id(i), value(v) {}
    virtual ~Row() {}

    int getId() const { return id; }
    int getValue() const { return value; }
    void setValue(int v) { value = v; }
};

class MVCC {
private:
    uint64_t begin_ts;
    uint64_t end_ts;
    uint64_t txn_id;
public:
    MVCC(): begin_ts(1), end_ts(0), txn_id(0) {}
    MVCC(uint64_t begin_ts, uint64_t end_ts, uint64_t txn_id): begin_ts(begin_ts), end_ts(end_ts), txn_id(txn_id) {}
    virtual ~MVCC() {}

    uint64_t getBeginTs() const { return begin_ts; }
    uint64_t getEndTs() const { return end_ts; }
    uint64_t getTxnId() const { return txn_id; }
    
    void setBeginTs(uint64_t ts) { begin_ts = ts; }
    void setEndTs(uint64_t ts) { end_ts = ts; }
    void setTxnId(uint64_t tid) { txn_id = tid; }
};

class OptimisticLockColumn{
private:
    uint64_t version;
public:
    OptimisticLockColumn() : version(1){}
    uint64_t getVersion() const {
        return version;
    }

    void updateVersion() {
        version++;
    }
    virtual ~OptimisticLockColumn() {}
};


class MVCCRow : public Row, public MVCC {
public:
    MVCCRow() : Row(), MVCC() {}
    MVCCRow(int i, int v) : Row(i, v), MVCC() {}
    MVCCRow(int i, int v,uint64_t begin_ts, uint64_t end_ts, uint64_t txn_id) : Row(i, v), MVCC(begin_ts,end_ts,txn_id) {}
    explicit MVCCRow(const Row& row) : Row(row),MVCC() {}
    static std::vector<MVCCRow> convertRowToMVCCRows(const std::vector<Row>& rows) {
        std::vector<MVCCRow> mvccRows;
        mvccRows.reserve(rows.size());
        
        std::transform(rows.begin(), rows.end(), 
                    std::back_inserter(mvccRows),
                    [](const Row& row) {
                        return MVCCRow(row);
                    });
        return mvccRows;
    }
    virtual ~MVCCRow() override {}
};

class OptimisticLockRow : public Row, public OptimisticLockColumn {
public:
    OptimisticLockRow(int i, int v) : Row(i,v), OptimisticLockColumn() {}        
    virtual ~OptimisticLockRow() = default;

    explicit OptimisticLockRow(const Row& row) : Row(row),OptimisticLockColumn() {}
    static std::vector<OptimisticLockRow> convertRowToOptimisticLockRows(const std::vector<Row>& rows) {
        std::vector<OptimisticLockRow> OplRows;
        OplRows.reserve(rows.size());
        
        std::transform(rows.begin(), rows.end(), 
                    std::back_inserter(OplRows),
                    [](const Row& row) {
                        return OptimisticLockRow(row);
                    });
        return OplRows;
    }
};

