ifdef PS5_PAYLOAD_SDK
include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
endif
OUTPUT ?= ../../dist/mono-metadata-query.elf
all: $(OUTPUT)
$(OUTPUT): main.cpp remote_query.hpp
	$(CXX) -std=c++20 -O2 -Wall -Wextra -o $@ main.cpp
