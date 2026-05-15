#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include "../include/Immutable/ImmutableArraySequence.hpp"
#include "../include/Immutable/ImmutableListSequence.hpp"
#include "../include/Mutable/MutableArraySequence.hpp"
#include "../include/Mutable/MutableListSequence.hpp"

#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_RED     "\033[31m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_BOLD    "\033[1m"

struct TestResult {
    std::string name;
    bool passed;
    int assertions;
    int passedAssertions;
};

std::vector<TestResult> allResults;
int totalAssertions = 0;
int totalPassed = 0;
bool currentTestPassed = false;
int currentTestAssertions = 0;

#define TEST(name) void name()

#define ASSERT_EQ(expected, actual, msg) do { \
    totalAssertions++; \
    currentTestAssertions++; \
    if ((expected) == (actual)) { \
        totalPassed++; \
        std::cout << COLOR_GREEN "  OK  " COLOR_RESET << msg << std::endl; \
        std::cout << "    Ожидалось: " << (expected) << ", Получено: " << (actual) << std::endl; \
    } else { \
        std::cout << COLOR_RED "  FAIL" COLOR_RESET << " " << msg << std::endl; \
        std::cout << "    " COLOR_YELLOW "Ожидалось: " COLOR_RESET << (expected) << std::endl; \
        std::cout << "    " COLOR_RED "Получено: " COLOR_RESET << (actual) << std::endl; \
        currentTestPassed = false; \
    } \
} while (0)

#define ASSERT_TRUE(condition, msg) do { \
    totalAssertions++; \
    currentTestAssertions++; \
    if (condition) { \
        totalPassed++; \
        std::cout << COLOR_GREEN "  OK  " COLOR_RESET << msg << std::endl; \
    } else { \
        std::cout << COLOR_RED "  FAIL" COLOR_RESET << " " << msg << std::endl; \
        currentTestPassed = false; \
    } \
} while (0)

#define ASSERT_THROWS(expr, exc, msg) do { \
    totalAssertions++; \
    currentTestAssertions++; \
    bool caught = false; \
    try { \
        expr; \
    } catch (const exc&) { \
        caught = true; \
    } catch (...) { \
        caught = false; \
    } \
    if (caught) { \
        totalPassed++; \
        std::cout << COLOR_GREEN "  OK  " COLOR_RESET << msg << std::endl; \
    } else { \
        std::cout << COLOR_RED "  FAIL" COLOR_RESET << " " << msg << std::endl; \
        std::cout << "    Ожидалось исключение " << #exc << std::endl; \
        currentTestPassed = false; \
    } \
} while (0)

#define RUN_TEST(name) do { \
    std::cout << COLOR_BOLD COLOR_CYAN "\n════════════════════════════════════════" COLOR_RESET << std::endl; \
    std::cout << COLOR_BOLD COLOR_BLUE " ТЕСТ: " COLOR_RESET << #name << std::endl; \
    std::cout << COLOR_CYAN "════════════════════════════════════════" COLOR_RESET << std::endl; \
    currentTestPassed = true; \
    currentTestAssertions = 0; \
    try { \
        name(); \
    } catch (const Exception& e) { \
        std::cout << COLOR_RED "  FAIL" COLOR_RESET << " Непредвиденное исключение: " << e.what() << std::endl; \
        currentTestPassed = false; \
    } catch (...) { \
        std::cout << COLOR_RED "  FAIL" COLOR_RESET << " Непредвиденное исключение неизвестного типа" << std::endl; \
        currentTestPassed = false; \
    } \
    allResults.push_back({#name, currentTestPassed, currentTestAssertions, currentTestPassed ? currentTestAssertions : 0}); \
    std::cout << COLOR_BOLD "\n Результат: " COLOR_RESET; \
    if (currentTestPassed) { \
        std::cout << COLOR_GREEN "ПРОЙДЕН" COLOR_RESET << std::endl; \
    } else { \
        std::cout << COLOR_RED "ПРОВАЛЕН" COLOR_RESET << std::endl; \
    } \
} while (0)

void PrintHeader(const std::string& text) {
    std::cout << COLOR_BOLD COLOR_YELLOW "\n┌─────────────────────────────────────────────────┐" COLOR_RESET << std::endl;
    std::cout << COLOR_BOLD COLOR_YELLOW "│ " COLOR_RESET << std::left << std::setw(47) << text << COLOR_BOLD COLOR_YELLOW " │" COLOR_RESET << std::endl;
    std::cout << COLOR_BOLD COLOR_YELLOW "└─────────────────────────────────────────────────┘" COLOR_RESET << std::endl;
}

void PrintSubHeader(const std::string& text) {
    std::cout << COLOR_CYAN "\n  -- " COLOR_RESET << text << COLOR_CYAN " --" COLOR_RESET << std::endl;
}

template<class T>
void PrintSequence(const std::string& label, const Sequence<T>* seq) {
    std::cout << "    " << label << ": [";
    IEnumerator<T>* enumerator = seq->GetEnumerator();
    bool first = true;
    while (enumerator->MoveNext()) {
        if (!first) {
            std::cout << ", ";
        }
        std::cout << enumerator->GetCurrent();
        first = false;
    }
    delete enumerator;
    std::cout << "]" << std::endl;
}

void PrintTupleSequence(const std::string& label, const Sequence<Tuple<int, char>>* seq) {
    std::cout << "    " << label << ": [";
    IEnumerator<Tuple<int, char>>* enumerator = seq->GetEnumerator();
    bool first = true;
    while (enumerator->MoveNext()) {
        const Tuple<int, char>& item = enumerator->GetCurrent();
        if (!first) {
            std::cout << ", ";
        }
        std::cout << "(" << item.first << ", " << item.second << ")";
        first = false;
    }
    delete enumerator;
    std::cout << "]" << std::endl;
}

TEST(TestDynamicArray_Create) {
    PrintSubHeader("Создание динамического массива");
    int arr[] = {10, 20, 30, 40, 50};
    DynamicArray<int> da(arr, 5);
    ASSERT_EQ(5, da.GetSize(), "Размер массива");
    ASSERT_EQ(10, da.Get(0), "Первый элемент");
    ASSERT_EQ(50, da.Get(4), "Последний элемент");
}

TEST(TestDynamicArray_Resize) {
    PrintSubHeader("Изменение размера массива");
    DynamicArray<int> da(3);
    da.Set(0, 1);
    da.Set(1, 2);
    da.Set(2, 3);
    da.Resize(5);
    ASSERT_EQ(5, da.GetSize(), "Размер после увеличения");
    ASSERT_EQ(1, da.Get(0), "Первый элемент сохранён");
    ASSERT_EQ(0, da.Get(4), "Новые элементы инициализированы");
}

TEST(TestDynamicArray_ReserveAndEnumerator) {
    PrintSubHeader("Reserve и прямой Enumerator DynamicArray");
    DynamicArray<int> da;
    da.Reserve(10);
    ASSERT_TRUE(da.GetCapacity() >= 10, "Reserve заранее выделяет ёмкость");

    da.Append(5);
    da.Append(10);
    da.Append(15);

    IEnumerator<int>* enumerator = da.GetEnumerator();
    ASSERT_TRUE(enumerator->MoveNext(), "Enumerator DynamicArray переходит к первому элементу");
    ASSERT_EQ(5, enumerator->GetCurrent(), "Enumerator DynamicArray читает первый элемент");
    ASSERT_TRUE(enumerator->MoveNext(), "Enumerator DynamicArray переходит ко второму элементу");
    ASSERT_EQ(10, enumerator->GetCurrent(), "Enumerator DynamicArray читает второй элемент");
    delete enumerator;
}

TEST(TestDynamicArray_Exception) {
    PrintSubHeader("Исключения DynamicArray");
    DynamicArray<int> da(2);
    ASSERT_THROWS(da.Get(-1), IndexOutOfRange, "Отрицательный индекс");
    ASSERT_THROWS(da.Set(2, 10), IndexOutOfRange, "Индекс за границей");
}

TEST(TestLinkedList_Basic) {
    PrintSubHeader("Базовые операции LinkedList");
    int arr[] = {1, 2, 3};
    LinkedList<int> list(arr, 3);
    list.Append(4);
    list.Prepend(0);
    ASSERT_EQ(5, list.GetLength(), "Длина после Append и Prepend");
    ASSERT_EQ(0, list.GetFirst(), "Первый элемент");
    ASSERT_EQ(4, list.GetLast(), "Последний элемент");
}

TEST(TestLinkedList_Contracts) {
    PrintSubHeader("Контракты и исключения LinkedList");
    LinkedList<int> empty;
    ASSERT_THROWS(empty.GetFirst(), IndexOutOfRange, "GetFirst у пустого списка");
    ASSERT_THROWS(empty.GetLast(), IndexOutOfRange, "GetLast у пустого списка");

    int arr[] = {1, 2, 3};
    LinkedList<int> list(arr, 3);
    ASSERT_THROWS(list.InsertAt(9, 3), IndexOutOfRange, "InsertAt в конец запрещён по ТЗ");
}

TEST(TestMutableArraySequence) {
    PrintSubHeader("MutableArraySequence");
    int arr[] = {1, 2, 3};
    MutableArraySequence<int> seq(arr, 3);
    seq.Append(4);
    seq.Prepend(0);
    ASSERT_EQ(5, seq.GetLength(), "Длина после мутаций");
    ASSERT_EQ(0, seq.Get(0), "Первый элемент");
    ASSERT_EQ(4, seq.Get(4), "Последний элемент");
}

TEST(TestImmutableArraySequence) {
    PrintSubHeader("ImmutableArraySequence");
    int arr[] = {1, 2, 3};
    auto* original = new ImmutableArraySequence<int>(arr, 3);
    Sequence<int>* appended = original->Append(4);
    ASSERT_EQ(3, original->GetLength(), "Оригинал не меняется");
    ASSERT_EQ(4, appended->GetLength(), "Новая последовательность имеет новый размер");
    delete original;
    delete appended;
}

TEST(TestMutableListSequence) {
    PrintSubHeader("MutableListSequence");
    int arr[] = {1, 2, 3};
    MutableListSequence<int> seq(arr, 3);
    seq.Append(4);
    seq.Prepend(0);
    ASSERT_EQ(5, seq.GetLength(), "Длина после мутаций");
    ASSERT_EQ(0, seq.GetFirst(), "Первый элемент");
    ASSERT_EQ(4, seq.GetLast(), "Последний элемент");
}

TEST(TestImmutableListSequence) {
    PrintSubHeader("ImmutableListSequence");
    int arr[] = {1, 2, 3};
    auto* original = new ImmutableListSequence<int>(arr, 3);
    Sequence<int>* prepended = original->Prepend(0);
    ASSERT_EQ(3, original->GetLength(), "Оригинал списка не меняется");
    ASSERT_EQ(4, prepended->GetLength(), "Возвращается новая последовательность");
    ASSERT_EQ(0, prepended->GetFirst(), "Новый первый элемент");
    delete original;
    delete prepended;
}

TEST(TestSequence_InsertAtContract) {
    PrintSubHeader("Контракт InsertAt для последовательностей");
    int arr[] = {1, 2, 3};
    MutableArraySequence<int> arraySeq(arr, 3);
    MutableListSequence<int> listSeq(arr, 3);
    ASSERT_THROWS(arraySeq.InsertAt(10, 3), IndexOutOfRange, "ArraySequence: вставка в конец запрещена");
    ASSERT_THROWS(listSeq.InsertAt(10, 3), IndexOutOfRange, "ListSequence: вставка в конец запрещена");
}

TEST(TestConcatAndSubsequence) {
    PrintSubHeader("Concat и GetSubsequence");
    int arr1[] = {1, 2, 3};
    int arr2[] = {4, 5, 6};
    MutableArraySequence<int> seq1(arr1, 3);
    MutableListSequence<int> seq2(arr2, 3);
    seq1.Concat(&seq2);
    ASSERT_EQ(6, seq1.GetLength(), "Длина после Concat");
    ASSERT_EQ(6, seq1.Get(5), "Последний элемент после Concat");

    Sequence<int>* sub = seq1.GetSubsequence(2, 4);
    ASSERT_EQ(3, sub->GetLength(), "Длина подпоследовательности");
    ASSERT_EQ(3, sub->Get(0), "Первый элемент подпоследовательности");
    ASSERT_EQ(5, sub->Get(2), "Последний элемент подпоследовательности");
    delete sub;
}

TEST(TestSelfConcat) {
    PrintSubHeader("Concat последовательности с самой собой");
    int arr[] = {1, 2, 3};

    MutableArraySequence<int> arraySeq(arr, 3);
    arraySeq.Concat(&arraySeq);
    ASSERT_EQ(6, arraySeq.GetLength(), "MutableArraySequence удваивает длину один раз");
    ASSERT_EQ(1, arraySeq.Get(3), "MutableArraySequence копирует исходный первый элемент");
    ASSERT_EQ(3, arraySeq.Get(5), "MutableArraySequence копирует исходный последний элемент");

    MutableListSequence<int> listSeq(arr, 3);
    listSeq.Concat(&listSeq);
    ASSERT_EQ(6, listSeq.GetLength(), "MutableListSequence удваивает длину один раз");
    ASSERT_EQ(1, listSeq.Get(3), "MutableListSequence копирует исходный первый элемент");
    ASSERT_EQ(3, listSeq.Get(5), "MutableListSequence копирует исходный последний элемент");

    ImmutableArraySequence<int> immutableArray(arr, 3);
    Sequence<int>* arrayResult = immutableArray.Concat(&immutableArray);
    ASSERT_EQ(3, immutableArray.GetLength(), "ImmutableArraySequence сохраняет оригинал");
    ASSERT_EQ(6, arrayResult->GetLength(), "ImmutableArraySequence возвращает удвоенную копию");

    ImmutableListSequence<int> immutableList(arr, 3);
    Sequence<int>* listResult = immutableList.Concat(&immutableList);
    ASSERT_EQ(3, immutableList.GetLength(), "ImmutableListSequence сохраняет оригинал");
    ASSERT_EQ(6, listResult->GetLength(), "ImmutableListSequence возвращает удвоенную копию");

    delete arrayResult;
    delete listResult;
}

TEST(TestNullConcatContract) {
    PrintSubHeader("Контракт Concat с nullptr");
    int arr[] = {1, 2, 3};
    MutableArraySequence<int> arraySeq(arr, 3);
    MutableListSequence<int> listSeq(arr, 3);

    ASSERT_THROWS(arraySeq.Concat(nullptr), InvalidArgument, "ArraySequence не принимает nullptr в Concat");
    ASSERT_THROWS(listSeq.Concat(nullptr), InvalidArgument, "ListSequence не принимает nullptr в Concat");
}

TEST(TestSlice) {
    PrintSubHeader("Slice");
    int arr[] = {1, 2, 3, 4, 5};
    int replacementArr[] = {9, 10};
    ImmutableArraySequence<int> seq(arr, 5);
    Sequence<int>* replacement = Sequence<int>::From(replacementArr, 2);
    Sequence<int>* sliced = seq.Slice(1, 2, replacement);
    ASSERT_EQ(5, sliced->GetLength(), "Длина после Slice");
    ASSERT_EQ(1, sliced->Get(0), "Первый элемент");
    ASSERT_EQ(9, sliced->Get(1), "Первый вставленный элемент");
    ASSERT_EQ(10, sliced->Get(2), "Второй вставленный элемент");
    ASSERT_EQ(5, sliced->Get(4), "Последний элемент");

    Sequence<int>* slicedFromEnd = seq.Slice(-2, 1);
    ASSERT_EQ(4, slicedFromEnd->GetLength(), "Slice с отрицательным индексом");
    ASSERT_EQ(5, slicedFromEnd->Get(3), "Последний элемент после удаления");

    Sequence<int>* slicedToEnd = seq.Slice(3, 20);
    ASSERT_EQ(3, slicedToEnd->GetLength(), "Slice ограничивает удаление концом последовательности");
    ASSERT_EQ(3, slicedToEnd->GetLast(), "Slice сохраняет элементы до удаляемого диапазона");

    delete replacement;
    delete sliced;
    delete slicedFromEnd;
    delete slicedToEnd;
}

TEST(TestMapAndFlatMap) {
    PrintSubHeader("Map и FlatMap");
    int arr[] = {1, 2, 3};
    ImmutableListSequence<int> seq(arr, 3);
    Sequence<int>* mapped = seq.Map([](int value) { return value * 2; });
    ASSERT_EQ(2, mapped->Get(0), "Map умножает первый элемент");
    ASSERT_EQ(6, mapped->Get(2), "Map умножает последний элемент");

    Sequence<int>* flatMapped = seq.FlatMap([](int value) {
        int items[] = {value, value * 10};
        return Sequence<int>::From(items, 2);
    });
    ASSERT_EQ(6, flatMapped->GetLength(), "FlatMap разворачивает подпоследовательности");
    ASSERT_EQ(1, flatMapped->Get(0), "Первый элемент FlatMap");
    ASSERT_EQ(30, flatMapped->Get(5), "Последний элемент FlatMap");

    delete mapped;
    delete flatMapped;
}

TEST(TestReduceOrder) {
    PrintSubHeader("Порядок Reduce по PDF");
    int arr[] = {1, 2, 3};
    ImmutableArraySequence<int> seq(arr, 3);
    int result = seq.Reduce([](int value, int acc) { return 2 * value + 3 * acc; }, 4);
    ASSERT_EQ(144, result, "Reduce считает по формуле f(a_n, ... f(a_1, c))");
}

TEST(TestWhereFindTryFind) {
    PrintSubHeader("Where, Find и TryFind");
    int arr[] = {1, 2, 3, 4, 5, 6};
    ImmutableArraySequence<int> seq(arr, 6);
    Sequence<int>* filtered = seq.Where([](int value) { return value % 2 == 0; });
    ASSERT_EQ(3, filtered->GetLength(), "Количество чётных элементов");
    ASSERT_EQ(2, seq.Find([](int value) { return value == 2; }), "Find возвращает найденный элемент");
    ASSERT_THROWS(seq.Find([](int value) { return value == 99; }), ElementNotFound, "Find выбрасывает исключение, если элемент не найден");

    Option<int> found = seq.TryFind([](int value) { return value == 4; });
    Option<int> missing = seq.TryFind([](int value) { return value == 99; });
    ASSERT_TRUE(found.IsSome(), "TryFind возвращает Some");
    ASSERT_EQ(4, found.GetValue(), "TryFind возвращает корректное значение");
    ASSERT_TRUE(missing.IsNone(), "TryFind возвращает None");

    delete filtered;
}

TEST(TestZipAndUnzip) {
    PrintSubHeader("Zip и Unzip");
    int numbers[] = {1, 2, 3};
    char letters[] = {'A', 'B', 'C', 'D'};
    MutableArraySequence<int> seq1(numbers, 3);
    Sequence<char>* seq2 = Sequence<char>::From(letters, 4);

    Sequence<Tuple<int, char>>* zipped = seq1.Zip(seq2);
    PrintTupleSequence("Zip", zipped);
    ASSERT_EQ(3, zipped->GetLength(), "Zip обрезает по минимальной длине");
    ASSERT_EQ(1, zipped->Get(0).first, "Первый числовой элемент");
    ASSERT_EQ('C', zipped->Get(2).second, "Последний символьный элемент");

    auto [unzippedFirst, unzippedSecond] = zipped->Unzip();
    ASSERT_EQ(3, unzippedFirst->GetLength(), "Unzip восстанавливает длину первой последовательности");
    ASSERT_EQ(3, unzippedSecond->GetLength(), "Unzip восстанавливает длину второй последовательности");
    ASSERT_EQ(2, unzippedFirst->Get(1), "Восстановлен второй числовой элемент");
    ASSERT_EQ('B', unzippedSecond->Get(1), "Восстановлен второй символьный элемент");

    delete seq2;
    delete zipped;
    delete unzippedFirst;
    delete unzippedSecond;
}

TEST(TestSplit) {
    PrintSubHeader("Split");
    int arr[] = {1, 2, 3, 4, 3, 5};
    MutableListSequence<int> seq(arr, 6);
    Sequence<Sequence<int>*>* split = seq.Split([](int value) { return value == 3; });
    ASSERT_EQ(3, split->GetLength(), "Количество фрагментов");
    ASSERT_EQ(2, split->Get(0)->GetLength(), "Длина первого фрагмента");
    ASSERT_EQ(4, split->Get(1)->Get(0), "Первый элемент второго фрагмента");
    ASSERT_EQ(5, split->Get(2)->Get(0), "Единственный элемент третьего фрагмента");

    for (int i = 0; i < split->GetLength(); i++) {
        delete split->Get(i);
    }
    delete split;
}

TEST(TestOptionAndTrySemantics) {
    PrintSubHeader("Option и Try-семантика");
    int arr[] = {10, 20, 30};
    ImmutableArraySequence<int> seq(arr, 3);
    Option<int> first = seq.TryFirst();
    Option<int> last = seq.TryLast();
    Option<int> missing = seq.TryGet(99);
    ASSERT_TRUE(first.IsSome(), "TryFirst возвращает Some");
    ASSERT_EQ(10, first.GetValue(), "TryFirst возвращает первый элемент");
    ASSERT_TRUE(last.IsSome(), "TryLast возвращает Some");
    ASSERT_EQ(30, last.GetValue(), "TryLast возвращает последний элемент");
    ASSERT_TRUE(missing.IsNone(), "TryGet вне диапазона возвращает None");

    ImmutableListSequence<int> empty;
    ASSERT_TRUE(empty.TryFirst().IsNone(), "TryFirst у пустой последовательности");
    ASSERT_TRUE(empty.TryLast().IsNone(), "TryLast у пустой последовательности");
}

TEST(TestEnumerator) {
    PrintSubHeader("IEnumerable и IEnumerator");
    int arr[] = {7, 8, 9};
    MutableListSequence<int> seq(arr, 3);
    IEnumerator<int>* enumerator = seq.GetEnumerator();
    ASSERT_TRUE(enumerator->MoveNext(), "Первый переход успешен");
    ASSERT_EQ(7, enumerator->GetCurrent(), "Первый элемент перечислителя");
    ASSERT_TRUE(enumerator->MoveNext(), "Второй переход успешен");
    ASSERT_EQ(8, enumerator->GetCurrent(), "Второй элемент перечислителя");
    enumerator->Reset();
    ASSERT_TRUE(enumerator->MoveNext(), "После Reset снова можно итерироваться");
    ASSERT_EQ(7, enumerator->GetCurrent(), "Reset возвращает перечислитель в начало");
    delete enumerator;

    MutableArraySequence<int> arraySeq(arr, 3);
    IEnumerator<int>* arrayEnumerator = arraySeq.GetEnumerator();
    ASSERT_TRUE(arrayEnumerator->MoveNext(), "ArraySequence использует быстрый Enumerator");
    ASSERT_EQ(7, arrayEnumerator->GetCurrent(), "ArraySequence Enumerator читает первый элемент");
    delete arrayEnumerator;
}

TEST(TestOperatorOverload) {
    PrintSubHeader("Перегрузка оператора []");
    int arr[] = {10, 20, 30};
    MutableArraySequence<int> seq(arr, 3);
    ASSERT_EQ(10, seq[0], "Чтение через operator[]");
    seq[1] = 999;
    ASSERT_EQ(999, seq.Get(1), "Запись через operator[]");
}

TEST(TestFrom) {
    PrintSubHeader("From");
    int arr[] = {5, 10, 15, 20};
    Sequence<int>* seq = Sequence<int>::From(arr, 4);
    ASSERT_EQ(4, seq->GetLength(), "Длина созданной последовательности");
    ASSERT_EQ(5, seq->Get(0), "Первый элемент");
    ASSERT_EQ(20, seq->Get(3), "Последний элемент");
    delete seq;
}

void PrintSummary() {
    std::cout << COLOR_BOLD COLOR_CYAN "\n\n╔════════════════════════════════════════════════════════╗" COLOR_RESET << std::endl;
    std::cout << COLOR_BOLD COLOR_CYAN "║" COLOR_RESET << std::left << std::setw(58) << "                    ИТОГОВЫЙ ОТЧЁТ" << COLOR_BOLD COLOR_CYAN " ║" COLOR_RESET << std::endl;
    std::cout << COLOR_BOLD COLOR_CYAN "╠════════════════════════════════════════════════════════╣" COLOR_RESET << std::endl;

    int passedTests = 0;
    int failedTests = 0;

    for (const auto& result : allResults) {
        std::cout << COLOR_BOLD COLOR_CYAN "║" COLOR_RESET << " ";
        if (result.passed) {
            std::cout << COLOR_GREEN << "OK" << COLOR_RESET;
            passedTests++;
        } else {
            std::cout << COLOR_RED << "NO" << COLOR_RESET;
            failedTests++;
        }
        std::cout << " " << std::left << std::setw(44) << result.name;
        std::cout << " [" << result.passedAssertions << "/" << result.assertions << "]" << std::endl;
    }

    std::cout << COLOR_BOLD COLOR_CYAN "╠════════════════════════════════════════════════════════╣" COLOR_RESET << std::endl;

    int percentage = totalAssertions > 0 ? (totalPassed * 100) / totalAssertions : 0;

    std::cout << COLOR_BOLD COLOR_CYAN "║" COLOR_RESET << " Всего тестов:      " << std::left << std::setw(37) << (passedTests + failedTests);
    std::cout << COLOR_BOLD COLOR_CYAN " ║" COLOR_RESET << std::endl;
    std::cout << COLOR_BOLD COLOR_CYAN "║" COLOR_RESET << " Пройдено:          " << COLOR_GREEN << std::left << std::setw(37) << passedTests << COLOR_BOLD COLOR_CYAN " ║" COLOR_RESET << std::endl;
    std::cout << COLOR_BOLD COLOR_CYAN "║" COLOR_RESET << " Провалено:         " << COLOR_RED << std::left << std::setw(37) << failedTests << COLOR_BOLD COLOR_CYAN " ║" COLOR_RESET << std::endl;
    std::cout << COLOR_BOLD COLOR_CYAN "║" COLOR_RESET << " Всего проверок:    " << std::left << std::setw(37) << totalAssertions << COLOR_BOLD COLOR_CYAN " ║" COLOR_RESET << std::endl;
    std::cout << COLOR_BOLD COLOR_CYAN "║" COLOR_RESET << " Успешных проверок: " << COLOR_GREEN << std::left << std::setw(37) << totalPassed << COLOR_BOLD COLOR_CYAN " ║" COLOR_RESET << std::endl;

    std::string status = (failedTests == 0) ? COLOR_GREEN "ВСЕ ТЕСТЫ ПРОЙДЕНЫ!" : COLOR_RED "ЕСТЬ ПРОВАЛЫ!";
    std::cout << COLOR_BOLD COLOR_CYAN "╠════════════════════════════════════════════════════════╣" COLOR_RESET << std::endl;
    std::cout << COLOR_BOLD COLOR_CYAN "║" COLOR_RESET << " Статус: " << std::left << std::setw(49) << status << COLOR_BOLD COLOR_CYAN " ║" COLOR_RESET << std::endl;
    std::cout << COLOR_BOLD COLOR_CYAN "║" COLOR_RESET << " Процент успеха: " << std::left << std::setw(49) << (percentage >= 80 ? COLOR_GREEN : COLOR_YELLOW) << percentage << "%" << COLOR_RESET << COLOR_BOLD COLOR_CYAN " ║" COLOR_RESET << std::endl;
    std::cout << COLOR_BOLD COLOR_CYAN "╚════════════════════════════════════════════════════════╝" COLOR_RESET << std::endl;
}

void RunAllTests() {
    allResults.clear();
    totalAssertions = 0;
    totalPassed = 0;

    std::cout << COLOR_BOLD COLOR_BLUE "\n";
    std::cout << "╔══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                                                          ║" << std::endl;
    std::cout << "║        ЛАБОРАТОРНАЯ РАБОТА №2 - МОДУЛЬНЫЕ ТЕСТЫ          ║" << std::endl;
    std::cout << "║                                                          ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << COLOR_RESET << std::endl;

    PrintHeader("ТЕСТИРОВАНИЕ DYNAMIC ARRAY");
    RUN_TEST(TestDynamicArray_Create);
    RUN_TEST(TestDynamicArray_Resize);
    RUN_TEST(TestDynamicArray_ReserveAndEnumerator);
    RUN_TEST(TestDynamicArray_Exception);

    PrintHeader("ТЕСТИРОВАНИЕ LINKED LIST");
    RUN_TEST(TestLinkedList_Basic);
    RUN_TEST(TestLinkedList_Contracts);

    PrintHeader("ТЕСТИРОВАНИЕ ARRAY/LIST SEQUENCE");
    RUN_TEST(TestMutableArraySequence);
    RUN_TEST(TestImmutableArraySequence);
    RUN_TEST(TestMutableListSequence);
    RUN_TEST(TestImmutableListSequence);
    RUN_TEST(TestSequence_InsertAtContract);
    RUN_TEST(TestConcatAndSubsequence);
    RUN_TEST(TestSelfConcat);
    RUN_TEST(TestNullConcatContract);
    RUN_TEST(TestSlice);

    PrintHeader("ТЕСТИРОВАНИЕ MAP-REDUCE");
    RUN_TEST(TestMapAndFlatMap);
    RUN_TEST(TestReduceOrder);
    RUN_TEST(TestWhereFindTryFind);
    RUN_TEST(TestZipAndUnzip);
    RUN_TEST(TestSplit);

    PrintHeader("ТЕСТИРОВАНИЕ ДОПОЛНИТЕЛЬНЫХ ВОЗМОЖНОСТЕЙ");
    RUN_TEST(TestOptionAndTrySemantics);
    RUN_TEST(TestEnumerator);
    RUN_TEST(TestOperatorOverload);
    RUN_TEST(TestFrom);

    PrintSummary();
    std::cout << std::endl;
}
