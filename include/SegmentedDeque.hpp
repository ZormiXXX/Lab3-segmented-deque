#pragma once

#include "ArraySequence.hpp"
#include "DynamicArray.hpp"
#include "Exceptions.hpp"
#include "ListSequence.hpp"
#include "Sequence.hpp"
#include <algorithm>
#include <functional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

enum class DequeStorageKind {
    ArraySequence,
    ListSequence
};

inline std::string DequeStorageKindToString(DequeStorageKind kind) {
    return kind == DequeStorageKind::ArraySequence ? "ArraySequence" : "ListSequence";
}

inline std::ostream& operator<<(std::ostream& stream, DequeStorageKind kind) {
    stream << DequeStorageKindToString(kind);
    return stream;
}

template<class T>
class SegmentedDeque {
public:
    virtual ~SegmentedDeque() = default;

    virtual int GetLength() const = 0;
    virtual int GetBlockCapacity() const = 0;
    virtual int GetSegmentCount() const = 0;
    virtual DequeStorageKind GetStorageKind() const = 0;
    virtual bool IsEmpty() const = 0;

    virtual T Get(int index) const = 0;
    virtual T Front() const = 0;
    virtual T Back() const = 0;

    virtual SegmentedDeque<T>* PushFront(const T& item) = 0;
    virtual SegmentedDeque<T>* PushBack(const T& item) = 0;
    virtual SegmentedDeque<T>* PopFront() = 0;
    virtual SegmentedDeque<T>* PopBack() = 0;

    virtual SegmentedDeque<T>* Concat(const SegmentedDeque<T>& other) const = 0;
    virtual SegmentedDeque<T>* GetSubDeque(int start, int end) const = 0;
    virtual int FindSubDeque(const SegmentedDeque<T>& pattern) const = 0;
    virtual bool ContainsSubDeque(const SegmentedDeque<T>& pattern) const = 0;

    virtual SegmentedDeque<T>* Map(const std::function<T(const T&)>& mapper) const = 0;
    virtual SegmentedDeque<T>* Where(const std::function<bool(const T&)>& predicate) const = 0;
    virtual T Reduce(const std::function<T(const T&, const T&)>& reducer, const T& initial) const = 0;

    virtual SegmentedDeque<T>* Sorted(
        const std::function<bool(const T&, const T&)>& comparator = std::less<T>()
    ) const = 0;

    virtual SegmentedDeque<T>* MergeSorted(
        const SegmentedDeque<T>& other,
        const std::function<bool(const T&, const T&)>& comparator = std::less<T>()
    ) const = 0;

    virtual Sequence<T>* ToSequence() const = 0;
    virtual std::vector<int> GetBlockSizes() const = 0;
    virtual std::string DescribeLayout() const = 0;

    T operator[](int index) const {
        return Get(index);
    }
};

template<class T>
class SegmentedDequeBase : public SegmentedDeque<T> {
protected:
    struct Block {
        DynamicArray<T> data;
        int begin;
        int end;

        Block(int capacity, int startIndex)
            : data(capacity), begin(startIndex), end(startIndex) {}

        Block(const Block& other)
            : data(other.data), begin(other.begin), end(other.end) {}

        int Capacity() const {
            return data.GetSize();
        }

        int Size() const {
            return end - begin;
        }

        bool Empty() const {
            return begin == end;
        }

        bool CanPushFront() const {
            return begin > 0;
        }

        bool CanPushBack() const {
            return end < Capacity();
        }

        void PushFront(const T& item) {
            if (!CanPushFront()) {
                throw std::runtime_error("Front side of block is full");
            }
            data.Set(--begin, item);
        }

        void PushBack(const T& item) {
            if (!CanPushBack()) {
                throw std::runtime_error("Back side of block is full");
            }
            data.Set(end++, item);
        }

        void PopFront() {
            if (Empty()) {
                throw EmptyCollection();
            }
            begin++;
        }

        void PopBack() {
            if (Empty()) {
                throw EmptyCollection();
            }
            end--;
        }

        T Get(int index) const {
            if (index < 0 || index >= Size()) {
                throw IndexOutOfRange(index, Size());
            }
            return data.Get(begin + index);
        }
    };

    Sequence<Block*>* blocks;
    int blockCapacity;
    int totalSize;
    DequeStorageKind storageKind;

    virtual SegmentedDequeBase<T>* Clone() const = 0;
    virtual SegmentedDequeBase<T>* Instance() = 0;
    virtual SegmentedDequeBase<T>* CreateEmptySameKind() const = 0;

    template<class U>
    Sequence<U>* CreateSequenceForKind() const {
        if (storageKind == DequeStorageKind::ArraySequence) {
            return new ArraySequence<U>();
        }
        return new ListSequence<U>();
    }

    Sequence<Block*>* CreateBlockSequence(DequeStorageKind kind) const {
        if (kind == DequeStorageKind::ArraySequence) {
            return new ArraySequence<Block*>();
        }
        return new ListSequence<Block*>();
    }

    void ReplaceBlocksSequence(Sequence<Block*>* updated) {
        if (updated != blocks) {
            delete blocks;
            blocks = updated;
        }
    }

    void DestroyBlocks() {
        if (blocks == nullptr) {
            return;
        }
        for (int i = 0; i < blocks->GetLength(); i++) {
            delete blocks->Get(i);
        }
    }

    Block* MakeCenteredBlock() const {
        return new Block(blockCapacity, blockCapacity / 2);
    }

    Block* MakeFrontExpansionBlock() const {
        return new Block(blockCapacity, blockCapacity);
    }

    Block* MakeBackExpansionBlock() const {
        return new Block(blockCapacity, 0);
    }

    void RemoveFrontBlock() {
        Block* victim = blocks->GetFirst();
        delete victim;
        Sequence<Block*>* updated = blocks->Slice(0, 1);
        ReplaceBlocksSequence(updated);
    }

    void RemoveBackBlock() {
        const int lastIndex = blocks->GetLength() - 1;
        Block* victim = blocks->Get(lastIndex);
        delete victim;
        Sequence<Block*>* updated = blocks->Slice(lastIndex, 1);
        ReplaceBlocksSequence(updated);
    }

    void PushBackDirect(const T& item) {
        if (blocks->GetLength() == 0) {
            ReplaceBlocksSequence(blocks->Append(MakeCenteredBlock()));
        }

        Block* last = blocks->GetLast();
        if (!last->CanPushBack()) {
            ReplaceBlocksSequence(blocks->Append(MakeBackExpansionBlock()));
            last = blocks->GetLast();
        }

        last->PushBack(item);
        totalSize++;
    }

    void PushFrontDirect(const T& item) {
        if (blocks->GetLength() == 0) {
            ReplaceBlocksSequence(blocks->Append(MakeCenteredBlock()));
        }

        Block* first = blocks->GetFirst();
        if (!first->CanPushFront()) {
            ReplaceBlocksSequence(blocks->Prepend(MakeFrontExpansionBlock()));
            first = blocks->GetFirst();
        }

        first->PushFront(item);
        totalSize++;
    }

    void PopFrontDirect() {
        if (IsEmpty()) {
            throw EmptyCollection();
        }

        Block* first = blocks->GetFirst();
        first->PopFront();
        totalSize--;

        if (first->Empty()) {
            RemoveFrontBlock();
        }
    }

    void PopBackDirect() {
        if (IsEmpty()) {
            throw EmptyCollection();
        }

        Block* last = blocks->GetLast();
        last->PopBack();
        totalSize--;

        if (last->Empty()) {
            RemoveBackBlock();
        }
    }

public:
    explicit SegmentedDequeBase(
        int blockCapacityValue = 8,
        DequeStorageKind kind = DequeStorageKind::ArraySequence
    )
        : blocks(nullptr), blockCapacity(blockCapacityValue), totalSize(0), storageKind(kind) {
        if (blockCapacityValue <= 0) {
            throw std::invalid_argument("blockCapacity must be positive");
        }
        blocks = CreateBlockSequence(storageKind);
    }

    SegmentedDequeBase(
        const T* items,
        int count,
        int blockCapacityValue = 8,
        DequeStorageKind kind = DequeStorageKind::ArraySequence
    )
        : SegmentedDequeBase(blockCapacityValue, kind) {
        if (count < 0) {
            throw std::invalid_argument("count must be non-negative");
        }
        for (int i = 0; i < count; i++) {
            PushBackDirect(items[i]);
        }
    }

    SegmentedDequeBase(const SegmentedDequeBase<T>& other)
        : blocks(nullptr),
          blockCapacity(other.blockCapacity),
          totalSize(other.totalSize),
          storageKind(other.storageKind) {
        blocks = CreateBlockSequence(storageKind);
        for (int i = 0; i < other.blocks->GetLength(); i++) {
            ReplaceBlocksSequence(blocks->Append(new Block(*other.blocks->Get(i))));
        }
    }

    SegmentedDequeBase<T>& operator=(const SegmentedDequeBase<T>&) = delete;

    ~SegmentedDequeBase() override {
        DestroyBlocks();
        delete blocks;
    }

    int GetLength() const override {
        return totalSize;
    }

    int GetBlockCapacity() const override {
        return blockCapacity;
    }

    int GetSegmentCount() const override {
        return blocks->GetLength();
    }

    DequeStorageKind GetStorageKind() const override {
        return storageKind;
    }

    bool IsEmpty() const override {
        return totalSize == 0;
    }

    T Get(int index) const override {
        if (index < 0 || index >= totalSize) {
            throw IndexOutOfRange(index, totalSize);
        }

        int offset = index;
        for (int i = 0; i < blocks->GetLength(); i++) {
            Block* block = blocks->Get(i);
            if (offset < block->Size()) {
                return block->Get(offset);
            }
            offset -= block->Size();
        }

        throw IndexOutOfRange(index, totalSize);
    }

    T Front() const override {
        if (IsEmpty()) {
            throw EmptyCollection();
        }
        return blocks->GetFirst()->Get(0);
    }

    T Back() const override {
        if (IsEmpty()) {
            throw EmptyCollection();
        }
        Block* last = blocks->GetLast();
        return last->Get(last->Size() - 1);
    }

    SegmentedDeque<T>* PushFront(const T& item) override {
        SegmentedDequeBase<T>* result = Instance();
        result->PushFrontDirect(item);
        return result;
    }

    SegmentedDeque<T>* PushBack(const T& item) override {
        SegmentedDequeBase<T>* result = Instance();
        result->PushBackDirect(item);
        return result;
    }

    SegmentedDeque<T>* PopFront() override {
        SegmentedDequeBase<T>* result = Instance();
        result->PopFrontDirect();
        return result;
    }

    SegmentedDeque<T>* PopBack() override {
        SegmentedDequeBase<T>* result = Instance();
        result->PopBackDirect();
        return result;
    }

    SegmentedDeque<T>* Concat(const SegmentedDeque<T>& other) const override {
        SegmentedDequeBase<T>* result = CreateEmptySameKind();
        for (int i = 0; i < GetLength(); i++) {
            result->PushBackDirect(Get(i));
        }
        for (int i = 0; i < other.GetLength(); i++) {
            result->PushBackDirect(other.Get(i));
        }
        return result;
    }

    SegmentedDeque<T>* GetSubDeque(int start, int end) const override {
        if (start < 0 || end >= GetLength() || start > end) {
            throw IndexOutOfRange(start, GetLength());
        }

        SegmentedDequeBase<T>* result = CreateEmptySameKind();
        for (int i = start; i <= end; i++) {
            result->PushBackDirect(Get(i));
        }
        return result;
    }

    int FindSubDeque(const SegmentedDeque<T>& pattern) const override {
        if (pattern.GetLength() == 0) {
            return 0;
        }
        if (pattern.GetLength() > GetLength()) {
            return -1;
        }

        for (int start = 0; start <= GetLength() - pattern.GetLength(); start++) {
            bool matched = true;
            for (int offset = 0; offset < pattern.GetLength(); offset++) {
                if (!(Get(start + offset) == pattern.Get(offset))) {
                    matched = false;
                    break;
                }
            }
            if (matched) {
                return start;
            }
        }

        return -1;
    }

    bool ContainsSubDeque(const SegmentedDeque<T>& pattern) const override {
        return FindSubDeque(pattern) >= 0;
    }

    SegmentedDeque<T>* Map(const std::function<T(const T&)>& mapper) const override {
        SegmentedDequeBase<T>* result = CreateEmptySameKind();
        for (int i = 0; i < GetLength(); i++) {
            result->PushBackDirect(mapper(Get(i)));
        }
        return result;
    }

    SegmentedDeque<T>* Where(const std::function<bool(const T&)>& predicate) const override {
        SegmentedDequeBase<T>* result = CreateEmptySameKind();
        for (int i = 0; i < GetLength(); i++) {
            T item = Get(i);
            if (predicate(item)) {
                result->PushBackDirect(item);
            }
        }
        return result;
    }

    T Reduce(const std::function<T(const T&, const T&)>& reducer, const T& initial) const override {
        T result = initial;
        for (int i = 0; i < GetLength(); i++) {
            result = reducer(Get(i), result);
        }
        return result;
    }

    SegmentedDeque<T>* Sorted(
        const std::function<bool(const T&, const T&)>& comparator = std::less<T>()
    ) const override {
        std::vector<T> values;
        values.reserve(GetLength());
        for (int i = 0; i < GetLength(); i++) {
            values.push_back(Get(i));
        }

        std::sort(values.begin(), values.end(), comparator);

        SegmentedDequeBase<T>* result = CreateEmptySameKind();
        for (const T& value : values) {
            result->PushBackDirect(value);
        }
        return result;
    }

    SegmentedDeque<T>* MergeSorted(
        const SegmentedDeque<T>& other,
        const std::function<bool(const T&, const T&)>& comparator = std::less<T>()
    ) const override {
        SegmentedDequeBase<T>* result = CreateEmptySameKind();

        int left = 0;
        int right = 0;
        while (left < GetLength() && right < other.GetLength()) {
            if (comparator(other.Get(right), Get(left))) {
                result->PushBackDirect(other.Get(right++));
            } else {
                result->PushBackDirect(Get(left++));
            }
        }

        while (left < GetLength()) {
            result->PushBackDirect(Get(left++));
        }

        while (right < other.GetLength()) {
            result->PushBackDirect(other.Get(right++));
        }

        return result;
    }

    Sequence<T>* ToSequence() const override {
        Sequence<T>* result = CreateSequenceForKind<T>();
        for (int i = 0; i < GetLength(); i++) {
            Sequence<T>* updated = result->Append(Get(i));
            if (updated != result) {
                delete result;
                result = updated;
            }
        }
        return result;
    }

    std::vector<int> GetBlockSizes() const override {
        std::vector<int> result;
        result.reserve(blocks->GetLength());
        for (int i = 0; i < blocks->GetLength(); i++) {
            result.push_back(blocks->Get(i)->Size());
        }
        return result;
    }

    std::string DescribeLayout() const override {
        std::ostringstream stream;
        stream << "storage=" << DequeStorageKindToString(storageKind)
               << ", blockCapacity=" << blockCapacity
               << ", segments=" << GetSegmentCount()
               << ", fills=[";

        for (int i = 0; i < blocks->GetLength(); i++) {
            stream << blocks->Get(i)->Size();
            if (i + 1 < blocks->GetLength()) {
                stream << ", ";
            }
        }
        stream << "]";
        return stream.str();
    }
};
