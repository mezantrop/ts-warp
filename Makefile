# -------------------------------------------------------------------------------------------------------------------- #
#  TS-Warp - Transparent Socks proxy server and traffic Wrapper                                                        #
# -------------------------------------------------------------------------------------------------------------------- #

# Copyright (c) 2021-2023, Mikhail Zakharov <zmey20000@yahoo.com>
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

CC = cc -O3
CFLAGS += -Wall -DPREFIX='"$(PREFIX)"' -DWITH_TCP_NODELAY=1
WARP_OBJS = inifile.o logfile.o natlook.o network.o pidfile.o pidlist.o socks.o http.o ts-warp.o utility.o xedec.o
PASS_OBJS = ts-pass.o xedec.o

.PHONY:	all clean examples-general examples-special install install-configs install-examples release uninstall version

all: ts-warp ts-warp.sh ts-warp_autofw.sh examples-special ts-pass

release: version all

version:
	sh ./version.sh RELEASE

ts-warp: $(WARP_OBJS)
	$(CC) -o $@ $(WARP_OBJS)

ts-warp.sh:
	sed 's|tswarp_prefix=.*|tswarp_prefix="$(PREFIX)"|' ts-warp.sh.in > ts-warp.sh

ts-warp_autofw.sh:
	sed 's|tswarp_prefix=.*|tswarp_prefix="$(PREFIX)"|' ts-warp_autofw.sh.in > ts-warp_autofw.sh

examples-general:
	@[ `id -u` -eq 0 ] || { echo "FATAL: You must run configuration targets only as normal (non-root) user!"; exit 1; }

	sed "s|%USER%|`whoami`|" ./examples/ts-warp_general_iptables.sh.in > ./examples/ts-warp_iptables.sh
	sed "s|%USER%|`whoami`|" ./examples/ts-warp_general_nftables.sh.in > ./examples/ts-warp_nftables.sh
	sed "s|%USER%|`whoami`|" ./examples/ts-warp_general_pf_freebsd.conf.in > ./examples/ts-warp_pf_freebsd.conf
	sed "s|%USER%|`whoami`|" ./examples/ts-warp_general_pf_macos.conf.in > ./examples/ts-warp_pf_macos.conf
	sed "s|%USER%|`whoami`|" ./examples/ts-warp_general_pf_openbsd.conf.in > ./examples/ts-warp_pf_openbsd.conf
	touch .configured

examples-special:
	@[ `id -u` -ne 0 ] || { echo "FATAL: You must run configuration targets only as normal (non-root) user!"; exit 1; }

	sed "s|%USER%|`whoami`|" ./examples/ts-warp_special_iptables.sh.in > ./examples/ts-warp_iptables.sh
	sed "s|%USER%|`whoami`|" ./examples/ts-warp_special_nftables.sh.in > ./examples/ts-warp_nftables.sh
	sed "s|%USER%|`whoami`|" ./examples/ts-warp_special_pf_freebsd.conf.in > ./examples/ts-warp_pf_freebsd.conf
	sed "s|%USER%|`whoami`|" ./examples/ts-warp_special_pf_macos.conf.in > ./examples/ts-warp_pf_macos.conf
	sed "s|%USER%|`whoami`|" ./examples/ts-warp_special_pf_openbsd.conf.in > ./examples/ts-warp_pf_openbsd.conf
	touch .configured

ts-pass: $(PASS_OBJS)
	$(CC) -o $@ $(PASS_OBJS)

install-examples:
    # ---------------------------------------------------------------------------------------------------------------- #
    # Examples (special version) are installed by default                                                              #
    # ---------------------------------------------------------------------------------------------------------------- #
	@[ -f .configured ] || { echo "FATAL: run \"make all\" command first!"; exit 1; }

	install -d $(PREFIX)/etc/
	install -m 644 ./examples/ts-warp.ini $(PREFIX)/etc/ts-warp.ini.sample
	@case `uname -s` in \
		Darwin) \
			install -m 644 ./examples/ts-warp_pf_macos.conf $(PREFIX)/etc/ts-warp_pf.conf.sample \
			;; \
		FreeBSD) \
			install -m 644 ./examples/ts-warp_pf_freebsd.conf $(PREFIX)/etc/ts-warp_pf.conf.sample \
			;; \
		OpenBSD) \
			install -m 644 ./examples/ts-warp_pf_openbsd.conf $(PREFIX)/etc/ts-warp_pf.conf.sample \
			;; \
		Linux) \
			install -m 755 ./examples/ts-warp_iptables.sh $(PREFIX)/etc/ts-warp_iptables.sh.sample; \
			install -m 755 ./examples/ts-warp_nftables.sh $(PREFIX)/etc/ts-warp_nftables.sh.sample \
			;; \
		*) \
			echo "Unsupported OS" \
			;; \
	esac

install-configs:
    # ---------------------------------------------------------------------------------------------------------------- #
    # Danger zone! The targer is not run by default, it overwrites INSTALLED configuration files                       #
    # ---------------------------------------------------------------------------------------------------------------- #
	@[ -f .configured ] || { echo "FATAL: run \"make all\" command first!"; exit 1; }

	install -d $(PREFIX)/etc/
	install -b -m 644 ./examples/ts-warp.ini $(PREFIX)/etc/ts-warp.ini
	@case `uname -s` in \
		Darwin) \
			install -b -m 644 ./examples/ts-warp_pf_macos.conf $(PREFIX)/etc/ts-warp_pf.conf \
			;; \
		FreeBSD) \
			install -b -m 644 ./examples/ts-warp_pf_freebsd.conf $(PREFIX)/etc/ts-warp_pf.conf \
			;; \
		OpenBSD) \
			install -b -m 644 ./examples/ts-warp_pf_openbsd.conf $(PREFIX)/etc/ts-warp_pf.conf \
			;; \
		Linux) \
			install -b -m 755 ./examples/ts-warp_nftables.sh $(PREFIX)/etc/ts-warp_nftables.sh; \
			install -b -m 755 ./examples/ts-warp_iptables.sh $(PREFIX)/etc/ts-warp_iptables.sh \
			;; \
		*) \
			echo "Unsupported OS" \
			;; \
	esac

install: ts-warp ts-warp.sh ts-warp_autofw.sh ts-pass install-examples
	@[ -f .configured ] || { echo "FATAL: run \"make all\" command first!"; exit 1; }

	install -d $(PREFIX)/bin/
	install -m 755 -s ts-warp $(PREFIX)/bin/
	install -m 755 -s ts-pass $(PREFIX)/bin/
	install -m 755 ts-warp_autofw.sh $(PREFIX)/bin/
	install -d $(PREFIX)/etc/
	install -m 755 ts-warp.sh $(PREFIX)/etc/
	install -d $(PREFIX)/var/log/
	install -d $(PREFIX)/var/run/

uninstall:
	rm -f $(PREFIX)/bin/ts-warp
	rm -f $(PREFIX)/bin/ts-pass
	rm -f $(PREFIX)/bin/ts-warp_autofw.sh
	rm -f $(PREFIX)/etc/ts-warp.sh
	rm -f $(PREFIX)/etc/ts-warp.ini.sample
	rm -f $(PREFIX)/etc/ts-warp_pf_openbsd.conf.sample
	rm -f $(PREFIX)/etc/ts-warp_pf_freebsd.conf.sample
	rm -f $(PREFIX)/etc/ts-warp_pf_macos.conf.sample
	rm -f $(PREFIX)/etc/ts-warp_nftables.sh.sample
	rm -f $(PREFIX)/etc/ts-warp_iptables.sh.sample
	rm -f $(PREFIX)/var/log/ts-warp.log
	rm -f $(PREFIX)/var/run/ts-warp.pid

clean:
	rm -rf ts-warp ts-warp.sh ts-warp_autofw.sh ts-pass *.o *.dSYM *.core examples/*.conf examples/*.sh .configured

inifile.o: inifile.h
natlook.o: natlook.h
network.o: network.h
logfile.o: logfile.h
pidfile.o: pidfile.h
pidlist.o: pidlist.h
socks.o: socks.h
http.o: http.h
ts-warp.o: ts-warp.h
utility.o: utility.h
xedec.o: xedec.h
