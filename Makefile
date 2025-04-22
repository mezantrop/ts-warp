PREFIX?=/usr/local
WITH_TCP_NODELAY?=1
WITH_LIBSSH2?=0
CPATH+=
LDLIBS+=
LDFLAGS+=
USER?=
CC=cc
CFLAGS += -O3 -Wall -DPREFIX='"$(PREFIX)"' -DWITH_TCP_NODELAY=$(WITH_TCP_NODELAY) -DWITH_LIBSSH2=$(WITH_LIBSSH2) $(CPATH)
WARP_OBJS = base64.o inifile.o logfile.o natlook.o network.o pidfile.o pidlist.o ssh2.o socks.o http.o ts-warp.o \
utility.o xedec.o

PASS_OBJS = ts-pass.o xedec.o

.PHONY:	all clean examples-general examples-special install install-configs install-examples release \
deinstall uninstall version

all: ts-warp examples-special ts-pass

release: version all

version:
	sh ./version.sh RELEASE

ts-warp: $(WARP_OBJS)
	$(CC) $(CFLAGS) $(WARP_OBJS) $(LDFLAGS) $(LDLIBS) -o $@

ts-warp.sh:
	sed 's|tswarp_prefix=.*|tswarp_prefix="$(PREFIX)"|' ts-warp.sh.in > ts-warp.sh

ts-warp_autofw.sh:
	sed 's|tswarp_prefix=.*|tswarp_prefix="$(PREFIX)"|' ts-warp_autofw.sh.in > ts-warp_autofw.sh

examples-general:
	sed "s|%USER%|$(USER)|" ./examples/ts-warp_general_iptables.sh.in > ./examples/ts-warp_iptables.sh
	sed "s|%USER%|$(USER)|" ./examples/ts-warp_general_nftables.sh.in > ./examples/ts-warp_nftables.sh
	sed "s|%USER%|$(USER)|" ./examples/ts-warp_general_pf_freebsd.conf.in > ./examples/ts-warp_pf_freebsd.conf
	sed "s|%USER%|$(USER)|" ./examples/ts-warp_general_pf_macos.conf.in > ./examples/ts-warp_pf_macos.conf
	sed "s|%USER%|$(USER)|" ./examples/ts-warp_general_pf_openbsd.conf.in > ./examples/ts-warp_pf_openbsd.conf

examples-special:
	sed "s|%USER%|$(USER)|" ./examples/ts-warp_special_iptables.sh.in > ./examples/ts-warp_iptables.sh
	sed "s|%USER%|$(USER)|" ./examples/ts-warp_special_nftables.sh.in > ./examples/ts-warp_nftables.sh
	sed "s|%USER%|$(USER)|" ./examples/ts-warp_special_pf_freebsd.conf.in > ./examples/ts-warp_pf_freebsd.conf
	sed "s|%USER%|$(USER)|" ./examples/ts-warp_special_pf_macos.conf.in > ./examples/ts-warp_pf_macos.conf
	sed "s|%USER%|$(USER)|" ./examples/ts-warp_special_pf_openbsd.conf.in > ./examples/ts-warp_pf_openbsd.conf

ts-pass: $(PASS_OBJS)
	$(CC) -o $@ $(PASS_OBJS)

install-examples:
# -------------------------------------------------------------------------------------------------------------------- #
# Examples (special version) are installed by default                                                                  #
# -------------------------------------------------------------------------------------------------------------------- #
	@[ -f .configured ] || { echo "FATAL: run \"./configure\" and \"make all\" commands first!"; exit 1; }

	install -d $(PREFIX)/etc/
	install -m 644 ./examples/ts-warp.ini $(PREFIX)/etc/ts-warp.ini.sample
	touch $(PREFIX)/etc/ts-warp.ini
	chmod 640 $(PREFIX)/etc/ts-warp.ini
	@case `uname -s` in \
		Darwin) \
			install -m 644 ./examples/ts-warp_pf_macos.conf $(PREFIX)/etc/ts-warp_pf.conf.sample; \
			touch $(PREFIX)/etc/ts-warp_pf.conf \
			;; \
		FreeBSD) \
			install -m 644 ./examples/ts-warp_pf_freebsd.conf $(PREFIX)/etc/ts-warp_pf.conf.sample; \
			touch $(PREFIX)/etc/ts-warp_pf.conf \
			;; \
		OpenBSD) \
			install -m 644 ./examples/ts-warp_pf_openbsd.conf $(PREFIX)/etc/ts-warp_pf.conf.sample; \
			touch $(PREFIX)/etc/ts-warp_pf.conf \
			;; \
		Linux) \
			install -m 755 ./examples/ts-warp_iptables.sh $(PREFIX)/etc/ts-warp_iptables.sh.sample; \
			install -m 755 ./examples/ts-warp_nftables.sh $(PREFIX)/etc/ts-warp_nftables.sh.sample; \
			touch $(PREFIX)/etc/ts-warp_iptables.sh $(PREFIX)/etc/ts-warp_nftables.sh \
			;; \
		*) \
			echo "Unsupported OS" \
			;; \
	esac

install-configs:
# -------------------------------------------------------------------------------------------------------------------- #
# Danger zone! The targer is not run by default, it overwrites INSTALLED configuration files                           #
# -------------------------------------------------------------------------------------------------------------------- #
	@[ -f .configured ] || { echo "FATAL: run \"./configure\" and \"make all\" commands first!"; exit 1; }

	install -d $(PREFIX)/etc/
	install -b -m 640 ./examples/ts-warp.ini $(PREFIX)/etc/ts-warp.ini
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

install: ts-warp.sh ts-warp_autofw.sh ts-pass install-examples
	@[ -f .configured ] || { echo "FATAL: run \"./configure\" and \"make all\" commands first!"; exit 1; }

	chown "`cat .configured`" $(PREFIX)/etc/ts-warp*

	install -d $(PREFIX)/bin/
	install -m 755 -s ts-warp $(PREFIX)/bin/
	install -m 755 -s ts-pass $(PREFIX)/bin/
	install -m 755 ts-warp_autofw.sh $(PREFIX)/bin/
	install -d $(PREFIX)/etc/
	install -m 755 ts-warp.sh $(PREFIX)/etc/
	install -d $(PREFIX)/var/log/
	install -d $(PREFIX)/var/run/
	install -d $(PREFIX)/var/spool/ts-warp/
	install -d $(PREFIX)/man/
	install -d $(PREFIX)/man/man1/
	install -d $(PREFIX)/man/man5/
	install -d $(PREFIX)/man/man8/
	install -m 755 man/ts-pass.1 $(PREFIX)/man/man1
	install -m 755 man/ts-warp.sh.1 $(PREFIX)/man/man1
	install -m 755 man/ts-warp.5 $(PREFIX)/man/man5
	install -m 755 man/ts-warp.8 $(PREFIX)/man/man8
	install -d $(PREFIX)/share/
	install -d $(PREFIX)/share/ts-warp/
	install -m 644 CHANGELOG.md $(PREFIX)/share/ts-warp
	install -m 644 CONTRIBUTORS.md $(PREFIX)/share/ts-warp
	install -m 644 LICENSE $(PREFIX)/share/ts-warp
	install -m 644 README.md $(PREFIX)/share/ts-warp

deinstall: uninstall
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
	rm -f $(PREFIX)/man/man1/ts-pass.1
	rm -f $(PREFIX)/man/man1/ts-warp.sh.1
	rm -f $(PREFIX)/man/man5/ts-warp.5
	rm -f $(PREFIX)/man/man8/ts-warp.8
	rm -f $(PREFIX)/share/ts-warp/CHANGELOG.md
	rm -f $(PREFIX)/share/ts-warp/CONTRIBUTORS.md
	rm -f $(PREFIX)/share/ts-warp/LICENSE
	rm -f $(PREFIX)/share/ts-warp/README.md
	rmdir $(PREFIX)/share/ts-warp/

clean:
	rm -rf ts-warp ts-warp.sh ts-warp_autofw.sh ts-pass *.o *.dSYM *.core examples/*.conf examples/*.sh .configured

base64.o: base64.h
inifile.o: inifile.h
natlook.o: natlook.h
network.o: network.h
logfile.o: logfile.h
pidfile.o: pidfile.h
pidlist.o: pidlist.h
socks.o: socks.h
http.o: http.h
ssh2.o: ssh2.h
ts-warp.o: ts-warp.h
utility.o: utility.h
xedec.o: xedec.h
