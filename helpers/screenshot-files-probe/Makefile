ifdef PS5_PAYLOAD_SDK
include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
else
include /opt/ps5-payload-sdk/toolchain/prospero.mk
endif
OUTPUT ?= ../../dist/screenshot-files-probe.elf
CXXFLAGS := -std=c++20 -O2 -Wall -Wextra -fno-exceptions -fno-rtti
all: $(OUTPUT)
$(OUTPUT): main.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -Wl,--build-id=none -o $@ main.cpp
