#pragma once

#include "../SegmentedDeque.hpp"

template<class T>
class MutableSegmentedDeque : public SegmentedDequeBase<T> {
protected:
    MutableSegmentedDeque<T>* Clone() const override {
        return new MutableSegmentedDeque<T>(*this);
    }

    MutableSegmentedDeque<T>* Instance() override {
        return this;
    }

    MutableSegmentedDeque<T>* CreateEmptySameKind() const override {
        return new MutableSegmentedDeque<T>(this->blockCapacity, this->storageKind);
    }

public:
    explicit MutableSegmentedDeque(
        int blockCapacityValue = 8,
        DequeStorageKind kind = DequeStorageKind::ArraySequence
    )
        : SegmentedDequeBase<T>(blockCapacityValue, kind) {}

    MutableSegmentedDeque(
        const T* items,
        int count,
        int blockCapacityValue = 8,
        DequeStorageKind kind = DequeStorageKind::ArraySequence
    )
        : SegmentedDequeBase<T>(items, count, blockCapacityValue, kind) {}

    MutableSegmentedDeque(const MutableSegmentedDeque<T>& other)
        : SegmentedDequeBase<T>(other) {}
};
