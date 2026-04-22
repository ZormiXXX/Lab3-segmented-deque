#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include "../include/Immutable/ImmutableSegmentedDeque.hpp"
#include "../include/InversionAlgorithms.hpp"
#include "../include/Mutable/MutableSegmentedDeque.hpp"

extern int RunAllTests();

namespace {

void ShowMenu() {
    std::cout << "\n╔══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║        ЛАБОРАТОРНАЯ РАБОТА №3 - SEGMENTED DEQUE             ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  СОЗДАНИЕ И ЗАГРУЗКА                                        ║" << std::endl;
    std::cout << "║   1. Создать дек вручную                                    ║" << std::endl;
    std::cout << "║   2. Сгенерировать случайный дек                            ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  БАЗОВЫЕ ОПЕРАЦИИ                                            ║" << std::endl;
    std::cout << "║   3. PushFront                                              ║" << std::endl;
    std::cout << "║   4. PushBack                                               ║" << std::endl;
    std::cout << "║   5. PopFront                                               ║" << std::endl;
    std::cout << "║   6. PopBack                                                ║" << std::endl;
    std::cout << "║   7. Показать дек                                           ║" << std::endl;
    std::cout << "║   8. Показать segmented-buffer layout                       ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  ОПЕРАЦИИ НАД КОЛЛЕКЦИЕЙ                                     ║" << std::endl;
    std::cout << "║   9. Concat с другим деком                                  ║" << std::endl;
    std::cout << "║   10. Извлечь поддек                                         ║" << std::endl;
    std::cout << "║   11. Найти подпоследовательность                            ║" << std::endl;
    std::cout << "║   12. Map (x * 2)                                            ║" << std::endl;
    std::cout << "║   13. Where (только чётные)                                  ║" << std::endl;
    std::cout << "║   14. Reduce (сумма)                                         ║" << std::endl;
    std::cout << "║   15. Sort                                                   ║" << std::endl;
    std::cout << "║   16. MergeSorted с другим отсортированным деком             ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  ПРОВЕРКА                                                    ║" << std::endl;
    std::cout << "║   17. Запустить модульные тесты                              ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║   0. Выход                                                   ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "> ";
}

void RequireDeque(SegmentedDeque<int>* deque) {
    if (deque == nullptr) {
        throw std::runtime_error("Сначала создайте дек");
    }
}

bool IsImmutable(const SegmentedDeque<int>* deque) {
    return dynamic_cast<const ImmutableSegmentedDeque<int>*>(deque) != nullptr;
}

void UpdateCurrentDeque(SegmentedDeque<int>*& current, SegmentedDeque<int>* updated) {
    if (updated != current) {
        delete current;
        current = updated;
    }
}

void PrintDeque(const std::string& label, const SegmentedDeque<int>& deque) {
    std::cout << "\n" << label << ": [";
    for (int i = 0; i < deque.GetLength(); i++) {
        std::cout << deque.Get(i);
        if (i + 1 < deque.GetLength()) {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;
}

void PrintDequeSummary(const SegmentedDeque<int>& deque) {
    std::cout << "Тип: " << (IsImmutable(&deque) ? "ImmutableSegmentedDeque" : "MutableSegmentedDeque") << std::endl;
    std::cout << "Хранилище блоков: " << DequeStorageKindToString(deque.GetStorageKind()) << std::endl;
    std::cout << "Длина: " << deque.GetLength() << std::endl;
    std::cout << "Layout: " << deque.DescribeLayout() << std::endl;
}

DequeStorageKind ReadStorageKind() {
    int choice = 1;
    std::cout << "Выберите хранилище блоков (1 - ArraySequence, 2 - ListSequence): ";
    std::cin >> choice;
    return choice == 2 ? DequeStorageKind::ListSequence : DequeStorageKind::ArraySequence;
}

bool ReadImmutableFlag() {
    int choice = 1;
    std::cout << "Выберите тип дека (1 - mutable, 2 - immutable): ";
    std::cin >> choice;
    return choice == 2;
}

std::vector<int> ReadValues() {
    int count = 0;
    std::cout << "Количество элементов: ";
    std::cin >> count;
    if (count < 0) {
        throw std::invalid_argument("Количество элементов должно быть неотрицательным");
    }

    std::vector<int> values(count);
    for (int i = 0; i < count; i++) {
        std::cout << "Элемент [" << i << "]: ";
        std::cin >> values[i];
    }
    return values;
}

SegmentedDeque<int>* BuildDeque(
    bool immutable,
    int blockCapacity,
    DequeStorageKind storageKind,
    const std::vector<int>& values
) {
    if (immutable) {
        return new ImmutableSegmentedDeque<int>(
            values.data(),
            static_cast<int>(values.size()),
            blockCapacity,
            storageKind
        );
    }

    return new MutableSegmentedDeque<int>(
        values.data(),
        static_cast<int>(values.size()),
        blockCapacity,
        storageKind
    );
}

}  // namespace

int main() {
    setlocale(LC_ALL, "Russian");

    SegmentedDeque<int>* current = nullptr;

    std::cout << "\nЛабораторная работа №3 - дек с сегментированным буфером" << std::endl;
    std::cout << "==========================================================" << std::endl;

    while (true) {
        ShowMenu();
        int choice = 0;
        std::cin >> choice;

        try {
            switch (choice) {
                case 1: {
                    bool immutable = ReadImmutableFlag();
                    DequeStorageKind storageKind = ReadStorageKind();
                    int blockCapacity = 0;
                    std::cout << "Размер одного сегмента: ";
                    std::cin >> blockCapacity;
                    std::vector<int> values = ReadValues();

                    delete current;
                    current = BuildDeque(immutable, blockCapacity, storageKind, values);
                    PrintDeque("Созданный дек", *current);
                    PrintDequeSummary(*current);
                    break;
                }
                case 2: {
                    bool immutable = ReadImmutableFlag();
                    DequeStorageKind storageKind = ReadStorageKind();
                    int blockCapacity = 0;
                    int count = 0;
                    int minValue = 0;
                    int maxValue = 0;
                    unsigned int seed = 0;

                    std::cout << "Размер одного сегмента: ";
                    std::cin >> blockCapacity;
                    std::cout << "Количество элементов: ";
                    std::cin >> count;
                    std::cout << "Минимальное значение: ";
                    std::cin >> minValue;
                    std::cout << "Максимальное значение: ";
                    std::cin >> maxValue;
                    std::cout << "Seed генератора: ";
                    std::cin >> seed;

                    std::vector<int> values;
                    values.reserve(count);
                    auto* generated = GenerateRandomIntDeque(count, blockCapacity, minValue, maxValue, seed, storageKind);
                    for (int i = 0; i < generated->GetLength(); i++) {
                        values.push_back(generated->Get(i));
                    }
                    delete generated;

                    delete current;
                    current = BuildDeque(immutable, blockCapacity, storageKind, values);
                    PrintDeque("Случайно сгенерированный дек", *current);
                    PrintDequeSummary(*current);
                    break;
                }
                case 3: {
                    RequireDeque(current);
                    int value = 0;
                    std::cout << "Введите значение: ";
                    std::cin >> value;
                    UpdateCurrentDeque(current, current->PushFront(value));
                    PrintDeque("После PushFront", *current);
                    break;
                }
                case 4: {
                    RequireDeque(current);
                    int value = 0;
                    std::cout << "Введите значение: ";
                    std::cin >> value;
                    UpdateCurrentDeque(current, current->PushBack(value));
                    PrintDeque("После PushBack", *current);
                    break;
                }
                case 5: {
                    RequireDeque(current);
                    int removed = current->Front();
                    UpdateCurrentDeque(current, current->PopFront());
                    std::cout << "Удалён элемент с начала: " << removed << std::endl;
                    PrintDeque("После PopFront", *current);
                    break;
                }
                case 6: {
                    RequireDeque(current);
                    int removed = current->Back();
                    UpdateCurrentDeque(current, current->PopBack());
                    std::cout << "Удалён элемент с конца: " << removed << std::endl;
                    PrintDeque("После PopBack", *current);
                    break;
                }
                case 7: {
                    RequireDeque(current);
                    PrintDeque("Текущий дек", *current);
                    PrintDequeSummary(*current);
                    break;
                }
                case 8: {
                    RequireDeque(current);
                    std::cout << "\n" << current->DescribeLayout() << std::endl;
                    break;
                }
                case 9: {
                    RequireDeque(current);
                    std::cout << "Введите элементы второго дека" << std::endl;
                    std::vector<int> values = ReadValues();
                    MutableSegmentedDeque<int> other(
                        values.data(),
                        static_cast<int>(values.size()),
                        current->GetBlockCapacity(),
                        current->GetStorageKind()
                    );

                    UpdateCurrentDeque(current, current->Concat(other));
                    PrintDeque("После Concat", *current);
                    break;
                }
                case 10: {
                    RequireDeque(current);
                    int start = 0;
                    int end = 0;
                    std::cout << "Начальный индекс: ";
                    std::cin >> start;
                    std::cout << "Конечный индекс: ";
                    std::cin >> end;

                    UpdateCurrentDeque(current, current->GetSubDeque(start, end));
                    PrintDeque("Извлечённый поддек", *current);
                    break;
                }
                case 11: {
                    RequireDeque(current);
                    std::cout << "Введите элементы шаблона для поиска" << std::endl;
                    std::vector<int> values = ReadValues();
                    MutableSegmentedDeque<int> pattern(
                        values.data(),
                        static_cast<int>(values.size()),
                        current->GetBlockCapacity(),
                        current->GetStorageKind()
                    );

                    int index = current->FindSubDeque(pattern);
                    if (index >= 0) {
                        std::cout << "Подпоследовательность найдена с индекса " << index << std::endl;
                    } else {
                        std::cout << "Подпоследовательность не найдена" << std::endl;
                    }
                    break;
                }
                case 12: {
                    RequireDeque(current);
                    UpdateCurrentDeque(current, current->Map([](const int& value) { return value * 2; }));
                    PrintDeque("После Map(x * 2)", *current);
                    break;
                }
                case 13: {
                    RequireDeque(current);
                    UpdateCurrentDeque(current, current->Where([](const int& value) { return value % 2 == 0; }));
                    PrintDeque("После Where(even)", *current);
                    break;
                }
                case 14: {
                    RequireDeque(current);
                    int sum = current->Reduce([](const int& value, const int& acc) { return value + acc; }, 0);
                    std::cout << "Сумма элементов = " << sum << std::endl;
                    break;
                }
                case 15: {
                    RequireDeque(current);
                    UpdateCurrentDeque(current, current->Sorted());
                    PrintDeque("После Sort", *current);
                    break;
                }
                case 16: {
                    RequireDeque(current);
                    std::cout << "Введите второй дек, элементы должны быть отсортированы" << std::endl;
                    std::vector<int> values = ReadValues();
                    MutableSegmentedDeque<int> other(
                        values.data(),
                        static_cast<int>(values.size()),
                        current->GetBlockCapacity(),
                        current->GetStorageKind()
                    );

                    UpdateCurrentDeque(current, current->MergeSorted(other));
                    PrintDeque("После MergeSorted", *current);
                    break;
                }
                case 17: {
                    RunAllTests();
                    break;
                }
                case 0: {
                    delete current;
                    std::cout << "\nВыход из программы..." << std::endl;
                    return 0;
                }
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
