CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I./include

SRC_DIR = src
BUILD_DIR = build

MAIN_TARGET = $(BUILD_DIR)/lab3
TEST_TARGET = $(BUILD_DIR)/lab3_tests

MAIN_OBJECTS = $(BUILD_DIR)/main.o $(BUILD_DIR)/tests.o
TEST_OBJECTS = $(BUILD_DIR)/tests.o $(BUILD_DIR)/tests_main.o

all: $(BUILD_DIR) $(MAIN_TARGET) $(TEST_TARGET)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(MAIN_TARGET): $(MAIN_OBJECTS)
	@echo "Линковка $(MAIN_TARGET)..."
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_TARGET): $(TEST_OBJECTS)
	@echo "Линковка $(TEST_TARGET)..."
	$(CXX) $(CXXFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@echo "Компиляция $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(MAIN_TARGET)
	@echo "Запуск программы..."
	./$(MAIN_TARGET)

test: $(TEST_TARGET)
	@echo "Запуск тестов..."
	./$(TEST_TARGET)

clean:
	@echo "Очистка..."
	rm -rf $(BUILD_DIR)

rebuild: clean all

help:
	@echo "Доступные команды:"
	@echo "  make        - Собрать основную программу и тесты"
	@echo "  make run    - Запустить интерактивный CLI"
	@echo "  make test   - Запустить модульные тесты"
	@echo "  make clean  - Удалить build/"
	@echo "  make rebuild- Пересобрать всё заново"

.PHONY: all run test clean rebuild help
