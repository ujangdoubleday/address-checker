CC := gcc
CXX := g++
CFLAGS := -Wall -Wextra -O2
CXXFLAGS := -std=c++17 -Wall -Wextra -O2

BUILD := build
OBJ := $(BUILD)/obj

# C sources (hex, sha3)
C_SRC := hex/hex.c sha3/keccak.c sha3/sha3.c
C_OBJ := $(patsubst %.c,$(OBJ)/%.o,$(C_SRC))

# C++ sources (address, chain, main)
CXX_SRC := address/address.cpp chain/chain.cpp
CXX_OBJ := $(patsubst %.cpp,$(OBJ)/%.o,$(CXX_SRC))

TARGET := checker

.PHONY: all clean run

all: $(TARGET)
	@echo "✅ Build complete: $(TARGET)"

$(TARGET): $(OBJ)/main.o $(C_OBJ) $(CXX_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJ)/main.o: main.cpp
	@mkdir -p $(OBJ)
	$(CXX) $(CXXFLAGS) -I. -c $< -o $@

$(OBJ)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I. -c $< -o $@

$(OBJ)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I. -c $< -o $@

clean:
	@rm -rf $(BUILD) checker
	@echo "✅ Clean"

run: $(TARGET)
	./$(TARGET) --help
