include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
all: ../../dist/apps.elf
../../dist/apps.elf: main.cpp
	$(CXX) -std=c++20 -O2 -Wall -Wextra -o $@ $<
