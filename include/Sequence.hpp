#pragma once
#include "Exceptions.hpp"
#include "IEnumerable.hpp"
#include "Option.hpp"
#include <algorithm>
#include <stdexcept>
#include <tuple>
#include <utility>

template<class T>
class ArraySequence;

template<class T>
class Sequence;

template<class T>
class SequenceEnumerator : public IEnumerator<T> {
private:
    const Sequence<T>* sequence;
    int index;

public:
    explicit SequenceEnumerator(const Sequence<T>* seq) : sequence(seq), index(-1) {}

    bool MoveNext() override;
    T GetCurrent() const override;
    void Reset() override;
};

template<class T>
class Sequence : public IEnumerable<T> {
public:
    virtual ~Sequence() = default;

    virtual T Get(int index) const = 0;
    virtual int GetLength() const = 0;
    virtual T GetFirst() const = 0;
    virtual T GetLast() const = 0;

    virtual Sequence<T>* Append(T item) = 0;
    virtual Sequence<T>* Prepend(T item) = 0;
    virtual Sequence<T>* InsertAt(T item, int index) = 0;
    virtual Sequence<T>* Concat(Sequence<T>* other) = 0;
    virtual Sequence<T>* GetSubsequence(int start, int end) = 0;
    virtual Sequence<T>* CreateEmpty() const = 0;
    virtual Sequence<T>* CreateFromArray(const T* items, int count) const = 0;

    IEnumerator<T>* GetEnumerator() const override;

    template<typename Func>
    Sequence<T>* Map(Func f) const {
        Sequence<T>* result = CreateEmpty();
        for (int i = 0; i < GetLength(); i++) {
            Sequence<T>* updated = result->Append(f(Get(i)));
            if (updated != result) {
                delete result;
            }
            result = updated;
        }
        return result;
    }

    template<typename Func>
    Sequence<T>* FlatMap(Func f) const {
        Sequence<T>* result = CreateEmpty();
        for (int i = 0; i < GetLength(); i++) {
            Sequence<T>* expanded = f(Get(i));
            Sequence<T>* updated = result->Concat(expanded);
            if (updated != result) {
                delete result;
            }
            result = updated;
            delete expanded;
        }
        return result;
    }

    template<typename Func, typename U>
    U Reduce(Func f, U initial) const {
        U result = initial;
        for (int i = 0; i < GetLength(); i++) {
            result = f(Get(i), result);
        }
        return result;
    }

    template<typename Predicate>
    Sequence<T>* Where(Predicate pred) const {
        Sequence<T>* result = CreateEmpty();
        for (int i = 0; i < GetLength(); i++) {
            T item = Get(i);
            if (pred(item)) {
                Sequence<T>* updated = result->Append(item);
                if (updated != result) {
                    delete result;
                }
                result = updated;
            }
        }
        return result;
    }

    template<typename Predicate>
    T Find(Predicate pred) const {
        for (int i = 0; i < GetLength(); i++) {
            T item = Get(i);
            if (pred(item)) {
                return item;
            }
        }
        throw ElementNotFound();
    }

    template<typename U>
    Sequence<std::tuple<T, U>>* Zip(Sequence<U>* other) const {
        int minLen = std::min(GetLength(), other->GetLength());
        auto* result = new ArraySequence<std::tuple<T, U>>();
        for (int i = 0; i < minLen; i++) {
            result->Append(std::make_tuple(Get(i), other->Get(i)));
        }
        return result;
    }

    template<
        typename Tuple = T,
        typename First = std::tuple_element_t<0, Tuple>,
        typename Second = std::tuple_element_t<1, Tuple>
    >
    std::pair<Sequence<First>*, Sequence<Second>*> Unzip() const {
        auto* first = new ArraySequence<First>();
        auto* second = new ArraySequence<Second>();

        for (int i = 0; i < GetLength(); i++) {
            Tuple item = Get(i);
            first->Append(std::get<0>(item));
            second->Append(std::get<1>(item));
        }

        return {first, second};
    }

    template<typename Predicate>
    Sequence<Sequence<T>*>* Split(Predicate pred) const {
        auto* result = new ArraySequence<Sequence<T>*>();
        Sequence<T>* current = CreateEmpty();

        for (int i = 0; i < GetLength(); i++) {
            T item = Get(i);
            if (pred(item)) {
                if (current->GetLength() > 0) {
                    result->Append(current);
                    current = CreateEmpty();
                }
            } else {
                Sequence<T>* updated = current->Append(item);
                if (updated != current) {
                    delete current;
                }
                current = updated;
            }
        }

        if (current->GetLength() > 0) {
            result->Append(current);
        } else {
            delete current;
        }

        return result;
    }

    Sequence<T>* Slice(int index, int count, Sequence<T>* replacement = nullptr) const {
        if (count < 0) {
            throw std::invalid_argument("count must be non-negative");
        }

        int actualIndex = index;
        if (actualIndex < 0) {
            actualIndex = GetLength() + actualIndex;
        }

        if (actualIndex < 0 || actualIndex > GetLength()) {
            throw IndexOutOfRange(index, GetLength());
        }

        Sequence<T>* result = CreateEmpty();

        for (int i = 0; i < actualIndex; i++) {
            Sequence<T>* updated = result->Append(Get(i));
            if (updated != result) {
                delete result;
            }
            result = updated;
        }

        if (replacement != nullptr) {
            for (int i = 0; i < replacement->GetLength(); i++) {
                Sequence<T>* updated = result->Append(replacement->Get(i));
                if (updated != result) {
                    delete result;
                }
                result = updated;
            }
        }

        int skipUntil = actualIndex + count;
        if (skipUntil < actualIndex) {
            skipUntil = GetLength();
        }

        for (int i = std::min(skipUntil, GetLength()); i < GetLength(); i++) {
            Sequence<T>* updated = result->Append(Get(i));
            if (updated != result) {
                delete result;
            }
            result = updated;
        }

        return result;
    }

    Option<T> TryGet(int index) const {
        if (index < 0 || index >= GetLength()) {
            return Option<T>::None();
        }
        return Option<T>::Some(Get(index));
    }

    template<typename Predicate>
    Option<T> TryFind(Predicate pred) const {
        for (int i = 0; i < GetLength(); i++) {
            T item = Get(i);
            if (pred(item)) {
                return Option<T>::Some(item);
            }
        }
        return Option<T>::None();
    }

    Option<T> TryFirst() const {
        if (GetLength() == 0) {
            return Option<T>::None();
        }
        return Option<T>::Some(GetFirst());
    }

    Option<T> TryLast() const {
        if (GetLength() == 0) {
            return Option<T>::None();
        }
        return Option<T>::Some(GetLast());
    }

    T operator[](int index) const {
        return Get(index);
    }

    static Sequence<T>* From(const T* arr, int count) {
        ArraySequence<T> factory;
        return factory.CreateFromArray(arr, count);
    }
};

template<class T>
bool SequenceEnumerator<T>::MoveNext() {
    if (index + 1 >= sequence->GetLength()) {
        return false;
    }
    index++;
    return true;
}

template<class T>
T SequenceEnumerator<T>::GetCurrent() const {
    if (index < 0 || index >= sequence->GetLength()) {
        throw IndexOutOfRange(index, sequence->GetLength());
    }
    return sequence->Get(index);
}

template<class T>
void SequenceEnumerator<T>::Reset() {
    index = -1;
}

template<class T>
IEnumerator<T>* Sequence<T>::GetEnumerator() const {
    return new SequenceEnumerator<T>(this);
}
