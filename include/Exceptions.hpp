#pragma once
#include <exception>
#include <string>

class IndexOutOfRange : public std::exception {
private:
    std::string message;
public:
    IndexOutOfRange(int index, int size) {
        message = "IndexOutOfRange: index=" + std::to_string(index) + 
                  ", size=" + std::to_string(size);
    }
    const char* what() const noexcept override {
        return message.c_str();
    }
};

class EmptyCollection : public std::exception {
private:
    std::string message = "EmptyCollection: collection is empty";
public:
    const char* what() const noexcept override {
        return message.c_str();
    }
};

class ElementNotFound : public std::exception {
private:
    std::string message = "ElementNotFound: no matching element was found";
public:
    const char* what() const noexcept override {
        return message.c_str();
    }
};
