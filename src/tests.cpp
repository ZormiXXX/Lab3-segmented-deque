#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include "../include/DynamicArray.hpp"
#include "../include/Immutable/ImmutableSegmentedDeque.hpp"
#include "../include/InversionAlgorithms.hpp"
#include "../include/Mutable/MutableSegmentedDeque.hpp"

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

static DynamicArray<TestResult> allResults;
static int totalAssertions = 0;
static int totalPassed = 0;
static bool currentTestPassed = false;
static int currentTestAssertions = 0;

#define TEST(name) void name()

#define ASSERT_EQ(expected, actual, msg) do { \
    totalAssertions++; \
    currentTestAssertions++; \
    auto expectedValue = (expected); \
    auto actualValue = (actual); \
    if (expectedValue == actualValue) { \
        totalPassed++; \
        std::cout << COLOR_GREEN "  OK  " COLOR_RESET << msg << std::endl; \
        std::cout << "    Ожидалось: " << expectedValue << ", Получено: " << actualValue << std::endl; \
    } else { \
        std::cout << COLOR_RED "  FAIL" COLOR_RESET << " " << msg << std::endl; \
        std::cout << "    " COLOR_YELLOW "Ожидалось: " COLOR_RESET << expectedValue << std::endl; \
        std::cout << "    " COLOR_RED "Получено: " COLOR_RESET << actualValue << std::endl; \
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
    } catch (const std::exception& e) { \
        std::cout << COLOR_RED "  FAIL" COLOR_RESET << " Непредвиденное исключение: " << e.what() << std::endl; \
        currentTestPassed = false; \
    } \
    allResults.Append({#name, currentTestPassed, currentTestAssertions, currentTestPassed ? currentTestAssertions : 0}); \
    std::cout << COLOR_BOLD "\n Результат: " COLOR_RESET; \
    if (currentTestPassed) { \
        std::cout << COLOR_GREEN "ПРОЙДЕН" COLOR_RESET << std::endl; \
    } else { \
        std::cout << COLOR_RED "ПРОВАЛЕН" COLOR_RESET << std::endl; \
    } \
} while (0)

namespace {

template<class T>
std::string DequeToString(const SegmentedDeque<T>& deque) {
    std::string result = "[";
    for (int i = 0; i < deque.GetLength(); i++) {
        std::ostringstream stream;
        stream << deque.Get(i);
        result += stream.str();
        if (i + 1 < deque.GetLength()) {
            result += ", ";
        }
    }
    result += "]";
    return result;
}

void PrintSubHeader(const std::string& text) {
    std::cout << COLOR_CYAN "\n  -- " COLOR_RESET << text << COLOR_CYAN " --" COLOR_RESET << std::endl;
}

}  

TEST(TestMutableDequePushAndAccess) {
    PrintSubHeader("PushBack и доступ по индексу");
    MutableSegmentedDeque<int> deque(3, DequeStorageKind::ArraySequence);

    for (int value = 1; value <= 7; value++) {
        deque.PushBack(value);
    }

    ASSERT_EQ(7, deque.GetLength(), "Длина после 7 вставок");
    ASSERT_EQ(1, deque.Front(), "Первый элемент");
    ASSERT_EQ(7, deque.Back(), "Последний элемент");
    ASSERT_EQ(4, deque.Get(3), "Элемент в середине");
    ASSERT_EQ(3, deque.GetSegmentCount(), "Количество сегментов");
}

TEST(TestPushFrontAndPopAcrossSegments) {
    PrintSubHeader("PushFront, PopFront и PopBack");
    MutableSegmentedDeque<int> deque(4, DequeStorageKind::ArraySequence);

    for (int value = 1; value <= 6; value++) {
        deque.PushFront(value);
    }

    ASSERT_EQ("[6, 5, 4, 3, 2, 1]", DequeToString(deque), "Порядок после PushFront");

    deque.PopFront();
    deque.PopFront();
    deque.PopBack();

    ASSERT_EQ("[4, 3, 2]", DequeToString(deque), "Порядок после удалений");
    ASSERT_EQ(3, deque.GetLength(), "Размер после удалений");
}

TEST(TestListStorageVariant) {
    PrintSubHeader("Вариант на ListSequence");
    MutableSegmentedDeque<std::string> deque(2, DequeStorageKind::ListSequence);

    deque.PushBack("b");
    deque.PushBack("c");
    deque.PushFront("a");

    ASSERT_EQ(DequeStorageKind::ListSequence, deque.GetStorageKind(), "Используется ListSequence");
    ASSERT_EQ(std::string("a"), deque.Front(), "Первый строковый элемент");
    ASSERT_EQ(std::string("c"), deque.Back(), "Последний строковый элемент");
    ASSERT_EQ(3, deque.GetLength(), "Корректная длина");
}

TEST(TestImmutableSemantics) {
    PrintSubHeader("Неизменяемая версия");
    int values[] = {1, 2, 3};
    ImmutableSegmentedDeque<int> original(values, 3, 3, DequeStorageKind::ArraySequence);

    SegmentedDeque<int>* appended = original.PushBack(4);
    ASSERT_EQ(3, original.GetLength(), "Оригинал не меняется");
    ASSERT_EQ(4, appended->GetLength(), "Новая версия увеличена");
    ASSERT_EQ("[1, 2, 3]", DequeToString(original), "Содержимое оригинала сохранено");
    ASSERT_EQ("[1, 2, 3, 4]", DequeToString(*appended), "Содержимое новой версии корректно");

    delete appended;
}

TEST(TestConcatSubDequeAndSearch) {
    PrintSubHeader("Concat, поддек и поиск подпоследовательности");
    int leftValues[] = {1, 2, 3};
    int rightValues[] = {4, 5, 6};
    MutableSegmentedDeque<int> left(leftValues, 3, 3, DequeStorageKind::ArraySequence);
    MutableSegmentedDeque<int> right(rightValues, 3, 3, DequeStorageKind::ArraySequence);

    SegmentedDeque<int>* joined = left.Concat(right);
    SegmentedDeque<int>* middle = joined->GetSubDeque(1, 3);
    int patternValues[] = {3, 4};
    MutableSegmentedDeque<int> pattern(patternValues, 2, 3, DequeStorageKind::ArraySequence);

    ASSERT_EQ("[1, 2, 3, 4, 5, 6]", DequeToString(*joined), "Concat работает корректно");
    ASSERT_EQ("[2, 3, 4]", DequeToString(*middle), "Поддек извлечён корректно");
    ASSERT_EQ(2, joined->FindSubDeque(pattern), "Подпоследовательность найдена");
    ASSERT_TRUE(joined->ContainsSubDeque(pattern), "ContainsSubDeque возвращает true");

    delete joined;
    delete middle;
}

TEST(TestMapWhereReduceSortMerge) {
    PrintSubHeader("Map, Where, Reduce, Sorted, MergeSorted");
    int values[] = {3, 1, 4, 2};
    MutableSegmentedDeque<int> deque(values, 4, 3, DequeStorageKind::ArraySequence);

    SegmentedDeque<int>* mapped = deque.Map([](const int& value) { return value * 2; });
    SegmentedDeque<int>* filtered = deque.Where([](const int& value) { return value % 2 == 0; });
    SegmentedDeque<int>* sorted = deque.Sorted();

    int otherValues[] = {0, 5};
    MutableSegmentedDeque<int> other(otherValues, 2, 3, DequeStorageKind::ArraySequence);
    SegmentedDeque<int>* merged = sorted->MergeSorted(other);

    ASSERT_EQ("[6, 2, 8, 4]", DequeToString(*mapped), "Map удваивает элементы");
    ASSERT_EQ("[4, 2]", DequeToString(*filtered), "Where фильтрует чётные");
    ASSERT_EQ(10, deque.Reduce([](const int& value, const int& acc) { return value + acc; }, 0), "Reduce суммирует");
    ASSERT_EQ("[1, 2, 3, 4]", DequeToString(*sorted), "Sorted сортирует по возрастанию");
    ASSERT_EQ("[0, 1, 2, 3, 4, 5]", DequeToString(*merged), "MergeSorted сливает две отсортированные структуры");

    delete mapped;
    delete filtered;
    delete sorted;
    delete merged;
}

TEST(TestToSequenceAndLayout) {
    PrintSubHeader("Преобразование в Sequence и описание layout");
    int values[] = {1, 2, 3, 4, 5};
    MutableSegmentedDeque<int> deque(values, 5, 3, DequeStorageKind::ArraySequence);

    Sequence<int>* sequence = deque.ToSequence();
    std::string layout = deque.DescribeLayout();

    ASSERT_EQ(5, sequence->GetLength(), "ToSequence сохраняет длину");
    ASSERT_EQ(1, sequence->Get(0), "ToSequence сохраняет первый элемент");
    ASSERT_EQ(5, sequence->Get(4), "ToSequence сохраняет последний элемент");
    ASSERT_TRUE(layout.find("segments=") != std::string::npos, "Layout содержит число сегментов");
    ASSERT_TRUE(layout.find("fills=[") != std::string::npos, "Layout содержит заполнение сегментов");

    delete sequence;
}

TEST(TestInversionAlgorithmsAgree) {
    PrintSubHeader("Три алгоритма подсчёта инверсий");
    int values[] = {2, 4, 1, 3, 5};
    MutableSegmentedDeque<int> deque(values, 5, 3, DequeStorageKind::ArraySequence);

    ASSERT_EQ(3LL, CountInversionsMapReduce(deque), "MapReduce-алгоритм");
    ASSERT_EQ(3LL, CountInversionsMultiPass(deque), "Многопроходный алгоритм");
    ASSERT_EQ(3LL, CountInversionsOnePass(deque), "Однопроходный алгоритм");
}

TEST(TestExceptions) {
    PrintSubHeader("Граничные случаи и исключения");
    MutableSegmentedDeque<int> empty(3, DequeStorageKind::ArraySequence);

    ASSERT_THROWS(empty.Front(), EmptyCollection, "Front у пустого дека");
    ASSERT_THROWS(empty.Back(), EmptyCollection, "Back у пустого дека");
    ASSERT_THROWS(empty.PopFront(), EmptyCollection, "PopFront у пустого дека");
    ASSERT_THROWS(empty.PopBack(), EmptyCollection, "PopBack у пустого дека");

    empty.PushBack(10);
    ASSERT_THROWS(empty.Get(1), IndexOutOfRange, "Get за пределами диапазона");
}

int RunAllTests() {
    allResults.Resize(0);
    totalAssertions = 0;
    totalPassed = 0;

    std::cout << COLOR_BOLD "\nЛабораторная работа №3 - модульные тесты" COLOR_RESET << std::endl;
    std::cout << "=============================================================" << std::endl;

    RUN_TEST(TestMutableDequePushAndAccess);
    RUN_TEST(TestPushFrontAndPopAcrossSegments);
    RUN_TEST(TestListStorageVariant);
    RUN_TEST(TestImmutableSemantics);
    RUN_TEST(TestConcatSubDequeAndSearch);
    RUN_TEST(TestMapWhereReduceSortMerge);
    RUN_TEST(TestToSequenceAndLayout);
    RUN_TEST(TestInversionAlgorithmsAgree);
    RUN_TEST(TestExceptions);

    int passedTests = 0;
    for (int i = 0; i < allResults.GetSize(); i++) {
        if (allResults[i].passed) {
            passedTests++;
        }
    }
    int failedTests = allResults.GetSize() - passedTests;

    std::cout << COLOR_BOLD COLOR_YELLOW "\n┌──────────────────────────── ИТОГИ ────────────────────────────┐" COLOR_RESET << std::endl;
    std::cout << COLOR_BOLD COLOR_YELLOW "│" COLOR_RESET
              << " Всего тестов: " << std::setw(2) << allResults.GetSize()
              << "   Успешно: " << std::setw(2) << passedTests
              << "   Провалено: " << std::setw(2) << failedTests
              << std::setw(14) << " " << COLOR_BOLD COLOR_YELLOW "│" COLOR_RESET << std::endl;
    std::cout << COLOR_BOLD COLOR_YELLOW "│" COLOR_RESET
              << " Проверок: " << std::setw(3) << totalAssertions
              << "   Успешно: " << std::setw(3) << totalPassed
              << "   Точность: " << std::fixed << std::setprecision(1)
              << (totalAssertions == 0 ? 0.0 : 100.0 * totalPassed / totalAssertions) << "%"
              << std::setw(14) << " " << COLOR_BOLD COLOR_YELLOW "│" COLOR_RESET << std::endl;
    std::cout << COLOR_BOLD COLOR_YELLOW "└───────────────────────────────────────────────────────────────┘" COLOR_RESET << std::endl;

    return totalAssertions == totalPassed ? 0 : 1;
}
