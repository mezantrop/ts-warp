# -------------------------------------------------------------------------------------------------------------------- #
# TS-Warp - Transparent proxy server and traffic wrapper                                                               #
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
PREFIX ?= .
SRCPATH = ../../../

CC = cc
CFLAGS += -O3 -Wall -DPREFIX='"$(PREFIX)"' -DWITH_TCP_NODELAY=1
WARP_FILES = $(SRCPATH)base64.c $(SRCPATH)inifile.c $(SRCPATH)logfile.c $(SRCPATH)natlook.c $(SRCPATH)network.c \
$(SRCPATH)pidfile.c $(SRCPATH)pidlist.c $(SRCPATH)ssh2.c $(SRCPATH)socks.c $(SRCPATH)http.c $(SRCPATH)ts-warp.c \
$(SRCPATH)utility.c $(SRCPATH)xedec.c
PASS_FILES = $(SRCPATH)ts-pass.c $(SRCPATH)xedec.c

.PHONY:	all clean ts-warp_x64 ts-warp_arm

all: ts-warp

ts-warp_x64:
	$(CC) $(CFLAGS) -o ts-warp_x64 -target x86_64-apple-macos10.12 $(WARP_FILES)
	$(CC) $(CFLAGS) -o ts-pass_x64 -target x86_64-apple-macos10.12 $(PASS_FILES)

ts-warp_arm:
	$(CC) $(CFLAGS) -o ts-warp_arm -target arm64-apple-macos11 $(WARP_FILES)
	$(CC) $(CFLAGS) -o ts-pass_arm -target arm64-apple-macos11 $(PASS_FILES)

ts-warp: ts-warp_x64 ts-warp_arm
	lipo -create -output ts-warp ts-warp_x64 ts-warp_arm
	lipo -create -output ts-pass ts-pass_x64 ts-pass_arm

clean:
	rm -rf ts-warp ts-warp_x64 ts-warp_arm ts-pass ts-pass_x64 ts-pass_arm *.o *.dSYM *.core
