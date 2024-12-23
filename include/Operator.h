#pragma once
#include "Row.h"

template<typename T>
class Operator {
public:
    virtual ~Operator() = default;
    virtual void consume(const T& row) = 0;
};