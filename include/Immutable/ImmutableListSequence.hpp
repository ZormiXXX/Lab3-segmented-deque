#pragma once
#include "../ListSequence.hpp"

template<class T>
class ImmutableListSequence : public ListSequence<T> {
protected:
    ImmutableListSequence<T>* Clone() const override {
        return new ImmutableListSequence<T>(*this);
    }

    ImmutableListSequence<T>* Instance() override {
        return Clone();
    }

public:
    ImmutableListSequence() : ListSequence<T>() {}

    ImmutableListSequence(const T* arr, int count) : ListSequence<T>(arr, count) {}

    ImmutableListSequence(const ImmutableListSequence<T>& other)
        : ListSequence<T>(new LinkedList<T>(*other.items)) {}

    explicit ImmutableListSequence(const LinkedList<T>& list) : ListSequence<T>(list) {}

    explicit ImmutableListSequence(LinkedList<T>* list) : ListSequence<T>(list) {}

    ~ImmutableListSequence() override = default;

    Sequence<T>* CreateEmpty() const override {
        return new ImmutableListSequence<T>();
    }

    Sequence<T>* CreateFromArray(const T* items, int count) const override {
        return new ImmutableListSequence<T>(items, count);
    }
};
