# ────────────────────────────────────────────────────────────────
# Makefile – strict for your code, system-include for Tracy
# ────────────────────────────────────────────────────────────────

CXX        := clang++
CXXSTD     := c++23

TRACY_ROOT   := submodules/tracy
TRACY_PUBLIC := $(TRACY_ROOT)/public

# ────────────────────────────────────────────────────────────────
# Mark all of Tracy as system headers (suppresses warnings there)
# ────────────────────────────────────────────────────────────────
TRACY_SYSINC := \
  -isystem $(TRACY_ROOT) \
  -isystem $(TRACY_PUBLIC) \
  -isystem $(TRACY_PUBLIC)/tracy \
  -isystem $(TRACY_PUBLIC)/client \
  -isystem $(TRACY_PUBLIC)/common

# ────────────────────────────────────────────────────────────────
# 1) Your code: strict warnings, -Werror, but Tracy folders are system
# ────────────────────────────────────────────────────────────────
CXXFLAGS := \
  -std=$(CXXSTD) \
  -Wall -Wextra -Wpedantic \
  -Wconversion -Wsign-conversion \
  -Wshadow \
  -Wnull-dereference \
  -Wdouble-promotion \
  -Wformat=2 \
  -Wimplicit-fallthrough \
  -Wmissing-declarations \
  -Wzero-as-null-pointer-constant \
  -Wold-style-cast \
  -Werror \
  -DTRACY_ENABLE \
  -I. \
  $(TRACY_SYSINC)

# ────────────────────────────────────────────────────────────────
# 2) TracyClient.cpp: also uses system includes for its own headers,
#    but if you still want to quiet any stragglers you can add 
#    -Wno-* here (not strictly necessary once you use -isystem).
# ────────────────────────────────────────────────────────────────
TRACYFLAGS := \
  -std=$(CXXSTD) \
  -DTRACY_ENABLE \
  $(TRACY_SYSINC) \
  -Wall -Wextra -Wpedantic \
  -Wno-old-style-cast \
  -Wno-cast-function-type \
  -Wno-implicit-int-conversion \
  -Wno-missing-field-initializers \
  -Wno-unused-parameter \
  -Wno-unused-function

TARGET      := main.exe
SRC         := main.cpp
TRACY_SRC   := $(TRACY_PUBLIC)/TracyClient.cpp
TRACY_OBJ   := tracy.o

ifeq ($(OS),Windows_NT)
  LDFLAGS  := -lws2_32 -ldbghelp
  RM       := del /Q
else
  LDFLAGS  :=
  RM       := rm -f
endif

.PHONY: all clean

all: $(TARGET)

# Compile TracyClient with its relaxed flags & system headers
$(TRACY_OBJ): $(TRACY_SRC)
	$(CXX) $(TRACYFLAGS) -c $< -o $@

# Compile & link your main.cpp under strict flags; Tracy headers
# are still system includes so you get no errors from them.
$(TARGET): $(SRC) $(TRACY_OBJ)
	$(CXX) $(CXXFLAGS) $< $(TRACY_OBJ) -o $@ $(LDFLAGS)

clean:
	$(RM) $(TARGET) $(TRACY_OBJ)
