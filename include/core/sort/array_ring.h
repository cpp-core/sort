// Copyright (C) 2022, 2023 by Mark Melton
//

#pragma once
#include <algorithm>

namespace core::sort {

template<class T, size_t Size, size_t Align>
class ArrayRing {
public:
    auto size() const { return head_ - tail_; }
    auto& next() { return data_[head_ % Size]; }
    auto& front() { return data_[tail_ % Size]; }
    const auto& front() const { return data_[tail_ % Size]; }
    auto& back() { return data_[(head_ + Size - 1) % Size]; }
    const auto& back() const { return data_[(head_ + Size - 1) % Size]; }
    auto& head_index() { return head_; }
    const auto& head_index() const { return head_; }
    auto& tail_index() { return tail_; }
    const auto& tail_index() const { return tail_; }
    T pop_front() { return data_[tail_++ % Size]; }
    void push_back(const T& value) { data_[head_++ % Size] = value; }
    T pop_back() { return data_[--head_ % Size]; }
private:
    alignas(Align) std::array<T, Size> data_;
    size_t head_{}, tail_{};
};



}; // core::sort
