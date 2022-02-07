#!/bin/sh

# TS-Warp - Transparent SOCKS protocol Wrapper
#
# Copyright (c) 2021, 2022, Mikhail Zakharov <zmey20000@yahoo.com>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


# ---------------------------------------------------------------------------- #
# IPTABLES configuration example                                               #
# ---------------------------------------------------------------------------- #

iptables -t nat -N SOCKS

# Exclude SOCKS servers
iptables -t nat -A SOCKS -d 9.45.121.62,192.168.1.237,129.39.133.102 -j RETURN

# Destination network/address definitions
iptables -t nat -A SOCKS -p tcp -d 10.101.0.0/16 -j REDIRECT --to-ports 10800
iptables -t nat -A SOCKS -p tcp -d 192.168.1.0/24,192.168.3.0/24 -j REDIRECT --to-ports 10800
iptables -t nat -A SOCKS -p tcp -d 129.39.133.0/24,129.39.143.0/24 -j REDIRECT --to-ports 10800

# NB! User 'frodo' is a process owner username of the applictions to redirect to
# SOCKS servers. Change it to your username on localhost.
iptables -t nat -A OUTPUT -p tcp -m owner --uid-owner frodo -j SOCKS
