#!/bin/sh

# ---------------------------------------------------------------------------- #
# NS-Warp - IPTABLES configuration for Linux example                           #
# ---------------------------------------------------------------------------- #

iptables -t nat -N NS-WARP

iptables -t nat -A NS-WARP -p udp --dport 53 -j REDIRECT --to-ports 35353
iptables -t nat -A OUTPUT -p udp -m owner --uid-owner != nobody -j NS-WARP
