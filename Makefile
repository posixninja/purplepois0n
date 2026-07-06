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
SOURCES = $(wildcard $(SRC_DIR)/*.cpp) \
          $(wildcard $(SRC_DIR)/cli/*.cpp) \
          $(wildcard $(SRC_DIR)/pongo/*.cpp) \
          $(wildcard $(SRC_DIR)/primitives/*.cpp) \
          $(wildcard $(SRC_DIR)/store/*.cpp) \
          $(wildcard $(SRC_DIR)/devicetree/*.cpp) \
          $(wildcard $(SRC_DIR)/primitives/historical/*.cpp) \
          $(wildcard $(SRC_DIR)/primitives/pongo/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))

# Target executable
TARGET = $(BIN_DIR)/purplepois0n

# Libraries (Homebrew libimobiledevice / libirecovery versioned sonames)
LIBS = -limobiledevice-1.0 -lirecovery-1.0 -lplist-2.0 -lusbmuxd-2.0 -lsqlite3

# Optional libtatsu (libimobiledevice TSS — idevicerestore tss.c successor)
#   make release LIBTATSU=1     — force enable if pkg-config finds libtatsu-1.0
#   auto-links when libtatsu-1.0 is installed and LIBTATSU is unset
LIBTATSU_PKG := $(shell pkg-config --exists libtatsu-1.0 2>/dev/null && echo libtatsu-1.0)
ifeq ($(LIBTATSU_PKG),)
  LIBTATSU_PKG := $(shell pkg-config --exists libtatsu 2>/dev/null && echo libtatsu)
endif
ifeq ($(LIBTATSU),1)
  ifneq ($(LIBTATSU_PKG),)
    CXXFLAGS += -DPURPLEPOIS0N_HAVE_LIBTATSU $(shell pkg-config --cflags $(LIBTATSU_PKG))
    LIBS += $(shell pkg-config --libs $(LIBTATSU_PKG))
  else
    $(warning LIBTATSU=1 but pkg-config libtatsu not found)
  endif
else ifneq ($(LIBTATSU),0)
  ifneq ($(LIBTATSU_PKG),)
    CXXFLAGS += -DPURPLEPOIS0N_HAVE_LIBTATSU $(shell pkg-config --cflags $(LIBTATSU_PKG))
    LIBS += $(shell pkg-config --libs $(LIBTATSU_PKG))
  endif
endif

# Optional libusb (PongoOS USB client — auto-detect like libtatsu)
#   make release LIBUSB=1     — force enable if pkg-config finds libusb-1.0
LIBUSB_PKG := $(shell pkg-config --exists libusb-1.0 2>/dev/null && echo libusb-1.0)
ifeq ($(LIBUSB),1)
  ifneq ($(LIBUSB_PKG),)
    CXXFLAGS += -DPURPLEPOIS0N_HAVE_LIBUSB $(shell pkg-config --cflags $(LIBUSB_PKG))
    LIBS += $(shell pkg-config --libs $(LIBUSB_PKG))
  else
    $(warning LIBUSB=1 but pkg-config libusb-1.0 not found)
  endif
else ifneq ($(LIBUSB),0)
  ifneq ($(LIBUSB_PKG),)
    CXXFLAGS += -DPURPLEPOIS0N_HAVE_LIBUSB $(shell pkg-config --cflags $(LIBUSB_PKG))
    LIBS += $(shell pkg-config --libs $(LIBUSB_PKG))
  endif
endif

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
	@echo "  kpf      - Build kpf-purple Pongo module (legacy/scripts/kpf-build.sh all)"
	@echo "  smoke-kpf / smoke-kpf-data-only - Offline KPF patchfinder tests"
	@echo "  smoke-device-plan - --device-plan flag + JSON contract"
	@echo "  smoke-capabilities - --capabilities JSON contract"
	@echo "  smoke-mvp         - all offline MVP smokes + web build"
	@echo "  smoke-mvp-strict  - smoke-mvp + capabilities + rootless + mvp-gaps + test-fixtures"
	@echo "  smoke-e2e-delegate - hardware: already-jb store sync (needs UDID)"
	@echo "  smoke-hardware-validation - gap invariants + optional live device (UDID)"
	@echo "  smoke-recovery-chain      - recovery planner + CLI wiring smoke"
	@echo "  smoke-doctor / smoke-agent - Doctor + localhost agent"
	@echo "  LIBTATSU=1 - Link libtatsu for in-tree live TSS (brew install libtatsu)"
	@echo "  LIBUSB=1   - Link libusb for PongoOS USB (brew install libusb)"
	@echo "  submodules   - git submodule update --init external/ipsw"
	@echo "  external-libtatsu - build libtatsu to external/libtatsu-install"
	@echo "  external-ipsw  - build ipsw CLI in external/ipsw (requires Go)"
	@echo "  external-ipswd - build ipswd daemon in external/ipsw (requires Go)"
	@echo "  test-fixtures - offline backup/Mach-O smoke (tests/run_fixtures.sh)"
	@echo "  smoke-tss     - host TSS + chain report smoke (tests/smoke_tss.sh)"
	@echo "  smoke-futurerestore-parity - futurerestore/idevicerestore argv parity (offline)"
	@echo "  smoke-host-patch - offline kernelcache patchfind smoke"
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

external-libtatsu:
	@chmod +x scripts/build-libtatsu.sh
	@scripts/build-libtatsu.sh

test-fixtures: $(TARGET)
	@chmod +x tests/run_fixtures.sh 2>/dev/null || true
	@tests/run_fixtures.sh

smoke-tss: $(TARGET)
	@chmod +x tests/smoke_tss.sh tests/assert_chain_report.sh 2>/dev/null || true
	@tests/smoke_tss.sh

smoke-futurerestore-parity: $(TARGET)
	@chmod +x tests/smoke_futurerestore_parity.sh 2>/dev/null || true
	@tests/smoke_futurerestore_parity.sh

smoke-host-patch: $(TARGET)
	@chmod +x tests/smoke_host_patch.sh 2>/dev/null || true
	@tests/smoke_host_patch.sh

kpf:
	@chmod +x legacy/scripts/kpf-build.sh 2>/dev/null || true
	@legacy/scripts/kpf-build.sh all

smoke-kpf:
	@chmod +x tests/smoke_kpf_test.sh 2>/dev/null || true
	@tests/smoke_kpf_test.sh

smoke-kpf-data-only:
	@chmod +x tests/smoke_kpf_data_only.sh 2>/dev/null || true
	@tests/smoke_kpf_data_only.sh

smoke-dtree-mmio: $(TARGET)
	@chmod +x tests/smoke_dtree_mmio.sh 2>/dev/null || true
	@tests/smoke_dtree_mmio.sh

smoke-dfu-jailbreak: plugins
	@chmod +x tests/smoke_dfu_jailbreak.sh 2>/dev/null || true
	@tests/smoke_dfu_jailbreak.sh

smoke-medicine: $(TARGET)
	@chmod +x tests/smoke_medicine.sh 2>/dev/null || true
	@tests/smoke_medicine.sh

smoke-dpkg-store: $(TARGET)
	@chmod +x tests/smoke_dpkg_store.sh 2>/dev/null || true
	@tests/smoke_dpkg_store.sh

smoke-doctor: $(TARGET)
	@chmod +x tests/smoke_doctor.sh doctors/doctor_gui.py doctors/macos/run-doctor.command doctors/linux/run-doctor.sh 2>/dev/null || true
	@tests/smoke_doctor.sh

smoke-device-plan: $(TARGET)
	@chmod +x tests/smoke_device_plan.sh 2>/dev/null || true
	@tests/smoke_device_plan.sh

smoke-capabilities: $(TARGET)
	@chmod +x tests/smoke_capabilities.sh 2>/dev/null || true
	@tests/smoke_capabilities.sh

smoke-rootless-layout: $(TARGET)
	@chmod +x tests/smoke_rootless_layout.sh 2>/dev/null || true
	@tests/smoke_rootless_layout.sh

smoke-mvp: release
	@$(MAKE) smoke-kpf smoke-dfu-jailbreak smoke-dpkg-store smoke-doctor smoke-device-plan smoke-agent
	@$(MAKE) web-build
	@echo "smoke-mvp: all offline MVP checks passed"

smoke-mvp-strict: smoke-mvp
	@$(MAKE) smoke-capabilities smoke-rootless-layout smoke-mvp-gaps smoke-recovery-chain test-fixtures
	@echo "smoke-mvp-strict: extended offline checks passed"

web-install:
	cd ui/web && npm install

web-dev:
	cd ui/web && npm run dev

web-build:
	cd ui/web && npm ci && npm run build

agent:
	@chmod +x ui/agent/purple_agent.py
	python3 ui/agent/purple_agent.py

smoke-web:
	cd ui/web && npm ci && npm run build

smoke-agent: $(TARGET)
	@chmod +x tests/smoke_agent.sh ui/agent/purple_agent.py 2>/dev/null || true
	@tests/smoke_agent.sh

seed-store: $(TARGET)
	@chmod +x legacy/scripts/seed-store.sh legacy/packages/build-debs.py 2>/dev/null || true
	@legacy/scripts/seed-store.sh

smoke-e2e-delegate:
	bash tests/smoke_e2e_delegate.sh

smoke-mvp-gaps:
	@chmod +x tests/smoke_mvp_gaps.sh 2>/dev/null || true
	@tests/smoke_mvp_gaps.sh

smoke-recovery-chain: $(TARGET)
	@chmod +x tests/smoke_recovery_chain.sh 2>/dev/null || true
	@tests/smoke_recovery_chain.sh

smoke-hardware-validation: $(TARGET)
	@chmod +x tests/smoke_mvp_gaps.sh tests/smoke_hardware_validation.sh tests/smoke_dfu_jailbreak.sh tests/smoke_e2e_delegate.sh tests/smoke_recovery_chain.sh 2>/dev/null || true
	@tests/smoke_hardware_validation.sh

smoke-store-device: $(TARGET)
	@chmod +x tests/smoke_store_device.sh 2>/dev/null || true
	@tests/smoke_store_device.sh

tui-install:
	cd ui/tui && python3 -m venv .venv && .venv/bin/pip install -e . -q

tui: tui-install
	@cd ui/tui && .venv/bin/python -m purplepois0n_tui

.PHONY: all release debug clean install uninstall help plugins submodules external-ipsw external-ipswd external-libtatsu test-fixtures smoke-tss smoke-futurerestore-parity smoke-host-patch kpf smoke-kpf smoke-kpf-data-only smoke-dtree-mmio smoke-dfu-jailbreak smoke-medicine smoke-dpkg-store smoke-doctor smoke-device-plan smoke-capabilities smoke-rootless-layout smoke-mvp-gaps smoke-recovery-chain smoke-hardware-validation smoke-mvp smoke-mvp-strict web-install web-dev web-build agent smoke-web smoke-agent seed-store smoke-store-device smoke-e2e-delegate tui-install tui
