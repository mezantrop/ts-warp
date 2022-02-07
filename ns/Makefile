# -------------------------------------------------------------------------------------------------------------------- #
# NS-Warp - DNS responder/proxy                                                                                        #
# -------------------------------------------------------------------------------------------------------------------- #
# Copyright (c) 2022-2024, Mikhail Zakharov <zmey20000@yahoo.com>
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
# following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
#    disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
#    disclaimer in the documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


# -------------------------------------------------------------------------------------------------------------------- #
PREFIX ?= /usr/local

CC = cc
CFLAGS = -O3 -Wall -I ../
OBJECTS = ns-warp.o ns-inifile.o dns.o ../network.o ../utility.o ../logfile.o ../pidfile.o

.PHONY:	all clean install

all: ns-warp ns-warp.sh

ns-warp: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS)

ns-warp.sh:
	sed 's|nswarp_prefix=.*|nswarp_prefix="$(PREFIX)"|' ns-warp.sh.in > ns-warp.sh

install: ns-warp ns-warp.sh
	install -d $(PREFIX)/bin/
	install -m 755 -s ns-warp $(PREFIX)/bin/
	install -d $(PREFIX)/etc/
	install -m 755 ns-warp.sh $(PREFIX)/etc/

uninstall:
	rm -f $(PREFIX)/bin/ns-warp
	rm -f $(PREFIX)/etc/ns-warp_nftables.sh.sample
	rm -f $(PREFIX)/etc/ns-warp_iptables.sh.sample

clean:
	rm -rf ns-warp ns-warp.sh *.o *.core
