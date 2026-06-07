#include <algorithm>
#include <clocale>
#include <iostream>
#include <ncurses.h>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

#include "../include/Immutable/ImmutableArraySequence.hpp"
#include "../include/Immutable/ImmutableListSequence.hpp"
#include "../include/Mutable/MutableArraySequence.hpp"
#include "../include/Mutable/MutableListSequence.hpp"

extern void RunAllTests();

namespace {

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
                    const std::size_t count = std::min<std::size_t>(static_cast<std::size_t>(width), word.size() - start);
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

template<class T>
std::string FormatSequencePreview(const Sequence<T>* seq, std::size_t limit) {
    std::string result = "[";
    IEnumerator<T>* enumerator = seq->GetEnumerator();
    bool first = true;
    bool truncated = false;

    while (enumerator->MoveNext()) {
        std::ostringstream itemBuilder;
        itemBuilder << enumerator->GetCurrent();
        const std::string prefix = first ? "" : ", ";
        const std::string fragment = prefix + itemBuilder.str();

        if (result.size() + fragment.size() + 1 > limit) {
            truncated = true;
            break;
        }

        result += fragment;
        first = false;
    }
    delete enumerator;

    if (truncated) {
        result += first ? "..." : ", ...";
    }
    result += "]";
    return Truncate(result, limit);
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
            << ", данные = " << FormatSequencePreview(seq, 48);
    return builder.str();
}

template<class T>
std::string MakeSequenceMessage(const std::string& label, const Sequence<T>* seq) {
    return label + ": " + FormatSequence(seq);
}

std::string MakeTupleSequenceMessage(const std::string& label, const Sequence<Tuple<int, char>>* seq) {
    return label + ": " + FormatTupleSequence(seq);
}

std::string MakeSplitMessage(const Sequence<Sequence<int>*>* split) {
    std::ostringstream builder;
    builder << "Split по условию (x == 3):";
    for (int i = 0; i < split->GetLength(); i++) {
        builder << "\nФрагмент " << i << ": " << FormatSequence(split->Get(i));
    }
    return builder.str();
}

std::vector<MenuItem> BuildMenuItems() {
    return {
        {-1, "СОЗДАНИЕ", false},
        {1, "1. Создать MutableArraySequence", true},
        {2, "2. Создать ImmutableArraySequence", true},
        {3, "3. Создать MutableListSequence", true},
        {4, "4. Создать ImmutableListSequence", true},
        {-1, "ОСНОВНЫЕ ОПЕРАЦИИ", false},
        {5, "5. Append", true},
        {6, "6. Prepend", true},
        {7, "7. InsertAt", true},
        {8, "8. Concat", true},
        {9, "9. GetSubsequence", true},
        {10, "10. Slice", true},
        {-1, "MAP-REDUCE И ДОПОЛНЕНИЯ", false},
        {11, "11. Map", true},
        {12, "12. FlatMap", true},
        {13, "13. Reduce", true},
        {14, "14. Where", true},
        {15, "15. Find / TryFind", true},
        {16, "16. Zip", true},
        {17, "17. Unzip", true},
        {18, "18. Split", true},
        {19, "19. IEnumerable / IEnumerator", true},
        {-1, "ИНФОРМАЦИЯ И ПРОВЕРКА", false},
        {20, "20. Показать последовательность", true},
        {21, "21. Показать длину", true},
        {22, "22. Запустить модульные тесты", true},
        {-1, "ЗАВЕРШЕНИЕ", false},
        {0, "0. Выход", true},
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
                    const Sequence<int>* seq,
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
        DrawStateWindow(seq);
        DrawStatusWindow(status);
        DrawFooter(items, selectedIndex);
        refresh();
        wrefresh(menuWin_);
        wrefresh(stateWin_);
        wrefresh(statusWin_);
        wrefresh(footerWin_);
    }

private:
    static constexpr int kMinRows = 26;
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
        const int footerHeight = 3;
        const int bodyHeight = rows_ - headerHeight - footerHeight;
        const int menuWidth = std::max(38, cols_ * 42 / 100);
        const int rightWidth = cols_ - menuWidth;
        const int stateHeight = std::max(7, bodyHeight / 3);
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
        mvprintw(0, 2, "Лабораторная работа №2");
        mvprintw(1, 2, "Полноэкранный интерфейс в терминале");
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

    void DrawStateWindow(const Sequence<int>* seq) {
        werase(stateWin_);
        DrawBoxTitle(stateWin_, "Состояние");

        const int innerWidth = getmaxx(stateWin_) - 4;
        const std::vector<std::string> lines = WrapText(DescribeSequence(seq), innerWidth);
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

        const std::string help = "Стрелки: выбор  Enter: выполнить  q: выход";
        const std::string current = "Текущий пункт: " + items[static_cast<std::size_t>(selectedIndex)].label;

        PrintWindowLine(footerWin_, 1, 2, cols_ - 4, help);
        if (cols_ > 40) {
            PrintWindowLine(footerWin_, 1, std::max(2, cols_ / 2), cols_ / 2 - 3, current);
        }
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

public:
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

void ShowNumericMenu(const Sequence<int>* seq) {
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
    std::cout << "║   5. Append                                             ║" << std::endl;
    std::cout << "║   6. Prepend                                            ║" << std::endl;
    std::cout << "║   7. InsertAt                                           ║" << std::endl;
    std::cout << "║   8. Concat                                             ║" << std::endl;
    std::cout << "║   9. GetSubsequence                                     ║" << std::endl;
    std::cout << "║   10. Slice                                             ║" << std::endl;
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

int ReadInt(const std::string& prompt, TerminalUi* ui = nullptr) {
    if (ui != nullptr && ui->IsInteractive()) {
        return ui->PromptInt(prompt);
    }

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

std::vector<int> ReadSequenceValues(const std::string& label, TerminalUi* ui = nullptr) {
    int length = ReadInt("Введите количество элементов для " + label + ": ", ui);
    if (length < 0) {
        throw InvalidArgument("количество элементов не может быть отрицательным");
    }

    std::vector<int> values(static_cast<std::size_t>(length));
    for (int i = 0; i < length; i++) {
        values[static_cast<std::size_t>(i)] = ReadInt("Элемент [" + std::to_string(i) + "]: ", ui);
    }
    return values;
}

template<class ConcreteSequence>
Sequence<int>* CreateSequenceInteractively(const std::string& label, TerminalUi* ui = nullptr) {
    std::vector<int> values = ReadSequenceValues(label, ui);
    return new ConcreteSequence(values.data(), static_cast<int>(values.size()));
}

Sequence<int>* CreateHelperSequence(const std::string& label, TerminalUi* ui = nullptr) {
    std::vector<int> values = ReadSequenceValues(label, ui);
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

template<class ConcreteSequence>
bool CreateSequenceChoice(Sequence<int>*& seq,
                          std::string& status,
                          TerminalUi* ui,
                          const std::string& typeName) {
    ReplaceCurrentSequence(seq, CreateSequenceInteractively<ConcreteSequence>(typeName, ui));
    status = "Создана " + typeName + ": " + FormatSequence(seq);
    return true;
}

std::string BuildEnumeratorMessage(const Sequence<int>* seq) {
    std::ostringstream builder;
    builder << "Обход через IEnumerator: [";

    IEnumerator<int>* enumerator = seq->GetEnumerator();
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

bool HandleCreationChoice(int choice, Sequence<int>*& seq, std::string& status, TerminalUi* ui) {
    switch (choice) {
        case 1:
            return CreateSequenceChoice<MutableArraySequence<int>>(seq, status, ui, "MutableArraySequence");
        case 2:
            return CreateSequenceChoice<ImmutableArraySequence<int>>(seq, status, ui, "ImmutableArraySequence");
        case 3:
            return CreateSequenceChoice<MutableListSequence<int>>(seq, status, ui, "MutableListSequence");
        case 4:
            return CreateSequenceChoice<ImmutableListSequence<int>>(seq, status, ui, "ImmutableListSequence");
        default:
            return false;
    }
}

bool HandleBasicOperationChoice(int choice, Sequence<int>*& seq, std::string& status, TerminalUi* ui) {
    switch (choice) {
        case 5: {
            RequireSequence(seq);
            int value = ReadInt("Введите значение: ", ui);
            UpdateCurrentSequence(seq, seq->Append(value));
            status = MakeSequenceMessage("После Append", seq);
            return true;
        }
        case 6: {
            RequireSequence(seq);
            int value = ReadInt("Введите значение: ", ui);
            UpdateCurrentSequence(seq, seq->Prepend(value));
            status = MakeSequenceMessage("После Prepend", seq);
            return true;
        }
        case 7: {
            RequireSequence(seq);
            int value = ReadInt("Введите значение: ", ui);
            int index = ReadInt("Введите индекс: ", ui);
            UpdateCurrentSequence(seq, seq->InsertAt(value, index));
            status = MakeSequenceMessage("После InsertAt", seq);
            return true;
        }
        case 8: {
            RequireSequence(seq);
            Sequence<int>* extra = CreateHelperSequence("второй последовательности", ui);
            status = MakeSequenceMessage("Вторая последовательность", extra);
            UpdateCurrentSequence(seq, seq->Concat(extra));
            status += "\n" + MakeSequenceMessage("После Concat", seq);
            delete extra;
            return true;
        }
        case 9: {
            RequireSequence(seq);
            int start = ReadInt("Начальный индекс: ", ui);
            int end = ReadInt("Конечный индекс: ", ui);
            Sequence<int>* sub = seq->GetSubsequence(start, end);
            status = MakeSequenceMessage("Подпоследовательность", sub);
            delete sub;
            return true;
        }
        case 10: {
            RequireSequence(seq);
            int index = ReadInt("Индекс начала: ", ui);
            int count = ReadInt("Количество элементов для удаления: ", ui);
            Sequence<int>* replacement = CreateHelperSequence("заменяющей последовательности", ui);
            status = MakeSequenceMessage("Заменяющая последовательность", replacement);
            UpdateCurrentSequence(seq, seq->Slice(index, count, replacement));
            status += "\n" + MakeSequenceMessage("После Slice", seq);
            delete replacement;
            return true;
        }
        default:
            return false;
    }
}

bool HandleTransformChoice(int choice, Sequence<int>*& seq, std::string& status, TerminalUi* ui) {
    switch (choice) {
        case 11: {
            RequireSequence(seq);
            Sequence<int>* mapped = seq->Map([](int value) { return value * 2; });
            status = MakeSequenceMessage("Map (x * 2)", mapped);
            delete mapped;
            return true;
        }
        case 12: {
            RequireSequence(seq);
            Sequence<int>* flatMapped = seq->FlatMap([](int value) {
                int arr[] = {value, value * 10};
                return Sequence<int>::From(arr, 2);
            });
            status = MakeSequenceMessage("FlatMap ([x, x * 10])", flatMapped);
            delete flatMapped;
            return true;
        }
        case 13: {
            RequireSequence(seq);
            int sum = seq->Reduce([](int value, int acc) { return value + acc; }, 0);
            int custom = seq->Reduce([](int value, int acc) { return 2 * value + 3 * acc; }, 4);
            status = "Reduce (сумма): " + std::to_string(sum);
            status += "\nReduce по формуле 2*x + 3*acc при c=4: " + std::to_string(custom);
            return true;
        }
        case 14: {
            RequireSequence(seq);
            Sequence<int>* filtered = seq->Where([](int value) { return value % 2 == 0; });
            status = MakeSequenceMessage("Where (чётные)", filtered);
            delete filtered;
            return true;
        }
        case 15: {
            RequireSequence(seq);
            int target = ReadInt("Введите искомое значение: ", ui);

            status.clear();
            try {
                int found = seq->Find([target](int value) { return value == target; });
                status = "Find: найдено значение " + std::to_string(found);
            } catch (const ElementNotFound&) {
                status = "Find: значение не найдено";
            }

            Option<int> result = seq->TryFind([target](int value) { return value == target; });
            if (result.IsSome()) {
                status += "\nTryFind: найдено значение " + std::to_string(result.GetValue());
            } else {
                status += "\nTryFind: возвращено None";
            }
            return true;
        }
        default:
            return false;
    }
}

bool HandleCompositeOperationChoice(int choice, Sequence<int>*& seq, std::string& status) {
    switch (choice) {
        case 16: {
            RequireSequence(seq);
            char letters[] = {'A', 'B', 'C', 'D', 'E'};
            Sequence<char>* second = Sequence<char>::From(letters, 5);
            Sequence<Tuple<int, char>>* zipped = seq->Zip(second);
            status = MakeTupleSequenceMessage("Zip результат", zipped);
            delete zipped;
            delete second;
            return true;
        }
        case 17: {
            RequireSequence(seq);
            char letters[] = {'A', 'B', 'C', 'D', 'E'};
            Sequence<char>* second = Sequence<char>::From(letters, 5);
            Sequence<Tuple<int, char>>* zipped = seq->Zip(second);
            auto [first, secondPart] = zipped->Unzip();

            status = MakeTupleSequenceMessage("Исходная zip-последовательность", zipped);
            status += "\n" + MakeSequenceMessage("Unzip: первая последовательность", first);
            status += "\n" + MakeSequenceMessage("Unzip: вторая последовательность", secondPart);

            delete first;
            delete secondPart;
            delete zipped;
            delete second;
            return true;
        }
        case 18: {
            RequireSequence(seq);
            Sequence<Sequence<int>*>* split = seq->Split([](int value) { return value == 3; });
            status = MakeSplitMessage(split);
            for (int i = 0; i < split->GetLength(); i++) {
                delete split->Get(i);
            }
            delete split;
            return true;
        }
        case 19: {
            RequireSequence(seq);
            status = BuildEnumeratorMessage(seq);
            return true;
        }
        default:
            return false;
    }
}

bool HandleInfoChoice(int choice, Sequence<int>*& seq, std::string& status, TerminalUi* ui) {
    switch (choice) {
        case 20:
            RequireSequence(seq);
            status = MakeSequenceMessage("Текущая последовательность", seq);
            return true;
        case 21:
            RequireSequence(seq);
            status = "Длина последовательности: " + std::to_string(seq->GetLength());
            return true;
        case 22:
            RunTestsFromMenu(ui);
            status = "Модульные тесты выполнены.";
            return true;
        default:
            return false;
    }
}

bool ExecuteChoice(int choice, Sequence<int>*& seq, std::string& status, TerminalUi* ui) {
    if (HandleCreationChoice(choice, seq, status, ui)) {
        return true;
    }
    if (HandleBasicOperationChoice(choice, seq, status, ui)) {
        return true;
    }
    if (HandleTransformChoice(choice, seq, status, ui)) {
        return true;
    }
    if (HandleCompositeOperationChoice(choice, seq, status)) {
        return true;
    }
    if (HandleInfoChoice(choice, seq, status, ui)) {
        return true;
    }

    switch (choice) {
        case 0:
            return false;
        default:
            status = "Неверный выбор. Попробуйте снова.";
            return true;
    }
}

int RunInteractiveInterface() {
    TerminalUi ui;
    if (!ui.IsInteractive()) {
        return -1;
    }

    Sequence<int>* seq = nullptr;
    std::string status = "Выберите действие стрелками и нажмите Enter.";
    const std::vector<MenuItem> items = BuildMenuItems();
    int selectedIndex = FindFirstSelectable(items);

    ui.RenderMenu(items, selectedIndex, seq, status);

    while (true) {
        try {
            const Key key = ui.ReadKey();
            if (key == Key::Up) {
                selectedIndex = MoveSelection(items, selectedIndex, -1);
                ui.RenderMenu(items, selectedIndex, seq, status);
                continue;
            }
            if (key == Key::Down) {
                selectedIndex = MoveSelection(items, selectedIndex, 1);
                ui.RenderMenu(items, selectedIndex, seq, status);
                continue;
            }
            if (key == Key::Resize) {
                ui.RenderMenu(items, selectedIndex, seq, status);
                continue;
            }
            if (key == Key::Quit) {
                delete seq;
                return 0;
            }
            if (key == Key::Enter) {
                const int choice = items[static_cast<std::size_t>(selectedIndex)].id;
                if (!ExecuteChoice(choice, seq, status, &ui)) {
                    delete seq;
                    return 0;
                }
                ui.RenderMenu(items, selectedIndex, seq, status);
                continue;
            }
        } catch (const Exception& error) {
            status = std::string("Ошибка: ") + error.what();
            ui.RenderMenu(items, selectedIndex, seq, status);
        } catch (...) {
            status = "Ошибка: неизвестное исключение";
            ui.RenderMenu(items, selectedIndex, seq, status);
        }
    }
}

int RunNumericInterface() {
    Sequence<int>* seq = nullptr;
    std::string status;

    std::cout << "\nЛабораторная работа №2 - Система линейных структур данных" << std::endl;
    std::cout << "=============================================================" << std::endl;

    while (true) {
        ShowNumericMenu(seq);

        try {
            int choice = ReadInt("");
            if (!ExecuteChoice(choice, seq, status, nullptr)) {
                delete seq;
                std::cout << "\nВыход из программы..." << std::endl;
                return 0;
            }

            if (!status.empty()) {
                std::cout << status << std::endl;
            }
        } catch (const Exception& error) {
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
