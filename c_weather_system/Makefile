# 气象信息管理系统 - Makefile
# 常州大学 C 语言课程设计

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11
LDFLAGS = -lm
TARGET  = weather_system
OBJS    = main.o auth.o city.o weather.o stats.o file_io.o
DATA_DIR = data

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✅ 编译完成: ./$(TARGET)"

%.o: %.c weather_system.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
	rm -rf $(DATA_DIR)
	@echo "🧹 清理完成"

run: $(TARGET)
	./$(TARGET)

test: $(TARGET)
	@echo "=== 自动化测试 ==="
	@echo "admin\nadmin123\n2\n0\n0" | ./$(TARGET) 2>&1 | head -50
