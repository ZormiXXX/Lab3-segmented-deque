#pragma once

template<class T>
SegmentedDeque<T>::Block::Block(int capacity, int startIndex)
    : data(capacity), begin(startIndex), end(startIndex) {}

template<class T>
SegmentedDeque<T>::Block::Block(const Block& other)
    : data(other.data), begin(other.begin), end(other.end) {}

template<class T>
int SegmentedDeque<T>::Block::Capacity() const {
    return data.GetSize();
}

template<class T>
int SegmentedDeque<T>::Block::Size() const {
    return end - begin;
}

template<class T>
bool SegmentedDeque<T>::Block::Empty() const {
    return begin == end;
}

template<class T>
bool SegmentedDeque<T>::Block::CanPushFront() const {
    return begin > 0;
}

template<class T>
bool SegmentedDeque<T>::Block::CanPushBack() const {
    return end < Capacity();
}

template<class T>
void SegmentedDeque<T>::Block::PushFront(const T& item) {
    if (!CanPushFront()) {
        throw InvalidState("front side of block is full");
    }
    data.Set(--begin, item);
}

template<class T>
void SegmentedDeque<T>::Block::PushBack(const T& item) {
    if (!CanPushBack()) {
        throw InvalidState("back side of block is full");
    }
    data.Set(end++, item);
}

template<class T>
void SegmentedDeque<T>::Block::PopFront() {
    if (Empty()) {
        throw EmptyCollection();
    }
    begin++;
}

template<class T>
void SegmentedDeque<T>::Block::PopBack() {
    if (Empty()) {
        throw EmptyCollection();
    }
    end--;
}

template<class T>
const T& SegmentedDeque<T>::Block::Get(int index) const {
    if (index < 0 || index >= Size()) {
        throw IndexOutOfRange(index, Size());
    }
    return data.Get(begin + index);
}

template<class T>
template<class Pointer>
void SegmentedDeque<T>::ReplaceOwned(Pointer*& current, Pointer* updated) {
    if (updated != current) {
        delete current;
        current = updated;
    }
}

template<class T>
template<class U>
void SegmentedDeque<T>::AppendOwned(Sequence<U>*& sequence, const U& value) {
    ReplaceOwned(sequence, sequence->Append(value));
}

template<class T>
void SegmentedDeque<T>::AppendDequeTo(SegmentedDeque<T>& target, const SegmentedDeque<T>& source) {
    source.ForEachElement([&](const T& item) {
        target.PushBackDirect(item);
    });
}

template<class T>
template<class U>
Sequence<U>* SegmentedDeque<T>::CreateSequenceForKind() const {
    return storageKind == DequeStorageKind::ArraySequence
        ? static_cast<Sequence<U>*>(new ArraySequence<U>())
        : static_cast<Sequence<U>*>(new ListSequence<U>());
}

template<class T>
Sequence<typename SegmentedDeque<T>::Block*>* SegmentedDeque<T>::CreateBlockSequence(
    DequeStorageKind kind
) const {
    return kind == DequeStorageKind::ArraySequence
        ? static_cast<Sequence<Block*>*>(new ArraySequence<Block*>())
        : static_cast<Sequence<Block*>*>(new ListSequence<Block*>());
}

template<class T>
void SegmentedDeque<T>::DestroyBlocks() {
    if (blocks == nullptr) {
        return;
    }

    IEnumerator<Block*>* enumerator = blocks->GetEnumerator();
    while (enumerator->MoveNext()) {
        delete enumerator->GetCurrent();
    }
    delete enumerator;
}

template<class T>
typename SegmentedDeque<T>::Block* SegmentedDeque<T>::MakeBlock(int startIndex) const {
    return new Block(blockCapacity, startIndex);
}

template<class T>
typename SegmentedDeque<T>::Block* SegmentedDeque<T>::EnsureFrontBlockForPush() {
    if (blocks->GetLength() == 0) {
        ReplaceOwned(blocks, blocks->Append(MakeBlock(blockCapacity / 2)));
    }

    Block* first = blocks->GetFirst();
    if (!first->CanPushFront()) {
        ReplaceOwned(blocks, blocks->Prepend(MakeBlock(blockCapacity)));
        first = blocks->GetFirst();
    }
    return first;
}

template<class T>
typename SegmentedDeque<T>::Block* SegmentedDeque<T>::EnsureBackBlockForPush() {
    if (blocks->GetLength() == 0) {
        ReplaceOwned(blocks, blocks->Append(MakeBlock(blockCapacity / 2)));
    }

    Block* last = blocks->GetLast();
    if (!last->CanPushBack()) {
        ReplaceOwned(blocks, blocks->Append(MakeBlock(0)));
        last = blocks->GetLast();
    }
    return last;
}

template<class T>
void SegmentedDeque<T>::RemoveBoundaryBlock(bool fromFront) {
    const int index = fromFront ? 0 : blocks->GetLength() - 1;
    delete blocks->Get(index);
    ReplaceOwned(blocks, blocks->Slice(index, 1));
}

template<class T>
const typename SegmentedDeque<T>::Block& SegmentedDeque<T>::FindBlock(int index, int& localIndex) const {
    int offset = index;
    const Block* found = nullptr;

    IEnumerator<Block*>* enumerator = blocks->GetEnumerator();
    while (enumerator->MoveNext()) {
        Block* block = enumerator->GetCurrent();
        if (offset < block->Size()) {
            localIndex = offset;
            found = block;
            break;
        }
        offset -= block->Size();
    }
    delete enumerator;

    if (found != nullptr) {
        return *found;
    }
    throw IndexOutOfRange(index, totalSize);
}

template<class T>
const T& SegmentedDeque<T>::GetRef(int index) const {
    if (index < 0 || index >= totalSize) {
        throw IndexOutOfRange(index, totalSize);
    }

    int localIndex = 0;
    return FindBlock(index, localIndex).Get(localIndex);
}

template<class T>
template<class Visitor>
void SegmentedDeque<T>::ForEachElement(Visitor visitor) const {
    IEnumerator<Block*>* enumerator = blocks->GetEnumerator();
    while (enumerator->MoveNext()) {
        Block* block = enumerator->GetCurrent();
        for (int j = 0; j < block->Size(); j++) {
            visitor(block->Get(j));
        }
    }
    delete enumerator;
}

template<class T>
template<class Visitor>
void SegmentedDeque<T>::ForEachBlock(Visitor visitor) const {
    IEnumerator<Block*>* enumerator = blocks->GetEnumerator();
    int index = 0;
    while (enumerator->MoveNext()) {
        visitor(*enumerator->GetCurrent(), index++);
    }
    delete enumerator;
}

template<class T>
DynamicArray<T> SegmentedDeque<T>::CopyElementsToArray() const {
    DynamicArray<T> values;
    values.Reserve(totalSize);
    ForEachElement([&](const T& item) {
        values.Append(item);
    });
    return values;
}

template<class T>
SegmentedDeque<T>* SegmentedDeque<T>::BuildFromArray(const DynamicArray<T>& values) const {
    SegmentedDeque<T>* result = CreateEmptySameKind();
    for (int i = 0; i < values.GetSize(); i++) {
        result->PushBackDirect(values[i]);
    }
    return result;
}

template<class T>
DynamicArray<int> SegmentedDeque<T>::BuildPrefixTable(const DynamicArray<T>& pattern) {
    DynamicArray<int> prefix(pattern.GetSize());
    int matched = 0;

    for (int i = 1; i < pattern.GetSize(); i++) {
        while (matched > 0 && !(pattern[i] == pattern[matched])) {
            matched = prefix[matched - 1];
        }
        if (pattern[i] == pattern[matched]) {
            matched++;
        }
        prefix[i] = matched;
    }

    return prefix;
}

template<class T>
template<class Comparator>
void SegmentedDeque<T>::MergeSortValues(
    DynamicArray<T>& values,
    DynamicArray<T>& buffer,
    int left,
    int right,
    const Comparator& comparator
) {
    if (left >= right) {
        return;
    }

    const int mid = left + (right - left) / 2;
    MergeSortValues(values, buffer, left, mid, comparator);
    MergeSortValues(values, buffer, mid + 1, right, comparator);
    MergeSortedRanges(values, buffer, left, mid, right, comparator);
}

template<class T>
template<class Comparator>
void SegmentedDeque<T>::MergeSortedRanges(
    DynamicArray<T>& values,
    DynamicArray<T>& buffer,
    int left,
    int mid,
    int right,
    const Comparator& comparator
) {
    int leftIndex = left;
    int rightIndex = mid + 1;
    int target = left;

    while (leftIndex <= mid && rightIndex <= right) {
        if (comparator(values[rightIndex], values[leftIndex])) {
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
}

template<class T>
void SegmentedDeque<T>::PushBackDirect(const T& item) {
    EnsureBackBlockForPush()->PushBack(item);
    totalSize++;
}

template<class T>
void SegmentedDeque<T>::PushFrontDirect(const T& item) {
    EnsureFrontBlockForPush()->PushFront(item);
    totalSize++;
}

template<class T>
void SegmentedDeque<T>::PopFrontDirect() {
    if (IsEmpty()) {
        throw EmptyCollection();
    }

    Block* first = blocks->GetFirst();
    first->PopFront();
    totalSize--;

    if (first->Empty()) {
        RemoveBoundaryBlock(true);
    }
}

template<class T>
void SegmentedDeque<T>::PopBackDirect() {
    if (IsEmpty()) {
        throw EmptyCollection();
    }

    Block* last = blocks->GetLast();
    last->PopBack();
    totalSize--;

    if (last->Empty()) {
        RemoveBoundaryBlock(false);
    }
}

template<class T>
SegmentedDeque<T>::SegmentedDeque(int blockCapacityValue, DequeStorageKind kind)
    : blocks(nullptr), blockCapacity(blockCapacityValue), totalSize(0), storageKind(kind) {
    if (blockCapacityValue <= 0) {
        throw InvalidArgument("blockCapacity must be positive");
    }
    blocks = CreateBlockSequence(storageKind);
}

template<class T>
SegmentedDeque<T>::SegmentedDeque(
    const T* items,
    int count,
    int blockCapacityValue,
    DequeStorageKind kind
)
    : SegmentedDeque(blockCapacityValue, kind) {
    if (count < 0) {
        throw InvalidArgument("count must be non-negative");
    }
    for (int i = 0; i < count; i++) {
        PushBackDirect(items[i]);
    }
}

template<class T>
SegmentedDeque<T>::SegmentedDeque(const SegmentedDeque<T>& other)
    : blocks(nullptr),
      blockCapacity(other.blockCapacity),
      totalSize(other.totalSize),
      storageKind(other.storageKind) {
    blocks = CreateBlockSequence(storageKind);
    other.ForEachBlock([&](const Block& block, int) {
        ReplaceOwned(blocks, blocks->Append(new Block(block)));
    });
}

template<class T>
SegmentedDeque<T>::~SegmentedDeque() {
    DestroyBlocks();
    delete blocks;
}

template<class T>
int SegmentedDeque<T>::GetLength() const {
    return totalSize;
}

template<class T>
int SegmentedDeque<T>::GetBlockCapacity() const {
    return blockCapacity;
}

template<class T>
int SegmentedDeque<T>::GetSegmentCount() const {
    return blocks->GetLength();
}

template<class T>
DequeStorageKind SegmentedDeque<T>::GetStorageKind() const {
    return storageKind;
}

template<class T>
bool SegmentedDeque<T>::IsEmpty() const {
    return totalSize == 0;
}

template<class T>
T SegmentedDeque<T>::Get(int index) const {
    return GetRef(index);
}

template<class T>
T SegmentedDeque<T>::Front() const {
    if (IsEmpty()) {
        throw EmptyCollection();
    }
    return blocks->GetFirst()->Get(0);
}

template<class T>
T SegmentedDeque<T>::Back() const {
    if (IsEmpty()) {
        throw EmptyCollection();
    }
    Block* last = blocks->GetLast();
    return last->Get(last->Size() - 1);
}

template<class T>
SegmentedDeque<T>* SegmentedDeque<T>::PushFront(const T& item) {
    SegmentedDeque<T>* result = Instance();
    result->PushFrontDirect(item);
    return result;
}

template<class T>
SegmentedDeque<T>* SegmentedDeque<T>::PushBack(const T& item) {
    SegmentedDeque<T>* result = Instance();
    result->PushBackDirect(item);
    return result;
}

template<class T>
SegmentedDeque<T>* SegmentedDeque<T>::PopFront() {
    SegmentedDeque<T>* result = Instance();
    result->PopFrontDirect();
    return result;
}

template<class T>
SegmentedDeque<T>* SegmentedDeque<T>::PopBack() {
    SegmentedDeque<T>* result = Instance();
    result->PopBackDirect();
    return result;
}

template<class T>
SegmentedDeque<T>* SegmentedDeque<T>::Concat(const SegmentedDeque<T>& other) const {
    SegmentedDeque<T>* result = CreateEmptySameKind();
    AppendDequeTo(*result, *this);
    AppendDequeTo(*result, other);
    return result;
}

template<class T>
SegmentedDeque<T>* SegmentedDeque<T>::GetSubDeque(int start, int end) const {
    if (start < 0 || end >= GetLength() || start > end) {
        throw IndexOutOfRange(start, GetLength());
    }

    SegmentedDeque<T>* result = CreateEmptySameKind();
    int index = 0;
    ForEachElement([&](const T& item) {
        if (index >= start && index <= end) {
            result->PushBackDirect(item);
        }
        index++;
    });
    return result;
}

template<class T>
int SegmentedDeque<T>::FindSubDeque(const SegmentedDeque<T>& pattern) const {
    if (pattern.GetLength() == 0) {
        return 0;
    }
    if (pattern.GetLength() > GetLength()) {
        return -1;
    }

    DynamicArray<T> textValues = CopyElementsToArray();
    DynamicArray<T> patternValues = pattern.CopyElementsToArray();
    DynamicArray<int> prefix = BuildPrefixTable(patternValues);

    int matched = 0;
    for (int i = 0; i < textValues.GetSize(); i++) {
        while (matched > 0 && !(textValues[i] == patternValues[matched])) {
            matched = prefix[matched - 1];
        }

        if (textValues[i] == patternValues[matched]) {
            matched++;
            if (matched == patternValues.GetSize()) {
                return i - matched + 1;
            }
        }
    }

    return -1;
}

template<class T>
bool SegmentedDeque<T>::ContainsSubDeque(const SegmentedDeque<T>& pattern) const {
    return FindSubDeque(pattern) >= 0;
}

template<class T>
SegmentedDeque<T>* SegmentedDeque<T>::Map(const std::function<T(const T&)>& mapper) const {
    SegmentedDeque<T>* result = CreateEmptySameKind();
    ForEachElement([&](const T& item) {
        result->PushBackDirect(mapper(item));
    });
    return result;
}

template<class T>
SegmentedDeque<T>* SegmentedDeque<T>::Where(const std::function<bool(const T&)>& predicate) const {
    SegmentedDeque<T>* result = CreateEmptySameKind();
    ForEachElement([&](const T& item) {
        if (predicate(item)) {
            result->PushBackDirect(item);
        }
    });
    return result;
}

template<class T>
T SegmentedDeque<T>::Reduce(
    const std::function<T(const T&, const T&)>& reducer,
    const T& initial
) const {
    T result = initial;
    ForEachElement([&](const T& item) {
        result = reducer(item, result);
    });
    return result;
}

template<class T>
SegmentedDeque<T>* SegmentedDeque<T>::Sorted(
    const std::function<bool(const T&, const T&)>& comparator
) const {
    DynamicArray<T> values = CopyElementsToArray();
    if (values.GetSize() <= 1) {
        return BuildFromArray(values);
    }

    DynamicArray<T> buffer(values.GetSize());
    MergeSortValues(values, buffer, 0, values.GetSize() - 1, comparator);
    return BuildFromArray(values);
}

template<class T>
SegmentedDeque<T>* SegmentedDeque<T>::MergeSorted(
    const SegmentedDeque<T>& other,
    const std::function<bool(const T&, const T&)>& comparator
) const {
    DynamicArray<T> leftValues = CopyElementsToArray();
    DynamicArray<T> rightValues = other.CopyElementsToArray();
    DynamicArray<T> merged;
    merged.Reserve(leftValues.GetSize() + rightValues.GetSize());

    int left = 0;
    int right = 0;
    while (left < leftValues.GetSize() && right < rightValues.GetSize()) {
        if (comparator(rightValues[right], leftValues[left])) {
            merged.Append(rightValues[right++]);
        } else {
            merged.Append(leftValues[left++]);
        }
    }

    while (left < leftValues.GetSize()) {
        merged.Append(leftValues[left++]);
    }

    while (right < rightValues.GetSize()) {
        merged.Append(rightValues[right++]);
    }

    return BuildFromArray(merged);
}

template<class T>
Sequence<T>* SegmentedDeque<T>::ToSequence() const {
    Sequence<T>* result = CreateSequenceForKind<T>();
    ForEachElement([&](const T& item) {
        AppendOwned(result, item);
    });
    return result;
}

template<class T>
std::string SegmentedDeque<T>::DescribeLayout() const {
    std::string result = "storage=" + DequeStorageKindToString(storageKind)
                       + ", blockCapacity=" + std::to_string(blockCapacity)
                       + ", segments=" + std::to_string(GetSegmentCount())
                       + ", fills=[";

    ForEachBlock([&](const Block& block, int index) {
        if (index > 0) {
            result += ", ";
        }
        result += std::to_string(block.Size());
    });

    result += "]";
    return result;
}
