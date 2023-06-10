# -------------------------------------------------------------------------------------------------------------------- #
# GUI frontend for TS-Warp - Transparent proxy server and traffic wrapper                                              #
# -------------------------------------------------------------------------------------------------------------------- #

# Copyright (c) 2022-2023 Mikhail Zakharov <zmey20000@yahoo.com>
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

.PHONY:	install

all: gui-warp.py gui-warp.ini

gui-warp.py:
	sed 's|prefix *= *.*|prefix = "$(PREFIX)"|' gui-warp.py.in > gui-warp.py

gui-warp.ini:
	sed 's|prefix *= *.*|prefix = $(PREFIX)|' gui-warp.ini.in > gui-warp.ini

install: all
	install -d $(PREFIX)/bin/
	install -m 755 gui-warp.py $(PREFIX)/bin/
	install -d $(PREFIX)/etc/
	install -m 755 gui-warp.ini $(PREFIX)/etc/

uninstall:
	rm -f $(PREFIX)/bin/gui-warp.py
	rm -f $(PREFIX)/etc/gui-warp.ini

clean:
	rm -rf __pycache__
	rm -f gui-warp.py
	rm -f gui-warp.ini
