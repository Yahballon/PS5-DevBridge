ifdef PS5_PAYLOAD_SDK
include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
endif
OUTPUT ?= ../../dist/browser-reference-probe.elf
all: $(OUTPUT)
notify.o: ../../loader/notify.c
	$(CC) -O2 -c -o $@ $<
$(OUTPUT): main.cpp notify.o
	$(CXX) -std=c++20 -O2 -Wall -Wextra -o $@ main.cpp notify.o
