#pragma once

#include "Mutable/MutableArraySequence.hpp"
#include "Sequence.hpp"
#include <type_traits>

template<class T, class Func, class U = std::invoke_result_t<Func, T, int>>
Sequence<U>* MapIndexed(const Sequence<T>& sequence, Func mapper) {
    Sequence<U>* result = new MutableArraySequence<U>();
    for (int i = 0; i < sequence.GetLength(); i++) {
        Sequence<U>* updated = result->Append(mapper(sequence.Get(i), i));
        if (updated != result) {
            delete result;
            result = updated;
        }
    }
    return result;
}
