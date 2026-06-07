#include <algorithm>
#include <clocale>
#include <iostream>
#include <ncurses.h>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

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

struct DequeBuildSettings {
    bool immutable;
    DequeStorageKind storageKind;
    int blockCapacity;
};

enum class Key {
    Up,
    Down,
    Enter,
    Quit,
    Resize,
    Other
};

struct MenuItem {
    int id;
    std::string label;
    std::string description;
    bool selectable;
};

std::string Truncate(const std::string& text, std::size_t limit) {
    if (text.size() <= limit) {
        return text;
    }
    if (limit <= 3) {
        return text.substr(0, limit);
    }
    return text.substr(0, limit - 3) + "...";
}

std::vector<std::string> SplitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream input(text);
    std::string line;
    while (std::getline(input, line)) {
        lines.push_back(line);
    }
    if (lines.empty()) {
        lines.push_back("");
    }
    return lines;
}

std::vector<std::string> WrapText(const std::string& text, int width) {
    if (width <= 1) {
        return {""};
    }

    std::vector<std::string> wrapped;
    for (const std::string& sourceLine : SplitLines(text)) {
        if (sourceLine.empty()) {
            wrapped.push_back("");
            continue;
        }

        std::istringstream words(sourceLine);
        std::string word;
        std::string current;

        while (words >> word) {
            if (static_cast<int>(word.size()) > width) {
                if (!current.empty()) {
                    wrapped.push_back(current);
                    current.clear();
                }

                std::size_t start = 0;
                while (start < word.size()) {
                    const std::size_t count =
                        std::min<std::size_t>(static_cast<std::size_t>(width), word.size() - start);
                    wrapped.push_back(word.substr(start, count));
                    start += count;
                }
                continue;
            }

            if (current.empty()) {
                current = word;
                continue;
            }

            if (static_cast<int>(current.size() + 1 + word.size()) <= width) {
                current += " " + word;
            } else {
                wrapped.push_back(current);
                current = word;
            }
        }

        if (!current.empty()) {
            wrapped.push_back(current);
        }
    }

    if (wrapped.empty()) {
        wrapped.push_back("");
    }
    return wrapped;
}

template<class T>
std::vector<std::string> CollectDequeItems(const SegmentedDeque<T>* deque) {
    std::vector<std::string> items;
    if (deque == nullptr) {
        return items;
    }

    Sequence<T>* sequence = deque->ToSequence();
    IEnumerator<T>* enumerator = sequence->GetEnumerator();
    items.reserve(static_cast<std::size_t>(deque->GetLength()));

    while (enumerator->MoveNext()) {
        std::ostringstream builder;
        builder << enumerator->GetCurrent();
        items.push_back(builder.str());
    }

    delete enumerator;
    delete sequence;
    return items;
}

template<class T>
std::string FormatDeque(const SegmentedDeque<T>* deque) {
    const std::vector<std::string> items = CollectDequeItems(deque);
    std::string result = "[";

    for (std::size_t i = 0; i < items.size(); i++) {
        if (i > 0) {
            result += ", ";
        }
        result += items[i];
    }

    result += "]";
    return result;
}

template<class T>
std::string FormatDequePreview(const SegmentedDeque<T>* deque, std::size_t limit) {
    const std::vector<std::string> items = CollectDequeItems(deque);
    std::string result = "[";
    bool truncated = false;

    for (std::size_t i = 0; i < items.size(); i++) {
        const std::string prefix = (i == 0) ? "" : ", ";
        const std::string fragment = prefix + items[i];

        if (result.size() + fragment.size() + 1 > limit) {
            truncated = true;
            break;
        }
        result += fragment;
    }

    if (truncated) {
        result += (result.size() == 1) ? "..." : ", ...";
    }
    result += "]";
    return Truncate(result, limit);
}

std::string GetDequeTypeName(const IntDeque* deque) {
    if (deque == nullptr) {
        return "не создан";
    }
    if (dynamic_cast<const ImmutableIntDeque*>(deque) != nullptr) {
        return "ImmutableSegmentedDeque";
    }
    if (dynamic_cast<const MutableIntDeque*>(deque) != nullptr) {
        return "MutableSegmentedDeque";
    }
    return "SegmentedDeque";
}

std::string DescribeDeque(const IntDeque* deque) {
    if (deque == nullptr) {
        return "дек ещё не создан";
    }

    std::ostringstream builder;
    builder << GetDequeTypeName(deque)
            << ", длина = " << deque->GetLength()
            << ", блок = " << deque->GetBlockCapacity()
            << ", хранилище = " << DequeStorageKindToString(deque->GetStorageKind())
            << ", данные = " << FormatDequePreview(deque, 48)
            << "\n" << deque->DescribeLayout();
    return builder.str();
}

std::string MakeDequeMessage(const std::string& label, const IntDeque* deque) {
    return label + ": " + FormatDeque(deque);
}

std::vector<MenuItem> BuildMenuItems() {
    return {
        {-1, "СОЗДАНИЕ И ЗАГРУЗКА", "", false},
        {1, "1. Создать дек вручную", "Ручной ввод элементов, типа дека и варианта хранения сегментов.", true},
        {2, "2. Сгенерировать случайный дек", "Создаёт дек из случайных чисел по диапазону и seed генератора.", true},
        {-1, "БАЗОВЫЕ ОПЕРАЦИИ", "", false},
        {3, "3. PushFront", "Добавляет новый элемент в начало дека.", true},
        {4, "4. PushBack", "Добавляет новый элемент в конец дека.", true},
        {5, "5. PopFront", "Удаляет первый элемент и возвращает новую версию дека.", true},
        {6, "6. PopBack", "Удаляет последний элемент и возвращает новую версию дека.", true},
        {7, "7. Показать дек", "Печатает все элементы текущего дека в одном списке.", true},
        {8, "8. Показать layout", "Показывает сегменты, их заполненность и параметры хранения.", true},
        {-1, "ОПЕРАЦИИ НАД КОЛЛЕКЦИЕЙ", "", false},
        {9, "9. Concat с другим деком", "Склеивает текущий дек со вторым деком тех же настроек.", true},
        {10, "10. Извлечь поддек", "Строит поддек по диапазону индексов.", true},
        {11, "11. Найти подпоследовательность", "Ищет позицию вхождения другого дека внутри текущего.", true},
        {12, "12. Map (x * 2)", "Преобразует каждый элемент по правилу x * 2.", true},
        {13, "13. Where (только чётные)", "Оставляет только элементы, удовлетворяющие условию.", true},
        {14, "14. Reduce (сумма)", "Сворачивает элементы дека в одно значение-сумму.", true},
        {15, "15. Sort", "Возвращает отсортированную версию текущего дека.", true},
        {16, "16. MergeSorted", "Сливает два отсортированных дека в один общий.", true},
        {-1, "ПРОВЕРКА", "", false},
        {17, "17. Запустить модульные тесты", "Запускает все автоматические тесты лабораторной работы.", true},
        {-1, "ЗАВЕРШЕНИЕ", "", false},
        {0, "0. Выход", "Завершает работу программы.", true},
    };
}

int FindFirstSelectable(const std::vector<MenuItem>& items) {
    for (std::size_t i = 0; i < items.size(); i++) {
        if (items[i].selectable) {
            return static_cast<int>(i);
        }
    }
    return 0;
}

int MoveSelection(const std::vector<MenuItem>& items, int currentIndex, int direction) {
    int nextIndex = currentIndex;
    const int size = static_cast<int>(items.size());

    do {
        nextIndex += direction;
        if (nextIndex < 0) {
            nextIndex = size - 1;
        }
        if (nextIndex >= size) {
            nextIndex = 0;
        }
    } while (!items[static_cast<std::size_t>(nextIndex)].selectable && nextIndex != currentIndex);

    return nextIndex;
}

class TerminalUi {
public:
    TerminalUi()
        : interactive_(isatty(STDIN_FILENO) != 0 && isatty(STDOUT_FILENO) != 0),
          cursesStarted_(false),
          suspended_(false),
          rows_(0),
          cols_(0),
          menuOffset_(0),
          menuWin_(nullptr),
          stateWin_(nullptr),
          statusWin_(nullptr),
          footerWin_(nullptr) {
        if (!interactive_) {
            return;
        }

        initscr();
        cursesStarted_ = true;
        cbreak();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);
        set_escdelay(25);

        if (has_colors()) {
            start_color();
            use_default_colors();
            init_pair(1, COLOR_BLACK, COLOR_CYAN);
            init_pair(2, COLOR_CYAN, -1);
            init_pair(3, COLOR_YELLOW, -1);
        }

        UpdateLayout();
    }

    ~TerminalUi() {
        DestroyWindows();
        if (cursesStarted_) {
            endwin();
        }
    }

    bool IsInteractive() const {
        return interactive_;
    }

    Key ReadKey() {
        if (!interactive_) {
            return Key::Other;
        }

        UpdateLayout();
        WINDOW* source = menuWin_ != nullptr ? menuWin_ : stdscr;
        const int code = wgetch(source);
        switch (code) {
            case KEY_UP:
                return Key::Up;
            case KEY_DOWN:
                return Key::Down;
            case 10:
            case 13:
            case KEY_ENTER:
                return Key::Enter;
            case 'q':
            case 'Q':
                return Key::Quit;
            case KEY_RESIZE:
                return Key::Resize;
            default:
                return Key::Other;
        }
    }

    std::string ReadLine(const std::string& prompt) {
        return PromptText("Ввод", prompt);
    }

    void SuspendForStdIo() {
        if (!interactive_ || !cursesStarted_ || suspended_) {
            return;
        }

        def_prog_mode();
        endwin();
        suspended_ = true;
    }

    void ResumeFromStdIo() {
        if (!interactive_ || !cursesStarted_ || !suspended_) {
            return;
        }

        reset_prog_mode();
        refresh();
        keypad(stdscr, TRUE);
        noecho();
        cbreak();
        curs_set(0);
        suspended_ = false;
        UpdateLayout();
    }

    void RenderMenu(const std::vector<MenuItem>& items,
                    int selectedIndex,
                    const IntDeque* deque,
                    const std::string& status) {
        if (!interactive_) {
            return;
        }

        UpdateLayout();

        if (IsTooSmall()) {
            RenderTooSmallMessage();
            return;
        }

        erase();
        DrawHeader();
        DrawMenuWindow(items, selectedIndex);
        DrawStateWindow(deque);
        DrawStatusWindow(status);
        DrawFooter(items, selectedIndex);
        refresh();
        wrefresh(menuWin_);
        wrefresh(stateWin_);
        wrefresh(statusWin_);
        wrefresh(footerWin_);
    }

    int PromptInt(const std::string& prompt) {
        while (true) {
            const std::string line = ReadLine(prompt);
            std::istringstream input(line);
            int value = 0;
            char extra = '\0';
            if ((input >> value) && !(input >> extra)) {
                return value;
            }

            ShowDialogMessage("Ошибка ввода", "Введите целое число.");
        }
    }

private:
    static constexpr int kMinRows = 27;
    static constexpr int kMinCols = 90;

    void DestroyWindows() {
        if (menuWin_ != nullptr) {
            delwin(menuWin_);
            menuWin_ = nullptr;
        }
        if (stateWin_ != nullptr) {
            delwin(stateWin_);
            stateWin_ = nullptr;
        }
        if (statusWin_ != nullptr) {
            delwin(statusWin_);
            statusWin_ = nullptr;
        }
        if (footerWin_ != nullptr) {
            delwin(footerWin_);
            footerWin_ = nullptr;
        }
    }

    bool IsTooSmall() const {
        return rows_ < kMinRows || cols_ < kMinCols;
    }

    void UpdateLayout() {
        if (!interactive_) {
            return;
        }

        int newRows = 0;
        int newCols = 0;
        getmaxyx(stdscr, newRows, newCols);
        if (newRows == rows_ && newCols == cols_ &&
            menuWin_ != nullptr && stateWin_ != nullptr &&
            statusWin_ != nullptr && footerWin_ != nullptr) {
            return;
        }

        rows_ = newRows;
        cols_ = newCols;

        DestroyWindows();
        if (IsTooSmall()) {
            return;
        }

        const int headerHeight = 3;
        const int footerHeight = 4;
        const int bodyHeight = rows_ - headerHeight - footerHeight;
        const int menuWidth = std::max(38, cols_ * 42 / 100);
        const int rightWidth = cols_ - menuWidth;
        const int stateHeight = std::max(8, bodyHeight / 3);
        const int statusHeight = bodyHeight - stateHeight;

        menuWin_ = newwin(bodyHeight, menuWidth, headerHeight, 0);
        stateWin_ = newwin(stateHeight, rightWidth, headerHeight, menuWidth);
        statusWin_ = newwin(statusHeight, rightWidth, headerHeight + stateHeight, menuWidth);
        footerWin_ = newwin(footerHeight, cols_, rows_ - footerHeight, 0);

        keypad(menuWin_, TRUE);
    }

    void DrawBoxTitle(WINDOW* win, const std::string& title) {
        box(win, 0, 0);
        wattron(win, A_BOLD);
        mvwprintw(win, 0, 2, " %s ", title.c_str());
        wattroff(win, A_BOLD);
    }

    void DrawHeader() {
        attron(A_BOLD);
        mvprintw(0, 2, "Лабораторная работа №3");
        mvprintw(1, 2, "Полноэкранный интерфейс: дек с сегментированным буфером");
        attroff(A_BOLD);
        mvhline(2, 0, ACS_HLINE, cols_);
    }

    void AdjustMenuOffset(const std::vector<MenuItem>& items, int selectedIndex, int visibleLines) {
        if (visibleLines <= 0) {
            menuOffset_ = 0;
            return;
        }

        if (selectedIndex < menuOffset_) {
            menuOffset_ = selectedIndex;
        } else if (selectedIndex >= menuOffset_ + visibleLines) {
            menuOffset_ = selectedIndex - visibleLines + 1;
        }

        const int maxOffset = std::max(0, static_cast<int>(items.size()) - visibleLines);
        menuOffset_ = std::max(0, std::min(menuOffset_, maxOffset));
    }

    void PrintWindowLine(WINDOW* win, int row, int col, int maxWidth, const std::string& text, int attributes = 0) {
        std::string cropped = Truncate(text, static_cast<std::size_t>(std::max(0, maxWidth)));
        if (attributes != 0) {
            wattron(win, attributes);
        }
        mvwprintw(win, row, col, "%-*s", maxWidth, cropped.c_str());
        if (attributes != 0) {
            wattroff(win, attributes);
        }
    }

    void DrawMenuWindow(const std::vector<MenuItem>& items, int selectedIndex) {
        werase(menuWin_);
        DrawBoxTitle(menuWin_, "Меню");

        const int innerHeight = getmaxy(menuWin_) - 2;
        const int innerWidth = getmaxx(menuWin_) - 4;
        AdjustMenuOffset(items, selectedIndex, innerHeight);

        for (int row = 0; row < innerHeight; row++) {
            const int itemIndex = menuOffset_ + row;
            if (itemIndex >= static_cast<int>(items.size())) {
                break;
            }

            const MenuItem& item = items[static_cast<std::size_t>(itemIndex)];
            if (!item.selectable) {
                PrintWindowLine(menuWin_, row + 1, 2, innerWidth, item.label, A_BOLD);
                continue;
            }

            int attributes = 0;
            if (itemIndex == selectedIndex) {
                attributes = A_BOLD | A_REVERSE;
                if (has_colors()) {
                    attributes |= COLOR_PAIR(1);
                }
            }
            PrintWindowLine(menuWin_, row + 1, 2, innerWidth, item.label, attributes);
        }

        if (menuOffset_ > 0) {
            mvwprintw(menuWin_, 1, getmaxx(menuWin_) - 3, "^");
        }
        if (menuOffset_ + innerHeight < static_cast<int>(items.size())) {
            mvwprintw(menuWin_, getmaxy(menuWin_) - 2, getmaxx(menuWin_) - 3, "v");
        }
    }

    void DrawStateWindow(const IntDeque* deque) {
        werase(stateWin_);
        DrawBoxTitle(stateWin_, "Состояние");

        const int innerWidth = getmaxx(stateWin_) - 4;
        const std::vector<std::string> lines = WrapText(DescribeDeque(deque), innerWidth);
        const int visible = std::min<int>(static_cast<int>(lines.size()), getmaxy(stateWin_) - 2);

        for (int i = 0; i < visible; i++) {
            PrintWindowLine(stateWin_, i + 1, 2, innerWidth, lines[static_cast<std::size_t>(i)]);
        }
    }

    void DrawStatusWindow(const std::string& status) {
        werase(statusWin_);
        DrawBoxTitle(statusWin_, "Результат");

        const int innerWidth = getmaxx(statusWin_) - 4;
        const int innerHeight = getmaxy(statusWin_) - 2;
        const std::vector<std::string> lines = WrapText(status, innerWidth);
        const int visible = std::min<int>(static_cast<int>(lines.size()), innerHeight);

        for (int i = 0; i < visible; i++) {
            PrintWindowLine(statusWin_, i + 1, 2, innerWidth, lines[static_cast<std::size_t>(i)]);
        }

        if (static_cast<int>(lines.size()) > innerHeight) {
            PrintWindowLine(statusWin_, innerHeight, 2, innerWidth, "...");
        }
    }

    void DrawFooter(const std::vector<MenuItem>& items, int selectedIndex) {
        werase(footerWin_);
        box(footerWin_, 0, 0);

        const MenuItem& currentItem = items[static_cast<std::size_t>(selectedIndex)];
        const std::string help = "Стрелки: выбор  Enter: выполнить  q: выход";
        const std::string current = "Текущий пункт: " + currentItem.label;
        const std::string description = currentItem.selectable
            ? "Что делает: " + currentItem.description
            : "Что делает: это раздел меню.";

        PrintWindowLine(footerWin_, 1, 2, cols_ - 4, help);
        if (cols_ > 50) {
            PrintWindowLine(footerWin_, 1, std::max(2, cols_ / 2), cols_ / 2 - 3, current);
        }
        PrintWindowLine(footerWin_, 2, 2, cols_ - 4, description);
    }

    void RenderTooSmallMessage() {
        erase();
        attron(A_BOLD);
        mvprintw(1, 2, "Окно терминала слишком маленькое");
        attroff(A_BOLD);
        mvprintw(3, 2, "Для полноэкранного интерфейса нужно минимум %d x %d.", kMinCols, kMinRows);
        mvprintw(5, 2, "Увеличьте окно терминала или запустите программу без TTY для обычного режима.");
        mvprintw(7, 2, "Нажмите q для выхода.");
        refresh();
    }

    std::string PromptText(const std::string& title, const std::string& prompt) {
        UpdateLayout();
        if (IsTooSmall()) {
            throw InvalidState("увеличьте окно терминала для ввода данных");
        }

        const int maxDialogWidth = std::max(40, cols_ - 10);
        const int dialogWidth = std::min(maxDialogWidth, std::max(56, static_cast<int>(prompt.size()) + 8));
        const std::vector<std::string> promptLines = WrapText(prompt, dialogWidth - 4);
        const int dialogHeight = std::max(8, static_cast<int>(promptLines.size()) + 5);
        const int startY = std::max(1, (rows_ - dialogHeight) / 2);
        const int startX = std::max(1, (cols_ - dialogWidth) / 2);

        WINDOW* dialog = newwin(dialogHeight, dialogWidth, startY, startX);
        keypad(dialog, TRUE);
        DrawBoxTitle(dialog, title);

        for (std::size_t i = 0; i < promptLines.size() && static_cast<int>(i) < dialogHeight - 4; i++) {
            PrintWindowLine(dialog, static_cast<int>(i) + 1, 2, dialogWidth - 4, promptLines[i]);
        }

        PrintWindowLine(dialog, dialogHeight - 2, 2, dialogWidth - 6, "> ");
        wmove(dialog, dialogHeight - 2, 4);
        wrefresh(dialog);

        echo();
        curs_set(1);

        char buffer[256] = {};
        wgetnstr(dialog, buffer, 255);

        noecho();
        curs_set(0);

        const std::string result(buffer);
        delwin(dialog);
        touchwin(stdscr);
        refresh();
        return result;
    }

    void ShowDialogMessage(const std::string& title, const std::string& message) {
        UpdateLayout();
        if (IsTooSmall()) {
            return;
        }

        const int dialogWidth = std::min(cols_ - 6, std::max(50, static_cast<int>(message.size()) + 8));
        const std::vector<std::string> messageLines = WrapText(message, dialogWidth - 4);
        const int dialogHeight = std::max(7, static_cast<int>(messageLines.size()) + 4);
        const int startY = std::max(1, (rows_ - dialogHeight) / 2);
        const int startX = std::max(1, (cols_ - dialogWidth) / 2);

        WINDOW* dialog = newwin(dialogHeight, dialogWidth, startY, startX);
        keypad(dialog, TRUE);
        DrawBoxTitle(dialog, title);

        for (std::size_t i = 0; i < messageLines.size() && static_cast<int>(i) < dialogHeight - 3; i++) {
            PrintWindowLine(dialog, static_cast<int>(i) + 1, 2, dialogWidth - 4, messageLines[i]);
        }
        PrintWindowLine(dialog, dialogHeight - 2, 2, dialogWidth - 4, "Нажмите любую клавишу...");

        wrefresh(dialog);
        wgetch(dialog);
        delwin(dialog);
        touchwin(stdscr);
        refresh();
    }

    bool interactive_;
    bool cursesStarted_;
    bool suspended_;
    int rows_;
    int cols_;
    int menuOffset_;
    WINDOW* menuWin_;
    WINDOW* stateWin_;
    WINDOW* statusWin_;
    WINDOW* footerWin_;
};

void ShowNumericMenu(const IntDeque* deque) {
    std::cout << "\n╔══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║          ЛАБОРАТОРНАЯ РАБОТА №3 - МЕНЮ                 ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  СОЗДАНИЕ И ЗАГРУЗКА:                                   ║" << std::endl;
    std::cout << "║   1. Создать дек вручную                                ║" << std::endl;
    std::cout << "║   2. Сгенерировать случайный дек                        ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  БАЗОВЫЕ ОПЕРАЦИИ:                                      ║" << std::endl;
    std::cout << "║   3. PushFront                                          ║" << std::endl;
    std::cout << "║   4. PushBack                                           ║" << std::endl;
    std::cout << "║   5. PopFront                                           ║" << std::endl;
    std::cout << "║   6. PopBack                                            ║" << std::endl;
    std::cout << "║   7. Показать дек                                       ║" << std::endl;
    std::cout << "║   8. Показать layout                                    ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  ОПЕРАЦИИ НАД КОЛЛЕКЦИЕЙ:                               ║" << std::endl;
    std::cout << "║   9. Concat с другим деком                              ║" << std::endl;
    std::cout << "║   10. Извлечь поддек                                    ║" << std::endl;
    std::cout << "║   11. Найти подпоследовательность                       ║" << std::endl;
    std::cout << "║   12. Map (x * 2)                                       ║" << std::endl;
    std::cout << "║   13. Where (только чётные)                             ║" << std::endl;
    std::cout << "║   14. Reduce (сумма)                                    ║" << std::endl;
    std::cout << "║   15. Sort                                              ║" << std::endl;
    std::cout << "║   16. MergeSorted                                       ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  ПРОВЕРКА:                                              ║" << std::endl;
    std::cout << "║   17. Запустить модульные тесты                         ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║   0. Выход                                              ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "Текущее состояние: " << DescribeDeque(deque) << std::endl;
    std::cout << "> ";
}

int ReadInt(const std::string& prompt, TerminalUi* ui = nullptr) {
    if (ui != nullptr && ui->IsInteractive()) {
        return ui->PromptInt(prompt);
    }

    while (true) {
        std::cout << prompt;
        std::string line;
        if (!std::getline(std::cin, line)) {
            throw InputError("ввод прерван");
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

DequeStorageKind ReadStorageKind(TerminalUi* ui = nullptr) {
    const int choice = ReadInt("Выберите хранилище блоков (1 - ArraySequence, 2 - ListSequence): ", ui);
    return choice == 2 ? DequeStorageKind::ListSequence : DequeStorageKind::ArraySequence;
}

bool ReadImmutableFlag(TerminalUi* ui = nullptr) {
    return ReadInt("Выберите тип дека (1 - mutable, 2 - immutable): ", ui) == 2;
}

DequeBuildSettings ReadDequeBuildSettings(TerminalUi* ui = nullptr) {
    DequeBuildSettings settings{};
    settings.immutable = ReadImmutableFlag(ui);
    settings.storageKind = ReadStorageKind(ui);
    settings.blockCapacity = ReadInt("Размер одного сегмента: ", ui);
    return settings;
}

IntValues ReadValues(const std::string& label, TerminalUi* ui = nullptr) {
    int count = ReadInt("Введите количество элементов для " + label + ": ", ui);
    if (count < 0) {
        throw InvalidArgument("количество элементов не может быть отрицательным");
    }

    IntValues values;
    values.Reserve(count);
    for (int i = 0; i < count; i++) {
        values.Append(ReadInt("Элемент [" + std::to_string(i) + "]: ", ui));
    }
    return values;
}

IntValues GenerateRandomValues(int count, int minValue, int maxValue, unsigned int seed) {
    if (count < 0) {
        throw InvalidArgument("количество элементов не может быть отрицательным");
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

IntDeque* BuildDeque(const DequeBuildSettings& settings, const IntValues& values) {
    return BuildDeque(
        settings.immutable,
        settings.blockCapacity,
        settings.storageKind,
        values
    );
}

MutableIntDeque CreateHelperDeque(const IntDeque& reference, const std::string& label, TerminalUi* ui = nullptr) {
    IntValues values = ReadValues(label, ui);
    return MutableIntDeque(
        values.RawData(),
        values.GetSize(),
        reference.GetBlockCapacity(),
        reference.GetStorageKind()
    );
}

void ReplaceCurrentDeque(IntDeque*& current, IntDeque* replacement) {
    delete current;
    current = replacement;
}

void UpdateCurrentDeque(IntDeque*& current, IntDeque* updated) {
    if (updated != current) {
        delete current;
        current = updated;
    }
}

void RequireDeque(IntDeque* deque) {
    if (deque == nullptr) {
        throw InvalidState("сначала создайте дек");
    }
}

std::string BuildReducedMessage(const IntDeque* deque) {
    const int sum = deque->Reduce([](const int& value, const int& acc) { return value + acc; }, 0);
    return "Reduce (сумма): " + std::to_string(sum);
}

std::string BuildSearchResultMessage(const MutableIntDeque& pattern, int index) {
    std::string status = MakeDequeMessage("Шаблон", &pattern);
    if (index >= 0) {
        status += "\nПодпоследовательность найдена с индекса " + std::to_string(index);
    } else {
        status += "\nПодпоследовательность не найдена";
    }
    return status;
}

void CreateManualDeque(IntDeque*& deque, std::string& status, TerminalUi* ui) {
    const DequeBuildSettings settings = ReadDequeBuildSettings(ui);
    ReplaceCurrentDeque(deque, BuildDeque(settings, ReadValues("дека", ui)));
    status = "Создан новый дек.\n" + MakeDequeMessage("Данные", deque);
}

void CreateRandomDeque(IntDeque*& deque, std::string& status, TerminalUi* ui) {
    const DequeBuildSettings settings = ReadDequeBuildSettings(ui);
    const int count = ReadInt("Количество элементов: ", ui);
    const int minValue = ReadInt("Минимальное значение: ", ui);
    const int maxValue = ReadInt("Максимальное значение: ", ui);
    const int seed = ReadInt("Seed генератора: ", ui);

    ReplaceCurrentDeque(
        deque,
        BuildDeque(
            settings,
            GenerateRandomValues(count, minValue, maxValue, static_cast<unsigned int>(seed))
        )
    );
    status = "Сгенерирован случайный дек.\n" + MakeDequeMessage("Данные", deque);
}

void ApplyPushFront(IntDeque*& deque, std::string& status, TerminalUi* ui) {
    RequireDeque(deque);
    UpdateCurrentDeque(deque, deque->PushFront(ReadInt("Введите значение: ", ui)));
    status = MakeDequeMessage("После PushFront", deque);
}

void ApplyPushBack(IntDeque*& deque, std::string& status, TerminalUi* ui) {
    RequireDeque(deque);
    UpdateCurrentDeque(deque, deque->PushBack(ReadInt("Введите значение: ", ui)));
    status = MakeDequeMessage("После PushBack", deque);
}

void ApplyPopFront(IntDeque*& deque, std::string& status) {
    RequireDeque(deque);
    const int removed = deque->Front();
    UpdateCurrentDeque(deque, deque->PopFront());
    status = "Удалён первый элемент: " + std::to_string(removed);
    status += "\n" + MakeDequeMessage("После PopFront", deque);
}

void ApplyPopBack(IntDeque*& deque, std::string& status) {
    RequireDeque(deque);
    const int removed = deque->Back();
    UpdateCurrentDeque(deque, deque->PopBack());
    status = "Удалён последний элемент: " + std::to_string(removed);
    status += "\n" + MakeDequeMessage("После PopBack", deque);
}

void ShowCurrentDeque(IntDeque* deque, std::string& status) {
    RequireDeque(deque);
    status = MakeDequeMessage("Текущий дек", deque);
    status += "\n" + deque->DescribeLayout();
}

void ShowCurrentLayout(IntDeque* deque, std::string& status) {
    RequireDeque(deque);
    status = deque->DescribeLayout();
}

void ApplyConcat(IntDeque*& deque, std::string& status, TerminalUi* ui) {
    RequireDeque(deque);
    MutableIntDeque other = CreateHelperDeque(*deque, "второго дека", ui);
    status = MakeDequeMessage("Второй дек", &other);
    UpdateCurrentDeque(deque, deque->Concat(other));
    status += "\n" + MakeDequeMessage("После Concat", deque);
}

void ApplySubDeque(IntDeque*& deque, std::string& status, TerminalUi* ui) {
    RequireDeque(deque);
    const int start = ReadInt("Начальный индекс: ", ui);
    const int end = ReadInt("Конечный индекс: ", ui);
    UpdateCurrentDeque(deque, deque->GetSubDeque(start, end));
    status = MakeDequeMessage("Извлечённый поддек", deque);
}

void ApplyFindSubDeque(IntDeque* deque, std::string& status, TerminalUi* ui) {
    RequireDeque(deque);
    MutableIntDeque pattern = CreateHelperDeque(*deque, "шаблона для поиска", ui);
    status = BuildSearchResultMessage(pattern, deque->FindSubDeque(pattern));
}

void ApplyMapDouble(IntDeque*& deque, std::string& status) {
    RequireDeque(deque);
    UpdateCurrentDeque(deque, deque->Map([](const int& value) { return value * 2; }));
    status = MakeDequeMessage("После Map (x * 2)", deque);
}

void ApplyWhereEven(IntDeque*& deque, std::string& status) {
    RequireDeque(deque);
    UpdateCurrentDeque(deque, deque->Where([](const int& value) { return value % 2 == 0; }));
    status = MakeDequeMessage("После Where (чётные)", deque);
}

void ApplyReduceSum(IntDeque* deque, std::string& status) {
    RequireDeque(deque);
    status = BuildReducedMessage(deque);
}

void ApplySort(IntDeque*& deque, std::string& status) {
    RequireDeque(deque);
    UpdateCurrentDeque(deque, deque->Sorted());
    status = MakeDequeMessage("После Sort", deque);
}

void ApplyMergeSorted(IntDeque*& deque, std::string& status, TerminalUi* ui) {
    RequireDeque(deque);
    MutableIntDeque other = CreateHelperDeque(*deque, "второго отсортированного дека", ui);
    status = MakeDequeMessage("Второй отсортированный дек", &other);
    UpdateCurrentDeque(deque, deque->MergeSorted(other));
    status += "\n" + MakeDequeMessage("После MergeSorted", deque);
}

void RunTestsFromMenu(TerminalUi* ui) {
    if (ui != nullptr && ui->IsInteractive()) {
        ui->SuspendForStdIo();
        RunAllTests();
        std::cout << "\nНажмите Enter, чтобы вернуться в интерфейс...";
        std::string line;
        std::getline(std::cin, line);
        ui->ResumeFromStdIo();
        return;
    }

    RunAllTests();
}

bool HandleCreationChoice(int choice, IntDeque*& deque, std::string& status, TerminalUi* ui) {
    switch (choice) {
        case 1:
            CreateManualDeque(deque, status, ui);
            return true;
        case 2:
            CreateRandomDeque(deque, status, ui);
            return true;
        default:
            return false;
    }
}

bool HandleBasicChoice(int choice, IntDeque*& deque, std::string& status, TerminalUi* ui) {
    switch (choice) {
        case 3:
            ApplyPushFront(deque, status, ui);
            return true;
        case 4:
            ApplyPushBack(deque, status, ui);
            return true;
        case 5:
            ApplyPopFront(deque, status);
            return true;
        case 6:
            ApplyPopBack(deque, status);
            return true;
        case 7:
            ShowCurrentDeque(deque, status);
            return true;
        case 8:
            ShowCurrentLayout(deque, status);
            return true;
        default:
            return false;
    }
}

bool HandleCollectionChoice(int choice, IntDeque*& deque, std::string& status, TerminalUi* ui) {
    switch (choice) {
        case 9:
            ApplyConcat(deque, status, ui);
            return true;
        case 10:
            ApplySubDeque(deque, status, ui);
            return true;
        case 11:
            ApplyFindSubDeque(deque, status, ui);
            return true;
        case 12:
            ApplyMapDouble(deque, status);
            return true;
        case 13:
            ApplyWhereEven(deque, status);
            return true;
        case 14:
            ApplyReduceSum(deque, status);
            return true;
        case 15:
            ApplySort(deque, status);
            return true;
        case 16:
            ApplyMergeSorted(deque, status, ui);
            return true;
        default:
            return false;
    }
}

bool HandleInfoChoice(int choice, std::string& status, TerminalUi* ui) {
    switch (choice) {
        case 17:
            RunTestsFromMenu(ui);
            status = "Модульные тесты выполнены.";
            return true;
        default:
            return false;
    }
}

bool ExecuteChoice(int choice, IntDeque*& deque, std::string& status, TerminalUi* ui) {
    if (choice == 0) {
        delete deque;
        deque = nullptr;
        return false;
    }
    if (HandleCreationChoice(choice, deque, status, ui)) {
        return true;
    }
    if (HandleBasicChoice(choice, deque, status, ui)) {
        return true;
    }
    if (HandleCollectionChoice(choice, deque, status, ui)) {
        return true;
    }
    if (HandleInfoChoice(choice, status, ui)) {
        return true;
    }

    status = "Неверный выбор. Попробуйте снова.";
    return true;
}

int RunInteractiveInterface() {
    TerminalUi ui;
    if (!ui.IsInteractive()) {
        return -1;
    }

    IntDeque* deque = nullptr;
    std::string status = "Выберите действие стрелками и нажмите Enter.";
    const std::vector<MenuItem> items = BuildMenuItems();
    int selectedIndex = FindFirstSelectable(items);

    ui.RenderMenu(items, selectedIndex, deque, status);

    while (true) {
        try {
            const Key key = ui.ReadKey();
            if (key == Key::Up) {
                selectedIndex = MoveSelection(items, selectedIndex, -1);
                ui.RenderMenu(items, selectedIndex, deque, status);
                continue;
            }
            if (key == Key::Down) {
                selectedIndex = MoveSelection(items, selectedIndex, 1);
                ui.RenderMenu(items, selectedIndex, deque, status);
                continue;
            }
            if (key == Key::Resize) {
                ui.RenderMenu(items, selectedIndex, deque, status);
                continue;
            }
            if (key == Key::Quit) {
                delete deque;
                return 0;
            }
            if (key == Key::Enter) {
                const int choice = items[static_cast<std::size_t>(selectedIndex)].id;
                if (!ExecuteChoice(choice, deque, status, &ui)) {
                    return 0;
                }
                ui.RenderMenu(items, selectedIndex, deque, status);
                continue;
            }
        } catch (const Exception& error) {
            status = std::string("Ошибка: ") + error.what();
            ui.RenderMenu(items, selectedIndex, deque, status);
        } catch (const std::exception& error) {
            status = std::string("Ошибка: ") + error.what();
            ui.RenderMenu(items, selectedIndex, deque, status);
        } catch (...) {
            status = "Ошибка: неизвестное исключение";
            ui.RenderMenu(items, selectedIndex, deque, status);
        }
    }
}

int RunNumericInterface() {
    IntDeque* deque = nullptr;
    std::string status;

    std::cout << "\nЛабораторная работа №3 - дек с сегментированным буфером" << std::endl;
    std::cout << "==========================================================" << std::endl;

    while (true) {
        ShowNumericMenu(deque);

        try {
            const int choice = ReadInt("");
            if (!ExecuteChoice(choice, deque, status, nullptr)) {
                std::cout << "\nВыход из программы..." << std::endl;
                return 0;
            }

            if (!status.empty()) {
                std::cout << status << std::endl;
            }
        } catch (const InputError&) {
            delete deque;
            std::cout << "\nВвод завершён. Выход из программы..." << std::endl;
            return 0;
        } catch (const Exception& error) {
            std::cout << "Ошибка: " << error.what() << std::endl;
        } catch (const std::exception& error) {
            std::cout << "Ошибка: " << error.what() << std::endl;
        } catch (...) {
            std::cout << "Ошибка: неизвестное исключение" << std::endl;
        }
    }
}

}  // namespace

int main() {
    setlocale(LC_ALL, "");

    const int interactiveResult = RunInteractiveInterface();
    if (interactiveResult != -1) {
        return interactiveResult;
    }

    return RunNumericInterface();
}
