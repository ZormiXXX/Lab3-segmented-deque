#pragma once
#include "Exceptions.hpp"

template<class T>
class DynamicArray {
private:
    T* data;
    int size;
    int capacity;
    
    void EnsureCapacity(int minCapacity) {
        if (capacity >= minCapacity) return;
        
        int newCapacity = capacity == 0 ? 4 : capacity * 2;
        while (newCapacity < minCapacity) newCapacity *= 2;
        
        T* newData = new T[newCapacity];
        for (int i = 0; i < size; i++) {
            newData[i] = data[i];
        }
        delete[] data;
        data = newData;
        capacity = newCapacity;
    }
    
public:
    DynamicArray() : data(nullptr), size(0), capacity(0) {}
    
    DynamicArray(const T* items, int count) : size(count), capacity(count) {
        if (count < 0) {
            throw IndexOutOfRange(count, 0);
        }
        data = new T[count];
        for (int i = 0; i < count; i++) {
            data[i] = items[i];
        }
    }
    
    DynamicArray(int size) : size(size), capacity(size) {
        if (size < 0) {
            throw IndexOutOfRange(size, 0);
        }
        data = new T[size];
        for (int i = 0; i < size; i++) {
            data[i] = T();
        }
    }
    
    DynamicArray(const DynamicArray<T>& other) : size(other.size), capacity(other.capacity) {
        data = new T[capacity];
        for (int i = 0; i < size; i++) {
            data[i] = other.data[i];
        }
    }
    
    ~DynamicArray() {
        delete[] data;
    }
    
    T Get(int index) const {
        if (index < 0 || index >= size) {
            throw IndexOutOfRange(index, size);
        }
        return data[index];
    }
    
    void Set(int index, T value) {
        if (index < 0 || index >= size) {
            throw IndexOutOfRange(index, size);
        }
        data[index] = value;
    }
    
    int GetSize() const {
        return size;
    }
    
    void Resize(int newSize) {
        if (newSize < 0) throw IndexOutOfRange(newSize, 0);
        
        EnsureCapacity(newSize);
        
        if (newSize > size) {
            for (int i = size; i < newSize; i++) {
                data[i] = T();
            }
        }
        size = newSize;
    }
    
    T& operator[](int index) {
        if (index < 0 || index >= size) {
            throw IndexOutOfRange(index, size);
        }
        return data[index];
    }
    
    const T& operator[](int index) const {
        if (index < 0 || index >= size) {
            throw IndexOutOfRange(index, size);
        }
        return data[index];
    }
};
