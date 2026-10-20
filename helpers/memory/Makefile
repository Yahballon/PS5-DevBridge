include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
all: ../../dist/memory-worker.elf
../../dist/memory-worker.elf: main.cpp protocol.hpp
	@mkdir -p $(@D)
	$(CXX) -std=c++20 -O2 -Wall -Wextra -o $@ main.cpp
lab: ../../dist/memory-lab.elf
../../dist/memory-lab.elf: lab.cpp
	@mkdir -p $(@D)
	$(CXX) -std=c++20 -O2 -Wall -Wextra -o $@ lab.cpp
