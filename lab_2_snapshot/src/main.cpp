#include <clocale>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../include/Immutable/ImmutableArraySequence.hpp"
#include "../include/Immutable/ImmutableListSequence.hpp"
#include "../include/Mutable/MutableArraySequence.hpp"
#include "../include/Mutable/MutableListSequence.hpp"

namespace {

std::string Truncate(const std::string& text, std::size_t limit) {
    if (text.size() <= limit) {
        return text;
    }
    return text.substr(0, limit - 3) + "...";
}

template<class T>
std::string FormatSequence(const Sequence<T>* seq) {
    std::ostringstream builder;
    builder << "[";
    IEnumerator<T>* enumerator = seq->GetEnumerator();
    bool first = true;
    while (enumerator->MoveNext()) {
        if (!first) {
            builder << ", ";
        }
        builder << enumerator->GetCurrent();
        first = false;
    }
    delete enumerator;
    builder << "]";
    return builder.str();
}

std::string FormatTupleSequence(const Sequence<Tuple<int, char>>* seq) {
    std::ostringstream builder;
    builder << "[";
    IEnumerator<Tuple<int, char>>* enumerator = seq->GetEnumerator();
    bool first = true;
    while (enumerator->MoveNext()) {
        if (!first) {
            builder << ", ";
        }
        const Tuple<int, char>& item = enumerator->GetCurrent();
        builder << "(" << item.first << ", " << item.second << ")";
        first = false;
    }
    delete enumerator;
    builder << "]";
    return builder.str();
}

template<class T>
void PrintSequence(const std::string& label, const Sequence<T>* seq) {
    std::cout << "\n    " << label << ": " << FormatSequence(seq) << std::endl;
}

void PrintTupleSequence(const std::string& label, const Sequence<Tuple<int, char>>* seq) {
    std::cout << "\n    " << label << ": " << FormatTupleSequence(seq) << std::endl;
}

std::string GetSequenceTypeName(const Sequence<int>* seq) {
    if (seq == nullptr) {
        return "не создана";
    }
    if (dynamic_cast<const MutableArraySequence<int>*>(seq) != nullptr) {
        return "MutableArraySequence";
    }
    if (dynamic_cast<const ImmutableArraySequence<int>*>(seq) != nullptr) {
        return "ImmutableArraySequence";
    }
    if (dynamic_cast<const MutableListSequence<int>*>(seq) != nullptr) {
        return "MutableListSequence";
    }
    if (dynamic_cast<const ImmutableListSequence<int>*>(seq) != nullptr) {
        return "ImmutableListSequence";
    }
    return "Sequence<int>";
}

std::string DescribeSequence(const Sequence<int>* seq) {
    if (seq == nullptr) {
        return "последовательность ещё не создана";
    }

    std::ostringstream builder;
    builder << GetSequenceTypeName(seq)
            << ", длина = " << seq->GetLength()
            << ", данные = " << Truncate(FormatSequence(seq), 42);
    return builder.str();
}

void ShowMenu(const Sequence<int>* seq) {
    std::cout << "\n╔══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║           ЛАБОРАТОРНАЯ РАБОТА №2 - МЕНЮ                 ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  СОЗДАНИЕ:                                              ║" << std::endl;
    std::cout << "║   1. Создать MutableArraySequence                       ║" << std::endl;
    std::cout << "║   2. Создать ImmutableArraySequence                     ║" << std::endl;
    std::cout << "║   3. Создать MutableListSequence                        ║" << std::endl;
    std::cout << "║   4. Создать ImmutableListSequence                      ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  ОСНОВНЫЕ ОПЕРАЦИИ:                                     ║" << std::endl;
    std::cout << "║   5. Append (добавить в конец)                          ║" << std::endl;
    std::cout << "║   6. Prepend (добавить в начало)                        ║" << std::endl;
    std::cout << "║   7. InsertAt (вставить по индексу)                     ║" << std::endl;
    std::cout << "║   8. Concat (сцепить две последовательности)            ║" << std::endl;
    std::cout << "║   9. GetSubsequence (подпоследовательность)             ║" << std::endl;
    std::cout << "║   10. Slice (заменить фрагмент)                         ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  MAP-REDUCE И ДОПОЛНЕНИЯ:                               ║" << std::endl;
    std::cout << "║   11. Map                                               ║" << std::endl;
    std::cout << "║   12. FlatMap                                           ║" << std::endl;
    std::cout << "║   13. Reduce                                            ║" << std::endl;
    std::cout << "║   14. Where                                             ║" << std::endl;
    std::cout << "║   15. Find / TryFind                                    ║" << std::endl;
    std::cout << "║   16. Zip                                               ║" << std::endl;
    std::cout << "║   17. Unzip                                             ║" << std::endl;
    std::cout << "║   18. Split                                             ║" << std::endl;
    std::cout << "║   19. IEnumerable / IEnumerator                         ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  ИНФОРМАЦИЯ И ПРОВЕРКА:                                 ║" << std::endl;
    std::cout << "║   20. Показать последовательность                       ║" << std::endl;
    std::cout << "║   21. Показать длину                                    ║" << std::endl;
    std::cout << "║   22. Запустить модульные тесты                         ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║   0. Выход                                              ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "Текущее состояние: " << DescribeSequence(seq) << std::endl;
    std::cout << "> ";
}

int ReadInt(const std::string& prompt) {
    while (true) {
        std::cout << prompt;
        std::string line;
        if (!std::getline(std::cin, line)) {
            throw InputError("Ввод прерван");
        }

        std::istringstream input(line);
        int value = 0;
        char extra = '\0';
        if ((input >> value) && !(input >> extra)) {
            return value;
        }

        std::cout << "Некорректный ввод. Введите целое число." << std::endl;
    }
}

std::vector<int> ReadSequenceValues(const std::string& label) {
    int length = ReadInt("Введите количество элементов для " + label + ": ");
    if (length < 0) {
        throw InvalidArgument("количество элементов не может быть отрицательным");
    }

    std::vector<int> values(static_cast<std::size_t>(length));
    for (int i = 0; i < length; i++) {
        values[static_cast<std::size_t>(i)] = ReadInt("Элемент [" + std::to_string(i) + "]: ");
    }
    return values;
}

template<class ConcreteSequence>
Sequence<int>* CreateSequenceInteractively(const std::string& label) {
    std::vector<int> values = ReadSequenceValues(label);
    auto* created = new ConcreteSequence(values.data(), static_cast<int>(values.size()));
    std::cout << "Создана " << label << ": " << FormatSequence(created) << std::endl;
    return created;
}

Sequence<int>* CreateHelperSequence(const std::string& label) {
    std::vector<int> values = ReadSequenceValues(label);
    return Sequence<int>::From(values.data(), static_cast<int>(values.size()));
}

void ReplaceCurrentSequence(Sequence<int>*& current, Sequence<int>* replacement) {
    delete current;
    current = replacement;
}

void UpdateCurrentSequence(Sequence<int>*& current, Sequence<int>* updated) {
    if (updated != current) {
        delete current;
        current = updated;
    }
}

void RequireSequence(Sequence<int>* seq) {
    if (seq == nullptr) {
        throw InvalidState("сначала создайте последовательность");
    }
}

}  // namespace

extern void RunAllTests();

int main() {
    setlocale(LC_ALL, "Russian");

    Sequence<int>* seq = nullptr;

    std::cout << "\nЛабораторная работа №2 - Система линейных структур данных" << std::endl;
    std::cout << "=============================================================" << std::endl;

    while (true) {
        ShowMenu(seq);

        try {
            int choice = ReadInt("");

            switch (choice) {
                case 1:
                    ReplaceCurrentSequence(seq, CreateSequenceInteractively<MutableArraySequence<int>>("MutableArraySequence"));
                    break;
                case 2:
                    ReplaceCurrentSequence(seq, CreateSequenceInteractively<ImmutableArraySequence<int>>("ImmutableArraySequence"));
                    break;
                case 3:
                    ReplaceCurrentSequence(seq, CreateSequenceInteractively<MutableListSequence<int>>("MutableListSequence"));
                    break;
                case 4:
                    ReplaceCurrentSequence(seq, CreateSequenceInteractively<ImmutableListSequence<int>>("ImmutableListSequence"));
                    break;
                case 5: {
                    RequireSequence(seq);
                    int value = ReadInt("Введите значение: ");
                    UpdateCurrentSequence(seq, seq->Append(value));
                    PrintSequence("После Append", seq);
                    break;
                }
                case 6: {
                    RequireSequence(seq);
                    int value = ReadInt("Введите значение: ");
                    UpdateCurrentSequence(seq, seq->Prepend(value));
                    PrintSequence("После Prepend", seq);
                    break;
                }
                case 7: {
                    RequireSequence(seq);
                    int value = ReadInt("Введите значение: ");
                    int index = ReadInt("Введите индекс: ");
                    UpdateCurrentSequence(seq, seq->InsertAt(value, index));
                    PrintSequence("После InsertAt", seq);
                    break;
                }
                case 8: {
                    RequireSequence(seq);
                    Sequence<int>* extra = CreateHelperSequence("второй последовательности");
                    PrintSequence("Вторая последовательность", extra);
                    UpdateCurrentSequence(seq, seq->Concat(extra));
                    PrintSequence("После Concat", seq);
                    delete extra;
                    break;
                }
                case 9: {
                    RequireSequence(seq);
                    int start = ReadInt("Начальный индекс: ");
                    int end = ReadInt("Конечный индекс: ");
                    Sequence<int>* sub = seq->GetSubsequence(start, end);
                    PrintSequence("Подпоследовательность", sub);
                    delete sub;
                    break;
                }
                case 10: {
                    RequireSequence(seq);
                    int index = ReadInt("Индекс начала: ");
                    int count = ReadInt("Количество элементов для удаления: ");
                    Sequence<int>* replacement = CreateHelperSequence("заменяющей последовательности");
                    PrintSequence("Заменяющая последовательность", replacement);
                    UpdateCurrentSequence(seq, seq->Slice(index, count, replacement));
                    PrintSequence("После Slice", seq);
                    delete replacement;
                    break;
                }
                case 11: {
                    RequireSequence(seq);
                    Sequence<int>* mapped = seq->Map([](int value) { return value * 2; });
                    PrintSequence("Map (x * 2)", mapped);
                    delete mapped;
                    break;
                }
                case 12: {
                    RequireSequence(seq);
                    Sequence<int>* flatMapped = seq->FlatMap([](int value) {
                        int arr[] = {value, value * 10};
                        return Sequence<int>::From(arr, 2);
                    });
                    PrintSequence("FlatMap ([x, x * 10])", flatMapped);
                    delete flatMapped;
                    break;
                }
                case 13: {
                    RequireSequence(seq);
                    int sum = seq->Reduce([](int value, int acc) { return value + acc; }, 0);
                    std::cout << "Reduce (сумма): " << sum << std::endl;

                    int custom = seq->Reduce([](int value, int acc) { return 2 * value + 3 * acc; }, 4);
                    std::cout << "Reduce по формуле 2*x + 3*acc при c=4: " << custom << std::endl;
                    break;
                }
                case 14: {
                    RequireSequence(seq);
                    Sequence<int>* filtered = seq->Where([](int value) { return value % 2 == 0; });
                    PrintSequence("Where (чётные)", filtered);
                    delete filtered;
                    break;
                }
                case 15: {
                    RequireSequence(seq);
                    int target = ReadInt("Введите искомое значение: ");

                    try {
                        int found = seq->Find([target](int value) { return value == target; });
                        std::cout << "Find: найдено значение " << found << std::endl;
                    } catch (const ElementNotFound&) {
                        std::cout << "Find: значение не найдено" << std::endl;
                    }

                    Option<int> result = seq->TryFind([target](int value) { return value == target; });
                    if (result.IsSome()) {
                        std::cout << "TryFind: найдено значение " << result.GetValue() << std::endl;
                    } else {
                        std::cout << "TryFind: возвращено None" << std::endl;
                    }
                    break;
                }
                case 16: {
                    RequireSequence(seq);
                    char letters[] = {'A', 'B', 'C', 'D', 'E'};
                    Sequence<char>* second = Sequence<char>::From(letters, 5);
                    Sequence<Tuple<int, char>>* zipped = seq->Zip(second);
                    PrintTupleSequence("Zip результат", zipped);
                    delete zipped;
                    delete second;
                    break;
                }
                case 17: {
                    RequireSequence(seq);
                    char letters[] = {'A', 'B', 'C', 'D', 'E'};
                    Sequence<char>* second = Sequence<char>::From(letters, 5);
                    Sequence<Tuple<int, char>>* zipped = seq->Zip(second);
                    PrintTupleSequence("Исходная zip-последовательность", zipped);

                    auto [first, secondPart] = zipped->Unzip();
                    PrintSequence("Unzip: первая последовательность", first);
                    PrintSequence("Unzip: вторая последовательность", secondPart);

                    delete first;
                    delete secondPart;
                    delete zipped;
                    delete second;
                    break;
                }
                case 18: {
                    RequireSequence(seq);
                    Sequence<Sequence<int>*>* split = seq->Split([](int value) { return value == 3; });
                    std::cout << "\n    Split по условию (x == 3):" << std::endl;
                    for (int i = 0; i < split->GetLength(); i++) {
                        PrintSequence("Фрагмент " + std::to_string(i), split->Get(i));
                        delete split->Get(i);
                    }
                    delete split;
                    break;
                }
                case 19: {
                    RequireSequence(seq);
                    IEnumerator<int>* enumerator = seq->GetEnumerator();
                    std::cout << "\n    Обход через IEnumerator: [";
                    bool first = true;
                    while (enumerator->MoveNext()) {
                        if (!first) {
                            std::cout << ", ";
                        }
                        std::cout << enumerator->GetCurrent();
                        first = false;
                    }
                    std::cout << "]" << std::endl;
                    delete enumerator;
                    break;
                }
                case 20:
                    RequireSequence(seq);
                    PrintSequence("Текущая последовательность", seq);
                    break;
                case 21:
                    RequireSequence(seq);
                    std::cout << "Длина последовательности: " << seq->GetLength() << std::endl;
                    break;
                case 22:
                    RunAllTests();
                    break;
                case 0:
                    delete seq;
                    std::cout << "\nВыход из программы..." << std::endl;
                    return 0;
                default:
                    std::cout << "Неверный выбор. Попробуйте снова." << std::endl;
                    break;
            }
        } catch (const Exception& error) {
            std::cout << "Ошибка: " << error.what() << std::endl;
        } catch (...) {
            std::cout << "Ошибка: неизвестное исключение" << std::endl;
        }
    }
}
