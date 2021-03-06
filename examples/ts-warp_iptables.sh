#!/bin/sh

# ---------------------------------------------------------------------------- #
# TS-Warp - IPTABLES configuration for Linux example                           #
# ---------------------------------------------------------------------------- #

iptables -t nat -N SOCKS

# Exclude SOCKS servers
iptables -t nat -A SOCKS -d 10.0.12.1,192.168.1.237,123.45.1.11 -j RETURN

# Destination network/address definitions
iptables -t nat -A SOCKS -p tcp -d 10.0.10.0/16 -j REDIRECT --to-ports 10800
iptables -t nat -A SOCKS -p tcp -d 192.168.1.0/24,192.168.3.0/24 -j REDIRECT --to-ports 10800
iptables -t nat -A SOCKS -p tcp -d 123.45.123.0/24,123.45.234.96/24 -j REDIRECT --to-ports 10800

# NB! User 'frodo' is a process owner username of the applictions to redirect to
# SOCKS servers. Change it to your username on localhost.
iptables -t nat -A OUTPUT -p tcp -m owner --uid-owner frodo -j SOCKS
