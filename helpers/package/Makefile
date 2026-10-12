include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
all: ../../dist/package.elf
../../dist/package.elf: main.cpp
	$(CXX) -std=c++20 -O2 -Wall -Wextra -o $@ $<
