# ----------------------------------------------------------------------------------------------------------------------
# TS-Warp - Transparent SOCKS protocol Wrapper
# ----------------------------------------------------------------------------------------------------------------------

# Copyright (c) 2021, 2022, Mikhail Zakharov <zmey20000@yahoo.com>
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


# ----------------------------------------------------------------------------------------------------------------------
PREFIX ?= /usr/local

CC = cc
CFLAGS += -Wall -DPREFIX='"$(PREFIX)"'
WARP_OBJS = inifile.o logfile.o natlook.o network.o pidfile.o socks.o ts-warp.o utility.o xedec.o
PASS_OBJS = ts-pass.o xedec.o

.PHONY:	all clean install

all:	ts-warp ts-pass

ts-warp: $(WARP_OBJS)
	$(CC) -o $@ $(WARP_OBJS)

ts-pass: $(PASS_OBJS)
	$(CC) -o $@ $(PASS_OBJS)

install: ts-warp ts-pass
	install -d $(PREFIX)/bin/
	install -m 755 ts-warp $(PREFIX)/bin/
	install -m 755 ts-pass $(PREFIX)/bin/
	install -d $(PREFIX)/etc/
	sed 's|tswarp_prefix=.*|tswarp_prefix="$(PREFIX)"|' ts-warp.sh.in > ts-warp.sh
	install -m 755 ts-warp.sh $(PREFIX)/etc/
	install -m 644 ./examples/ts-warp.ini $(PREFIX)/etc/ts-warp.ini.sample
	install -m 644 ./examples/ts-warp_pf_openbsd.conf $(PREFIX)/etc/ts-warp_pf_openbsd.conf.sample
	install -m 644 ./examples/ts-warp_pf_freebsd.conf $(PREFIX)/etc/ts-warp_pf_freebsd.conf.sample
	install -m 644 ./examples/ts-warp_pf_macos.conf $(PREFIX)/etc/ts-warp_pf_macos.conf.sample
	install -m 755 ./examples/ts-warp_iptables.sh $(PREFIX)/etc/ts-warp_iptables.sh.sample
	install -d $(PREFIX)/var/log/
	install -d $(PREFIX)/var/run/

uninstall:
	rm -f $(PREFIX)/bin/ts-warp 
	rm -f $(PREFIX)/bin/ts-pass
	rm -f $(PREFIX)/etc/ts-warp.sh
	rm -f $(PREFIX)/etc/ts-warp.ini.sample
	rm -f $(PREFIX)/etc/ts-warp_pf_openbsd.conf.sample
	rm -f $(PREFIX)/etc/ts-warp_pf_freebsd.conf.sample
	rm -f $(PREFIX)/etc/ts-warp_pf_macos.conf.sample
	rm -f $(PREFIX)/etc/ts-warp_iptables.sh.sample
	rm -f $(PREFIX)/var/log/ts-warp.log
	rm -f $(PREFIX)/var/run/ts-warp.pid

clean:
	rm -rf ts-warp ts-warp.sh ts-pass *.o *.dSYM *.core

inifile.o: inifile.h
natlook.o: natlook.h
network.c: network.h
logfile.c: logfile.h 
pidfile.o: pidfile.h
socks.o: socks.h
ts-warp.o: ts-warp.h
utility.o: utility.h
