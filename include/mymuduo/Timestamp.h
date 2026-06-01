#pragma once

#include <iostream>
#include <string>

class Timestamp
{
public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    static Timestamp now();
    std::string toString() const;
    int64_t microSecondsSinceEpoch() const {
        return microSecondsSinceEpoch_;
    }

    bool operator<(const Timestamp& c) const{
        return this->microSecondsSinceEpoch() < c.microSecondsSinceEpoch();
    }


private:
    int64_t microSecondsSinceEpoch_;
};