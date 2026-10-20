#   Copyright (C) 2024 John TÃ¶rnblom
#
# This file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING. If not see
# <http://www.gnu.org/licenses/>.

PS5_HOST ?= ps5
PS5_PORT ?= 9020

ifdef PS5_PAYLOAD_SDK
    include $(PS5_PAYLOAD_SDK)/toolchain/prospero.mk
else
    $(error PS5_PAYLOAD_SDK is undefined)
endif

CFLAGS := -Wall -Werror -O1
LDADD  := -lSceSsl -lSceHttp

all: devbridge-loader.elf

browser-open.elf: browser_open.cpp ../app/src/browser_launcher.hpp
	$(CXX) -std=c++20 -O2 -o $@ browser_open.cpp
controller-service.elf: ../helpers/controller/service.cpp ../helpers/controller/main.cpp ../helpers/controller/one_shot.hpp ../helpers/controller/remote_bind.hpp bridge_config.h
	$(MAKE) -C ../helpers/controller service
	cp ../dist/controller-service.elf $@
package.elf: ../helpers/package/main.cpp
	$(MAKE) -C ../helpers/package
	cp ../dist/package.elf $@
photo-list.elf: ../helpers/screenshot-files-probe/main.cpp
	$(MAKE) -C ../helpers/screenshot-files-probe OUTPUT=$(CURDIR)/photo-list.elf
photo-read.elf: ../helpers/screenshot-file-reader/main.cpp
	$(MAKE) -C ../helpers/screenshot-file-reader OUTPUT=$(CURDIR)/photo-read.elf
apps.elf: ../helpers/apps/main.cpp
	$(MAKE) -C ../helpers/apps
	cp ../dist/apps.elf $@
memory-worker.elf: ../helpers/memory/main.cpp ../helpers/memory/protocol.hpp
	$(MAKE) -C ../helpers/memory
	cp ../dist/memory-worker.elf $@
socksrv.o: memory-worker.elf apps.elf bridge_config.h browser-open.elf controller-service.elf package.elf photo-list.elf photo-read.elf file_commands.h
bootstrap.o: socksrv.elf
main.o: bootstrap.elf

bootstrap.elf: bootstrap.o elfldr.o pt.o notify.o
	$(CC) -o $@ $^
	$(STRIP) $@

socksrv.elf: socksrv.o selfldr.o elfldr.o pt.o notify.o uri.o
	$(CC) -o $@ $^ $(LDADD)
	$(STRIP) $@

devbridge-loader.elf: main.o elfldr.o pt.o notify.o
	$(CC) -o $@ $^
	$(STRIP) $@

clean:
	rm -f *.o *.elf

test: devbridge-loader.elf
	$(PS5_DEPLOY) -h $(PS5_HOST) -p $(PS5_PORT) $^

.INTERMEDIATE: socksrv.elf bootstrap.elf
