#include <iostream>
#include <limits>
#include <random>
#include <string>
#include "../include/DynamicArray.hpp"
#include "../include/Exceptions.hpp"
#include "../include/Immutable/ImmutableSegmentedDeque.hpp"
#include "../include/Mutable/MutableSegmentedDeque.hpp"

extern int RunAllTests();

namespace {

using IntDeque = SegmentedDeque<int>;
using MutableIntDeque = MutableSegmentedDeque<int>;
using ImmutableIntDeque = ImmutableSegmentedDeque<int>;
using IntValues = DynamicArray<int>;

constexpr const char* kMenu = R"(
╔══════════════════════════════════════════════════════════════╗
║        ЛАБОРАТОРНАЯ РАБОТА №3 - SEGMENTED DEQUE             ║
╠══════════════════════════════════════════════════════════════╣
║  СОЗДАНИЕ И ЗАГРУЗКА                                        ║
║   1. Создать дек вручную                                    ║
║      Ввести тип, размер сегмента и все элементы             ║
║   2. Сгенерировать случайный дек                            ║
║      Заполнить дек случайными числами по диапазону          ║
╠══════════════════════════════════════════════════════════════╣
║  БАЗОВЫЕ ОПЕРАЦИИ                                           ║
║   3. PushFront                                              ║
║      Добавить элемент в начало дека                         ║
║   4. PushBack                                               ║
║      Добавить элемент в конец дека                          ║
║   5. PopFront                                               ║
║      Удалить первый элемент                                 ║
║   6. PopBack                                                ║
║      Удалить последний элемент                              ║
║   7. Показать дек                                           ║
║      Вывести элементы, тип и краткую сводку                 ║
║   8. Показать segmented-buffer layout                       ║
║      Показать сегменты и степень их заполнения              ║
╠══════════════════════════════════════════════════════════════╣
║  ОПЕРАЦИИ НАД КОЛЛЕКЦИЕЙ                                    ║
║   9. Concat с другим деком                                  ║
║      Приписать второй дек в конец текущего                  ║
║   10. Извлечь поддек                                        ║
║      Оставить элементы из диапазона [start, end]            ║
║   11. Найти подпоследовательность                           ║
║      Найти индекс первого вхождения шаблона                 ║
║   12. Map (x * 2)                                           ║
║      Умножить каждый элемент на 2                           ║
║   13. Where (только чётные)                                 ║
║      Оставить только чётные элементы                        ║
║   14. Reduce (сумма)                                        ║
║      Вычислить сумму всех элементов                         ║
║   15. Sort                                                  ║
║      Отсортировать дек по возрастанию                       ║
║   16. MergeSorted с другим отсортированным деком            ║
║      Слить два отсортированных дека                         ║
╠══════════════════════════════════════════════════════════════╣
║  ПРОВЕРКА                                                   ║
║   17. Запустить модульные тесты                             ║
║      Проверить корректность реализации                      ║
╠══════════════════════════════════════════════════════════════╣
║   0. Выход                                                  ║
║      Завершить работу программы                             ║
╚══════════════════════════════════════════════════════════════╝
)";

template<class T>
T ReadNumber(const std::string& prompt) {
    T value{};
    std::cout << prompt;
    std::cin >> value;
    return value;
}

void ShowMenu() {
    std::cout << kMenu << "> ";
}

void RequireDeque(IntDeque* deque) {
    if (deque == nullptr) {
        throw InvalidState("сначала создайте дек");
    }
}

bool IsImmutable(const IntDeque* deque) {
    return dynamic_cast<const ImmutableIntDeque*>(deque) != nullptr;
}

void ReplaceDeque(IntDeque*& current, IntDeque* updated) {
    if (updated != current) {
        delete current;
        current = updated;
    }
}

void PrintDeque(const std::string& label, const IntDeque& deque) {
    std::cout << "\n" << label << ": [";
    for (int i = 0; i < deque.GetLength(); i++) {
        if (i > 0) {
            std::cout << ", ";
        }
        std::cout << deque.Get(i);
    }
    std::cout << "]" << std::endl;
}

void PrintSummary(const IntDeque& deque) {
    std::cout << "Тип: " << (IsImmutable(&deque) ? "ImmutableSegmentedDeque" : "MutableSegmentedDeque") << std::endl;
    std::cout << "Хранилище блоков: " << DequeStorageKindToString(deque.GetStorageKind()) << std::endl;
    std::cout << "Длина: " << deque.GetLength() << std::endl;
    std::cout << "Layout: " << deque.DescribeLayout() << std::endl;
}

void PrintState(const std::string& label, const IntDeque& deque) {
    PrintDeque(label, deque);
    PrintSummary(deque);
}

DequeStorageKind ReadStorageKind() {
    return ReadNumber<int>("Выберите хранилище блоков (1 - ArraySequence, 2 - ListSequence): ") == 2
        ? DequeStorageKind::ListSequence
        : DequeStorageKind::ArraySequence;
}

bool ReadImmutableFlag() {
    return ReadNumber<int>("Выберите тип дека (1 - mutable, 2 - immutable): ") == 2;
}

IntValues ReadValues() {
    int count = ReadNumber<int>("Количество элементов: ");
    if (count < 0) {
        throw InvalidArgument("количество элементов должно быть неотрицательным");
    }

    IntValues values;
    values.Reserve(count);
    for (int i = 0; i < count; i++) {
        values.Append(ReadNumber<int>("Элемент [" + std::to_string(i) + "]: "));
    }
    return values;
}

IntValues GenerateRandomValues(int count, int minValue, int maxValue, unsigned int seed) {
    if (count < 0) {
        throw InvalidArgument("количество элементов должно быть неотрицательным");
    }
    if (minValue > maxValue) {
        throw InvalidArgument("минимальное значение не может быть больше максимального");
    }

    std::mt19937 generator(seed);
    std::uniform_int_distribution<int> distribution(minValue, maxValue);

    IntValues values;
    values.Reserve(count);
    for (int i = 0; i < count; i++) {
        values.Append(distribution(generator));
    }
    return values;
}

IntDeque* BuildDeque(
    bool immutable,
    int blockCapacity,
    DequeStorageKind storageKind,
    const IntValues& values
) {
    if (immutable) {
        return new ImmutableIntDeque(
            values.RawData(),
            values.GetSize(),
            blockCapacity,
            storageKind
        );
    }

    return new MutableIntDeque(
        values.RawData(),
        values.GetSize(),
        blockCapacity,
        storageKind
    );
}

MutableIntDeque BuildHelperDeque(const IntDeque& reference, const IntValues& values) {
    return MutableIntDeque(
        values.RawData(),
        values.GetSize(),
        reference.GetBlockCapacity(),
        reference.GetStorageKind()
    );
}

void ReplaceWithNewDeque(
    IntDeque*& current,
    bool immutable,
    int blockCapacity,
    DequeStorageKind storageKind,
    const IntValues& values,
    const std::string& label
) {
    delete current;
    current = BuildDeque(immutable, blockCapacity, storageKind, values);
    PrintState(label, *current);
}

void CreateManualDeque(IntDeque*& current) {
    bool immutable = ReadImmutableFlag();
    DequeStorageKind storageKind = ReadStorageKind();
    int blockCapacity = ReadNumber<int>("Размер одного сегмента: ");
    ReplaceWithNewDeque(
        current,
        immutable,
        blockCapacity,
        storageKind,
        ReadValues(),
        "Созданный дек"
    );
}

void CreateRandomDeque(IntDeque*& current) {
    bool immutable = ReadImmutableFlag();
    DequeStorageKind storageKind = ReadStorageKind();
    int blockCapacity = ReadNumber<int>("Размер одного сегмента: ");
    int count = ReadNumber<int>("Количество элементов: ");
    int minValue = ReadNumber<int>("Минимальное значение: ");
    int maxValue = ReadNumber<int>("Максимальное значение: ");
    unsigned int seed = ReadNumber<unsigned int>("Seed генератора: ");

    ReplaceWithNewDeque(
        current,
        immutable,
        blockCapacity,
        storageKind,
        GenerateRandomValues(count, minValue, maxValue, seed),
        "Случайно сгенерированный дек"
    );
}

MutableIntDeque ReadPeerDeque(const IntDeque& current, const std::string& title) {
    std::cout << title << std::endl;
    return BuildHelperDeque(current, ReadValues());
}

template<class Operation>
void ApplyAndShow(IntDeque*& current, const std::string& label, Operation operation) {
    RequireDeque(current);
    ReplaceDeque(current, operation(*current));
    PrintDeque(label, *current);
}

}  

int main() {
    setlocale(LC_ALL, "Russian");

    IntDeque* current = nullptr;

    std::cout << "\nЛабораторная работа №3 - дек с сегментированным буфером" << std::endl;
    std::cout << "==========================================================" << std::endl;

    while (true) {
        ShowMenu();
        int choice = 0;
        std::cin >> choice;

        try {
            switch (choice) {
                case 1:
                    CreateManualDeque(current);
                    break;
                case 2:
                    CreateRandomDeque(current);
                    break;
                case 3:
                    ApplyAndShow(current, "После PushFront", [&](IntDeque& deque) {
                        return deque.PushFront(ReadNumber<int>("Введите значение: "));
                    });
                    break;
                case 4:
                    ApplyAndShow(current, "После PushBack", [&](IntDeque& deque) {
                        return deque.PushBack(ReadNumber<int>("Введите значение: "));
                    });
                    break;
                case 5: {
                    RequireDeque(current);
                    int removed = current->Front();
                    ReplaceDeque(current, current->PopFront());
                    std::cout << "Удалён элемент с начала: " << removed << std::endl;
                    PrintDeque("После PopFront", *current);
                    break;
                }
                case 6: {
                    RequireDeque(current);
                    int removed = current->Back();
                    ReplaceDeque(current, current->PopBack());
                    std::cout << "Удалён элемент с конца: " << removed << std::endl;
                    PrintDeque("После PopBack", *current);
                    break;
                }
                case 7:
                    RequireDeque(current);
                    PrintState("Текущий дек", *current);
                    break;
                case 8:
                    RequireDeque(current);
                    std::cout << "\n" << current->DescribeLayout() << std::endl;
                    break;
                case 9: {
                    RequireDeque(current);
                    MutableIntDeque other = ReadPeerDeque(*current, "Введите элементы второго дека");
                    ApplyAndShow(current, "После Concat", [&](IntDeque& deque) {
                        return deque.Concat(other);
                    });
                    break;
                }
                case 10: {
                    int start = ReadNumber<int>("Начальный индекс: ");
                    int end = ReadNumber<int>("Конечный индекс: ");
                    ApplyAndShow(current, "Извлечённый поддек", [&](IntDeque& deque) {
                        return deque.GetSubDeque(start, end);
                    });
                    break;
                }
                case 11: {
                    RequireDeque(current);
                    MutableIntDeque pattern = ReadPeerDeque(*current, "Введите элементы шаблона для поиска");
                    int index = current->FindSubDeque(pattern);
                    std::cout << (index >= 0
                        ? "Подпоследовательность найдена с индекса " + std::to_string(index)
                        : "Подпоследовательность не найдена")
                              << std::endl;
                    break;
                }
                case 12:
                    ApplyAndShow(current, "После Map(x * 2)", [](IntDeque& deque) {
                        return deque.Map([](const int& value) { return value * 2; });
                    });
                    break;
                case 13:
                    ApplyAndShow(current, "После Where(even)", [](IntDeque& deque) {
                        return deque.Where([](const int& value) { return value % 2 == 0; });
                    });
                    break;
                case 14:
                    RequireDeque(current);
                    std::cout << "Сумма элементов = "
                              << current->Reduce([](const int& value, const int& acc) { return value + acc; }, 0)
                              << std::endl;
                    break;
                case 15:
                    ApplyAndShow(current, "После Sort", [](IntDeque& deque) {
                        return deque.Sorted();
                    });
                    break;
                case 16: {
                    RequireDeque(current);
                    MutableIntDeque other = ReadPeerDeque(
                        *current,
                        "Введите второй дек, элементы должны быть отсортированы"
                    );
                    ApplyAndShow(current, "После MergeSorted", [&](IntDeque& deque) {
                        return deque.MergeSorted(other);
                    });
                    break;
                }
                case 17:
                    RunAllTests();
                    break;
                case 0:
                    delete current;
                    std::cout << "\nВыход из программы..." << std::endl;
                    return 0;
                default:
                    std::cout << "Неизвестный пункт меню" << std::endl;
                    break;
            }
        } catch (const std::exception& error) {
            std::cout << "Ошибка: " << error.what() << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }
}
