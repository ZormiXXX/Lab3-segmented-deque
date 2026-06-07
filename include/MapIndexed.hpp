#pragma once

#include "Mutable/MutableArraySequence.hpp"
#include "Sequence.hpp"

template<class T, class U, class Func>
Sequence<U>* MapIndexed(const Sequence<T>& sequence, Func mapper) {
    Sequence<U>* result = new MutableArraySequence<U>();
    IEnumerator<T>* enumerator = sequence.GetEnumerator();
    int index = 0;

    while (enumerator->MoveNext()) {
        Sequence<U>* updated = result->Append(mapper(enumerator->GetCurrent(), index++));
        if (updated != result) {
            delete result;
            result = updated;
        }
    }

    delete enumerator;
    return result;
}
