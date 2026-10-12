ifdef PS5_PAYLOAD_SDK
include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
endif
OUTPUT ?= ../../dist/browser-process-probe.elf
all: $(OUTPUT)
$(OUTPUT): main.cpp
	$(CXX) -std=c++20 -O2 -Wall -Wextra -o $@ $<
