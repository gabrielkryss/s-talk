CXX = clang++ # or g++
CXXSTD = c++23  # or c++17, c++20, etc.
CXXFLAGS = -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wnull-dereference -Wdouble-promotion -Wformat=2 -Wimplicit-fallthrough -Wmissing-declarations -Wzero-as-null-pointer-constant -Wold-style-cast -Werror
TARGET = main.exe
SRC = main.cpp

# Detect platform
ifeq ($(OS),Windows_NT)
	LDFLAGS = -lws2_32
else
	LDFLAGS =
endif

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) -std=$(CXXSTD) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	del $(TARGET)
