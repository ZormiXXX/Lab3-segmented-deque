#pragma once
#include "LinkedList.hpp"
#include "Sequence.hpp"

template<class T>
class ListSequence : public Sequence<T> {
protected:
    LinkedList<T>* items;

    virtual ListSequence<T>* Clone() const {
        return new ListSequence<T>(*this);
    }

    virtual ListSequence<T>* Instance() {
        return this;
    }

    void AppendInternal(T item) {
        items->Append(item);
    }

    void PrependInternal(T item) {
        items->Prepend(item);
    }

    void InsertAtInternal(T item, int index) {
        items->InsertAt(item, index);
    }

    void ConcatInternal(Sequence<T>* other) {
        for (int i = 0; i < other->GetLength(); i++) {
            items->Append(other->Get(i));
        }
    }

public:
    ListSequence() : items(new LinkedList<T>()) {}

    ListSequence(const T* arr, int count) : items(new LinkedList<T>(arr, count)) {}

    ListSequence(const ListSequence<T>& other) : items(new LinkedList<T>(*other.items)) {}

    explicit ListSequence(const LinkedList<T>& list) : items(new LinkedList<T>(list)) {}

    explicit ListSequence(LinkedList<T>* list) : items(new LinkedList<T>(*list)) {}

    ~ListSequence() override {
        delete items;
    }

    T Get(int index) const override {
        return items->Get(index);
    }

    int GetLength() const override {
        return items->GetLength();
    }

    T GetFirst() const override {
        return items->GetFirst();
    }

    T GetLast() const override {
        return items->GetLast();
    }

    Sequence<T>* CreateEmpty() const override {
        return new ListSequence<T>();
    }

    Sequence<T>* CreateFromArray(const T* itemsArr, int count) const override {
        return new ListSequence<T>(itemsArr, count);
    }

    Sequence<T>* Append(T item) override {
        ListSequence<T>* result = Instance();
        result->AppendInternal(item);
        return result;
    }

    Sequence<T>* Prepend(T item) override {
        ListSequence<T>* result = Instance();
        result->PrependInternal(item);
        return result;
    }

    Sequence<T>* InsertAt(T item, int index) override {
        ListSequence<T>* result = Instance();
        result->InsertAtInternal(item, index);
        return result;
    }

    Sequence<T>* Concat(Sequence<T>* other) override {
        ListSequence<T>* result = Instance();
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
};
