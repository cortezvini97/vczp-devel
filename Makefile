# Definindo o compilador e as flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I./includes -I/usr/include/Fileinfolib -I/usr/include/cjson -I/usr/include/vczp
LDFLAGS = -lfileinfo -lz -lcjson -lvczp -L/usr/lib/vczp -L/usr/lib/Fileinfolib -L/usr/lib/b64 -lB64

# Nome do executável
TARGET = dist/vczp-devel

# Diretórios
BUILD_DIR = build
SRC_DIR = src
INCLUDE_DIR = includes

# Lista dos arquivos fonte
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Lista dos arquivos objeto
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Regra padrão
all: $(BUILD_DIR) $(TARGET)

# Regra para criar o diretório build
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Regra para compilar o executável
$(TARGET): $(OBJS)
	mkdir -p dist
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Regra para compilar os arquivos .c em .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Regra para limpar os arquivos gerados
clean:
	rm -rf $(BUILD_DIR) dist

install: all
	cp $(TARGET) /usr/bin/vczp-devel

uninstall:
	rm -f /usr/bin/vczp-devel

.PHONY: all clean install uninstall
