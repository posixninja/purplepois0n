# Makefile for purplepois0n
# iOS Jailbreak Tool

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++14 -Wall -Wextra -O2
DEBUG_FLAGS = -g -DDEBUG
RELEASE_FLAGS = -O3 -DNDEBUG

# Host architecture(s): native by default; override for single or universal builds.
#   make release                         — build for `uname -m`
#   make ARCH=x86_64 release             — Intel macOS slice
#   make ARCH=arm64 release              — Apple Silicon slice
#   make ARCHS="arm64 x86_64" release    — universal (requires fat/universal deps)
NATIVE_ARCH := $(shell uname -m)
ifdef ARCH
ARCHS := $(ARCH)
else
ARCHS ?= $(NATIVE_ARCH)
endif

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
ARCH_FLAGS := $(foreach arch,$(ARCHS),-arch $(arch))
CXXFLAGS += $(ARCH_FLAGS)
LDFLAGS += $(ARCH_FLAGS)
endif

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/primitives/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))

# Target executable
TARGET = $(BIN_DIR)/purplepois0n

# Libraries (Homebrew libimobiledevice / libirecovery versioned sonames)
LIBS = -limobiledevice-1.0 -lirecovery-1.0 -lplist-2.0 -lusbmuxd-2.0 -lsqlite3

# Include paths
INCLUDES = -I$(INC_DIR) -I$(SRC_DIR) \
           -I/usr/local/include \
           -I/usr/include

# Library paths
HOMEBREW_PREFIX := $(shell brew --prefix 2>/dev/null)
LIBPATHS = -L/usr/local/lib -L/usr/lib
ifneq ($(HOMEBREW_PREFIX),)
INCLUDES += -I$(HOMEBREW_PREFIX)/include
LIBPATHS += -L$(HOMEBREW_PREFIX)/lib
endif

# Default target
all: release

# Release build
release: CXXFLAGS += $(RELEASE_FLAGS)
release: $(TARGET)

# Debug build
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)

# Create directories
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# Build object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Build with in-tree exploit plugin gate enabled (mutating primitives)
plugins: CXXFLAGS += -DPURPLEPOIS0N_ENABLE_EXPLOIT_PLUGINS
plugins: release

# Link executable
$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OBJECTS) -o $(TARGET) $(LIBPATHS) $(LIBS)
	@echo "Build complete: $(TARGET) (archs: $(ARCHS))"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	@echo "Clean complete"

# Install (optional)
install: $(TARGET)
	@echo "Installing purplepois0n..."
	sudo cp $(TARGET) /usr/local/bin/
	@echo "Installation complete"

# Uninstall
uninstall:
	@echo "Uninstalling purplepois0n..."
	sudo rm -f /usr/local/bin/purplepois0n
	@echo "Uninstallation complete"

# Help
help:
	@echo "Available targets:"
	@echo "  all      - Build release version (default)"
	@echo "  release  - Build release version"
	@echo "  debug    - Build debug version"
	@echo "  clean    - Remove build artifacts"
	@echo "  install  - Install to /usr/local/bin"
	@echo "  uninstall- Remove from /usr/local/bin"
	@echo "  plugins  - Build with PURPLEPOIS0N_ENABLE_EXPLOIT_PLUGINS"
	@echo "  submodules   - git submodule update --init external/ipsw"
	@echo "  external-ipsw  - build ipsw CLI in external/ipsw (requires Go)"
	@echo "  external-ipswd - build ipswd daemon in external/ipsw (requires Go)"
	@echo "  test-fixtures - offline backup/Mach-O smoke (tests/run_fixtures.sh)"
	@echo "  help     - Show this help message"
	@echo ""
	@echo "Architecture variables (macOS):"
	@echo "  ARCH=arm64|x86_64     - single host slice"
	@echo "  ARCHS=\"arm64 x86_64\" - universal binary (needs universal deps)"

# Git submodule: blacktop/ipsw (Mach-O / dyld / IPSW CLI)
submodules:
	@if [ ! -d .git ]; then \
		echo "Initialize git first: git init && git submodule add https://github.com/blacktop/ipsw.git external/ipsw"; \
		exit 1; \
	fi
	git submodule update --init --recursive external/ipsw

external-ipsw: submodules
	$(MAKE) -C external/ipsw build
	@test -x external/ipsw/ipsw && echo "Built: $$(pwd)/external/ipsw/ipsw" || (echo "ipsw build failed" && exit 1)

external-ipswd: submodules
	cd external/ipsw && CGO_ENABLED=1 go build -o ipswd ./cmd/ipswd
	@test -x external/ipsw/ipswd && echo "Built: $$(pwd)/external/ipsw/ipswd" || (echo "ipswd build failed" && exit 1)

test-fixtures: $(TARGET)
	@chmod +x tests/run_fixtures.sh 2>/dev/null || true
	@tests/run_fixtures.sh

.PHONY: all release debug clean install uninstall help plugins submodules external-ipsw external-ipswd test-fixtures
