#pragma once
#include "Row.h"

class Operator {
public:
    virtual ~Operator() = default;
    virtual void consume(const Row& row) = 0;
};