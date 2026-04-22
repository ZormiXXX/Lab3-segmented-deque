#pragma once

#include "../SegmentedDeque.hpp"

template<class T>
class ImmutableSegmentedDeque : public SegmentedDequeBase<T> {
protected:
    ImmutableSegmentedDeque<T>* Clone() const override {
        return new ImmutableSegmentedDeque<T>(*this);
    }

    ImmutableSegmentedDeque<T>* Instance() override {
        return Clone();
    }

    ImmutableSegmentedDeque<T>* CreateEmptySameKind() const override {
        return new ImmutableSegmentedDeque<T>(this->blockCapacity, this->storageKind);
    }

public:
    explicit ImmutableSegmentedDeque(
        int blockCapacityValue = 8,
        DequeStorageKind kind = DequeStorageKind::ArraySequence
    )
        : SegmentedDequeBase<T>(blockCapacityValue, kind) {}

    ImmutableSegmentedDeque(
        const T* items,
        int count,
        int blockCapacityValue = 8,
        DequeStorageKind kind = DequeStorageKind::ArraySequence
    )
        : SegmentedDequeBase<T>(items, count, blockCapacityValue, kind) {}

    ImmutableSegmentedDeque(const ImmutableSegmentedDeque<T>& other)
        : SegmentedDequeBase<T>(other) {}
};
