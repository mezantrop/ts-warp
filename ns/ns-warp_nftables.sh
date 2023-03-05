#!/bin/sh

# ---------------------------------------------------------------------------- #
# NS-Warp - NFTABLES configuration for Linux example                           #
# ---------------------------------------------------------------------------- #

nft add chain ip nat NS-WARP

nft add rule ip nat NS-WARP ip protocol udp dport 53 counter redirect to :35353
nft add rule ip nat OUTPUT ip protocol udp skuid != nobody counter jump NS-WARP
