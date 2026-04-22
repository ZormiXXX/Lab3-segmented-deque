#pragma once
#include "DynamicArray.hpp"
#include "LinkedList.hpp"
#include "Sequence.hpp"

template<class T>
class ArraySequence : public Sequence<T> {
protected:
    DynamicArray<T>* items;

    virtual ArraySequence<T>* Clone() const {
        return new ArraySequence<T>(*this);
    }

    virtual ArraySequence<T>* Instance() {
        return this;
    }

    void AppendInternal(T item) {
        items->Resize(items->GetSize() + 1);
        items->Set(items->GetSize() - 1, item);
    }

    void PrependInternal(T item) {
        DynamicArray<T>* newArr = new DynamicArray<T>(items->GetSize() + 1);
        newArr->Set(0, item);
        for (int i = 0; i < items->GetSize(); i++) {
            newArr->Set(i + 1, items->Get(i));
        }
        delete items;
        items = newArr;
    }

    void InsertAtInternal(T item, int index) {
        if (index < 0 || index >= items->GetSize()) {
            throw IndexOutOfRange(index, items->GetSize());
        }

        DynamicArray<T>* newArr = new DynamicArray<T>(items->GetSize() + 1);
        for (int i = 0; i < index; i++) {
            newArr->Set(i, items->Get(i));
        }
        newArr->Set(index, item);
        for (int i = index; i < items->GetSize(); i++) {
            newArr->Set(i + 1, items->Get(i));
        }
        delete items;
        items = newArr;
    }

    void ConcatInternal(Sequence<T>* other) {
        for (int i = 0; i < other->GetLength(); i++) {
            AppendInternal(other->Get(i));
        }
    }

public:
    ArraySequence() : items(new DynamicArray<T>()) {}

    ArraySequence(const T* arr, int count) : items(new DynamicArray<T>(arr, count)) {}

    ArraySequence(DynamicArray<T>* arr) : items(arr) {}

    ArraySequence(const ArraySequence<T>& other) : items(new DynamicArray<T>(*other.items)) {}

    ArraySequence(const LinkedList<T>& list) : items(new DynamicArray<T>(list.GetLength())) {
        for (int i = 0; i < list.GetLength(); i++) {
            items->Set(i, list.Get(i));
        }
    }

    ArraySequence(LinkedList<T>* list) : ArraySequence(*list) {}

    ~ArraySequence() override {
        delete items;
    }

    T Get(int index) const override {
        return items->Get(index);
    }

    int GetLength() const override {
        return items->GetSize();
    }

    T GetFirst() const override {
        return items->Get(0);
    }

    T GetLast() const override {
        if (GetLength() == 0) {
            throw IndexOutOfRange(0, GetLength());
        }
        return items->Get(GetLength() - 1);
    }

    Sequence<T>* CreateEmpty() const override {
        return new ArraySequence<T>();
    }

    Sequence<T>* CreateFromArray(const T* source, int count) const override {
        return new ArraySequence<T>(source, count);
    }

    Sequence<T>* Append(T item) override {
        ArraySequence<T>* result = Instance();
        result->AppendInternal(item);
        return result;
    }

    Sequence<T>* Prepend(T item) override {
        ArraySequence<T>* result = Instance();
        result->PrependInternal(item);
        return result;
    }

    Sequence<T>* InsertAt(T item, int index) override {
        ArraySequence<T>* result = Instance();
        result->InsertAtInternal(item, index);
        return result;
    }

    Sequence<T>* Concat(Sequence<T>* other) override {
        ArraySequence<T>* result = Instance();
        result->ConcatInternal(other);
        return result;
    }

    Sequence<T>* GetSubsequence(int start, int end) override {
        if (start < 0 || end >= GetLength() || start > end) {
            throw IndexOutOfRange(start, GetLength());
        }

        Sequence<T>* result = CreateEmpty();
        for (int i = start; i <= end; i++) {
            Sequence<T>* updated = result->Append(items->Get(i));
            if (updated != result) {
                delete result;
            }
            result = updated;
        }
        return result;
    }

    T& operator[](int index) {
        return (*items)[index];
    }

    const T& operator[](int index) const {
        return (*items)[index];
    }
};
