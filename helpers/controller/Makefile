ifdef PS5_PAYLOAD_SDK
include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
else
include /opt/ps5-payload-sdk/toolchain/prospero.mk
endif
OUTPUT ?= ../../dist/controller.elf
CXXFLAGS := -std=c++20 -O2 -Wall -Wextra -fno-exceptions -fno-rtti
all: $(OUTPUT)
$(OUTPUT): main.cpp one_shot.hpp remote_bind.hpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -Wl,--build-id=none -o $@ main.cpp

service: ../../dist/controller-service.elf
../../dist/controller-service.elf: service.cpp main.cpp one_shot.hpp remote_bind.hpp ../../loader/bridge_config.h
	$(CXX) $(CXXFLAGS) -Wl,--build-id=none -o $@ service.cpp
