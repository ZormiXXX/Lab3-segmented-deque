#pragma once

#include "DynamicArray.hpp"
#include "Mutable/MutableSegmentedDeque.hpp"
#include "SegmentedDeque.hpp"
#include <random>

template<class T>
DynamicArray<T> CopySequenceToArray(const Sequence<T>& sequence) {
    DynamicArray<T> values;
    values.Reserve(sequence.GetLength());

    IEnumerator<T>* enumerator = sequence.GetEnumerator();
    while (enumerator->MoveNext()) {
        values.Append(enumerator->GetCurrent());
    }
    delete enumerator;

    return values;
}

template<class T>
DynamicArray<T> CopyDequeToArray(const SegmentedDeque<T>& deque) {
    Sequence<T>* flat = deque.ToSequence();
    DynamicArray<T> values = CopySequenceToArray(*flat);
    delete flat;
    return values;
}

template<class T, class Compare>
long long MergeAndCountInversions(
    DynamicArray<T>& values,
    DynamicArray<T>& buffer,
    int left,
    int mid,
    int right,
    Compare compare
) {
    int leftIndex = left;
    int rightIndex = mid + 1;
    int target = left;
    long long inversions = 0;

    while (leftIndex <= mid && rightIndex <= right) {
        if (compare(values[rightIndex], values[leftIndex])) {
            inversions += mid - leftIndex + 1;
            buffer[target++] = values[rightIndex++];
        } else {
            buffer[target++] = values[leftIndex++];
        }
    }

    while (leftIndex <= mid) {
        buffer[target++] = values[leftIndex++];
    }

    while (rightIndex <= right) {
        buffer[target++] = values[rightIndex++];
    }

    for (int i = left; i <= right; i++) {
        values[i] = buffer[i];
    }

    return inversions;
}

template<class T, class Compare>
long long CountInversionsMergeSort(
    DynamicArray<T>& values,
    DynamicArray<T>& buffer,
    int left,
    int right,
    Compare compare
) {
    if (left >= right) {
        return 0;
    }

    const int mid = left + (right - left) / 2;
    long long inversions = 0;
    inversions += CountInversionsMergeSort(values, buffer, left, mid, compare);
    inversions += CountInversionsMergeSort(values, buffer, mid + 1, right, compare);
    inversions += MergeAndCountInversions(values, buffer, left, mid, right, compare);
    return inversions;
}

template<class T, class Compare>
long long CountInversionsFast(DynamicArray<T> values, Compare compare) {
    if (values.GetSize() <= 1) {
        return 0;
    }

    DynamicArray<T> buffer(values.GetSize());
    return CountInversionsMergeSort(values, buffer, 0, values.GetSize() - 1, compare);
}

template<class T, class Compare = std::less<T>>
long long CountInversionsMapReduce(const SegmentedDeque<T>& deque, Compare compare = Compare{}) {
    return CountInversionsFast(CopyDequeToArray(deque), compare);
}

template<class T, class Compare = std::less<T>>
long long CountInversionsMultiPass(const SegmentedDeque<T>& deque, Compare compare = Compare{}) {
    return CountInversionsFast(CopyDequeToArray(deque), compare);
}

template<class T, class Compare = std::less<T>>
long long CountInversionsOnePass(const SegmentedDeque<T>& deque, Compare compare = Compare{}) {
    return CountInversionsFast(CopyDequeToArray(deque), compare);
}

inline MutableSegmentedDeque<int>* GenerateRandomIntDeque(
    int count,
    int blockCapacity,
    int minValue,
    int maxValue,
    unsigned int seed,
    DequeStorageKind storageKind = DequeStorageKind::ArraySequence
) {
    auto* result = new MutableSegmentedDeque<int>(blockCapacity, storageKind);
    std::mt19937 generator(seed);
    std::uniform_int_distribution<int> distribution(minValue, maxValue);

    for (int i = 0; i < count; i++) {
        result->PushBack(distribution(generator));
    }

    return result;
}
