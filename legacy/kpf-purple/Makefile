# purplepois0n minimal KPF build (host kpf-test + Pongo module).
# Requires PONGO_ROOT (default: ../modern-era/PongoOS).

ifndef PONGO_ROOT
PONGO_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../modern-era/PongoOS
endif

ifndef HOST_OS
 ifeq ($(OS),Windows_NT)
  HOST_OS = Windows
 else
  HOST_OS := $(shell uname -s)
 endif
endif

SRC         := $(PONGO_ROOT)/src
INC         := $(PONGO_ROOT)/include
RA1N        := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
BUILD       := $(RA1N)/build
KPF_TEST    := $(RA1N)/kpf-test-main.c

CHECKRA1N_C := $(KPF_TEST) $(RA1N)/main.c $(RA1N)/shellcode.S \
               $(SRC)/drivers/xnu/xnu.c $(SRC)/drivers/xnu/xnu.S
CHECKRA1N_LDFLAGS := -Wl,-fatal_warnings -Wl,-dead_strip
CHECKRA1N_FLAGS := -std=gnu17 -Wall -Wunused-label -Werror -O3 -flto \
                   -I$(INC) -I$(SRC)/kernel -I$(SRC)/drivers \
                   -DCHECKRAIN_VERSION='"purplepois0n-0.1"' \
                   -Diprintf=printf -Dpanic=realpanic \
                   -DOVERRIDE_CACHEABLE_VIEW=0x800000000ULL -DDEV_BUILD=1 \
                   $(CHECKRA1N_LDFLAGS) $(KPF_CFLAGS) $(CFLAGS)

ifeq ($(HOST_OS),Darwin)
 IOS_CC   ?= xcrun -sdk iphoneos clang -arch arm64
 MACOS_CC ?= clang -arch arm64
 IOS_FLAGS   ?= -miphoneos-version-min=7.0
 MACOS_FLAGS ?= -mmacosx-version-min=11.0 \
                '-Djit_set_exec(m)=void pthread_jit_write_protect_np(int); pthread_jit_write_protect_np(m)'
 SIGN ?= codesign -s - --entitlements $(RA1N)/ent.plist
else
 ifeq ($(HOST_OS),Linux)
  IOS_CC ?= arm64-apple-ios12.0.0-clang -arch arm64
  SIGN   ?= ldid -Sent.plist
 endif
endif

EMBEDDED_CC ?= xcrun -sdk iphoneos clang
LIB         := $(PONGO_ROOT)/newlib/aarch64-none-darwin
EMBEDDED_LD_FLAGS ?= -nostdlib -static -Wl,-fatal_warnings -Wl,-dead_strip -Wl,-Z
EMBEDDED_CC_FLAGS ?= --target=arm64-apple-ios12.0 -std=gnu17 -Wall -Wunused-label -Werror -O3 -flto \
                     -ffreestanding -U__nonnull -nostdlibinc -DTARGET_OS_OSX=0 -DTARGET_OS_MACCATALYST=0 \
                     -I$(LIB)/include $(EMBEDDED_LD_FLAGS)
PONGO_MODULE_FLAGS ?= -DCHECKRAIN_VERSION='"purplepois0n-0.1"' -I$(INC) -I$(PONGO_ROOT)/apple-include \
                      -I$(SRC)/kernel -I$(SRC)/drivers -Wl,-kext -DDER_TAG_SIZE=8 -I$(SRC)/lib -DPONGO_PRIVATE=1

.PHONY: all clean kpf-test kpf-test.macos kpf-test.ios purplepois0n-kpf-pongo sync-check

all: kpf-test.macos

sync-check:
	@test -d "$(PONGO_ROOT)/src/drivers/xnu" || \
	  (echo "PongoOS not found at $(PONGO_ROOT); run legacy/scripts/kpf-build.sh sync" && exit 1)
	@test -f "$(KPF_TEST)" || \
	  (echo "Missing $(KPF_TEST); run legacy/scripts/kpf-build.sh sync" && exit 1)

kpf-test: kpf-test.macos

kpf-test.macos: sync-check $(CHECKRA1N_C)
	@mkdir -p $(BUILD)
	$(MACOS_CC) -o $(BUILD)/kpf-test-purple.macos $(CHECKRA1N_C) $(CHECKRA1N_FLAGS) $(MACOS_FLAGS)

kpf-test.ios: sync-check $(CHECKRA1N_C)
	@mkdir -p $(BUILD)
	$(IOS_CC) -o $(BUILD)/kpf-test-purple.ios $(CHECKRA1N_C) $(CHECKRA1N_FLAGS) $(IOS_FLAGS)
	$(SIGN) $(BUILD)/kpf-test-purple.ios

purplepois0n-kpf-pongo: sync-check $(RA1N)/main.c $(RA1N)/shellcode.S
	@mkdir -p $(BUILD)
	$(MAKE) -C "$(PONGO_ROOT)" newlib/aarch64-none-darwin/lib/libc.a
	$(EMBEDDED_CC) -o $(BUILD)/purplepois0n-kpf-pongo $(EMBEDDED_CC_FLAGS) $(PONGO_MODULE_FLAGS) \
	  $(RA1N)/main.c $(RA1N)/shellcode.S
	strip -x $(BUILD)/purplepois0n-kpf-pongo -s $(RA1N)/not_strip.txt
	strip -u $(BUILD)/purplepois0n-kpf-pongo -s $(RA1N)/not_strip.txt

clean:
	rm -rf $(BUILD)
