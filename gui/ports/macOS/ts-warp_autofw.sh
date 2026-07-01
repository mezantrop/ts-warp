#!/bin/sh

# ---------------------------------------------------------------------------- #
# TS-Warp - Firewall configuration builder (mac)                               #
# ---------------------------------------------------------------------------- #

# Copyright (c) 2023-2026, Mikhail Zakharov <zmey20000@yahoo.com>
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
tswarp_prefix="$1"
tsw_ini=$tswarp_prefix/etc/ts-warp.ini

# ---------------------------------------------------------------------------- #
nl="
"

# ---------------------------------------------------------------------------- #
make_networks() {
    # Collect target networks

    nets_raw=$(printf "%s\n" "$cfg" | grep target_network | cut -f 2 -d '=')
    nets=""
    for net in $nets_raw; do
        netw=$(printf "%s" $net | cut -f 1 -d '/')
        mask=$(printf "%s" $net | cut -f 2 -d '/')

        chk_ip "$netw" || return 1

        if [ $(echo $mask | wc -c) -lt 4 ]; then
            # Assuming a CIDR mask
            [ $mask -le 32 ] || return 1

        elif [ $(echo $mask | grep '\.') ]; then
            # Supposing it's a four-octet mask

            chk_ip "$mask" || return 1

            bits=''
            for oct in $(echo $mask | sed 's/\./ /g'); do
                bits=$bits$(echo "obase=2; ibase=10; ${oct}"|bc)
            done
            mask=$(printf "$bits" | tr -d '0' | wc -c | tr -d " \n")
        else
            # Spoiled mask value
            continue
        fi

        nets="$nets""$netw"/"$mask""$nl"
    done

    eval $1='$nets'
    return 0
}


# ---------------------------------------------------------------------------- #
make_ranges() {
    # Collect ranges information

    rngs_raw=$(printf "%s\n" "$cfg" |
        grep target_range |
        cut -f 2 -d '=')

    rngs=""
    for range in $rngs_raw; do
        s_ip=$(printf "%s" $range | cut -f 1 -d '/')
        e_ip=$(printf "%s" $range | cut -f 2 -d '/')

        chk_ip "$s_ip" || return 1
        chk_ip "$e_ip" || return 1

        rngs=${rngs}$(awk -v start="$s_ip" -v end="$e_ip" '
                function ip2int(ip, a) {
                        split(ip,a,".")
                        return (((a[1]*256+a[2])*256+a[3])*256+a[4])
                }

                function int2ip(n) {
                        return int(n/16777216) "." \
                        int(n/65536)%256 "." \
                        int(n/256)%256 "." \
                        n%256
                }

                BEGIN {
                        s = ip2int(start)
                        e = ip2int(end)

                        while (s <= e) {
                                # largest aligned block
                                block = 1
                                while ((s % (block * 2) == 0) && (block < 2147483648))
                                block *= 2

                                # do not run past end
                                while (block > (e - s + 1))
                                block /= 2

                                # block size -> prefix
                                prefix = 32
                                b = block
                                while (b > 1) {
                                        prefix--
                                        b /= 2
                                }

                                print (int2ip(s) "/" prefix)

                                s += block
                        }
                }')${nl}
    done

    eval $1='$rngs'
    return 0
}

# ---------------------------------------------------------------------------- #
make_hosts() {
    # Collect hosts information

    hosts_raw=$(printf "%s\n" "$cfg" |
        grep target_host |
        tr -d " \t" | awk -F '[=:]' '{print $2}')

    hosts=""
    for hst in $hosts_raw; do
        chk_ip $hst && hosts="$hosts""$hst""$nl" ||
            hosts="$hosts"$(host "$hst" |
                grep 'has address' |
                cut -f 4 -d ' ')"$nl"
    done

    eval $1='$hosts'
    return 0
}

# ---------------------------------------------------------------------------- #
make_domains() {
    # Collect domains information

    domains_raw=$(printf "%s\n" "$cfg" |
        grep target_domain |
        tr -d " \t" | awk -F '[=:]' '{print $2}')

    domains=""
    for dom in $domains_raw; do
        domains="$domains"$(host "$dom" |
            grep 'has address' |
            cut -f 4 -d ' ')"$nl"
    done

    eval $1='$domains'
    return 0
}

# ---------------------------------------------------------------------------- #
make_proxy() {
    # Collect proxy server information

    sservers_raw=$(printf "%s\n" "$cfg" |
        grep -E 'socks_server|proxy_server' |
        tr -d " \t" | awk -F '[=:]' '{print $2}')

    sservers=""
    for srv in $sservers_raw; do
        chk_ip $srv && sservers="$sservers""$srv""$nl" ||
            sservers="$sservers"$(host "$srv" |
                grep 'has address' |
                cut -f 4 -d ' ')"$nl"
    done

    eval $1='$sservers'
    return 0
}

# ---------------------------------------------------------------------------- #
chk_ip() {
    # Validate IP

    echo "$1" | grep -q -E "\
^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.\
(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.\
(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.\
(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$" && return 0 || return 1
}

# ---------------------------------------------------------------------------- #
ip2octets() {
    # Convert an IP-address ($1) into 4 octets and return them
    # as ($2, $3, $4, $5) variables

    read o1 o2 o3 o4 << EOF
$(echo $1 | sed 's/\./ /g')
EOF

    eval $2='$o1'
    eval $3='$o2'
    eval $4='$o3'
    eval $5='$o4'
}

# ---------------------------------------------------------------------------- #
make_conf_pf() {
    # Make PF configuration

    rslt=""
    for n in $networks; do
        _nets="$_nets""$n, "
    done
    rslt="$rslt""$_nets"'\'"$nl"

    for r in $ranges; do
        _rngs="$_rngs""$r, "
    done
    rslt="$rslt""$_rngs"'\'"$nl"

    for h in $hosts; do
        _hsts="$_hsts""$h, "
    done
    rslt="$rslt""$_hsts"'\'"$nl"

    for d in $domains; do
        _doms="$_doms""$d, "
    done
    rslt="$rslt""$_doms"'\'"$nl"

    for s in $sservers; do
        _ssrv="$_ssrv"!"$s, "
    done
    rslt="$rslt""$_ssrv"'\'"$nl"

    cat <<EOF
table <$TSW_TNAME> { \\
`printf "%s" "$rslt" | sed '$! { P; D; }; s|...$| \\\|'`
}

$pf_rules

EOF
}

# ---------------------------------------------------------------------------- #
make_conf_nftables() {
    # Make NFTABLES configuration

    cat << EOF
nft add chain ip nat $TSW_TNAME

$(for s in $sservers; do
    printf "nft add rule ip nat %s ip daddr %s counter return\n" $TSW_TNAME $s
done)

$(for n in $networks; do
    printf "nft add rule ip nat %s ip protocol tcp ip daddr %s counter redirect to :%d\n" $TSW_TNAME $n $TSW_PORT
done)

$(for r in $ranges; do
    printf "nft add rule ip nat %s ip protocol tcp ip daddr %s counter redirect to :%d\n" $TSW_TNAME $r $TSW_PORT
done)

$(for h in $hosts; do
    printf "nft add rule ip nat %s ip protocol tcp ip daddr %s counter redirect to :%d\n" $TSW_TNAME $h $TSW_PORT
done)

nft add rule ip nat OUTPUT ip protocol tcp skuid $TSW_USER counter jump $TSW_TNAME
EOF
}

# ---------------------------------------------------------------------------- #
make_conf_iptables() {
    # Make IPTABLES configuration

    cat << EOF
iptables -t nat -N $TSW_TNAME

$(for s in $sservers; do
    printf "iptables -t nat -A %s -d %s -j RETURN\n" $TSW_TNAME $s
done)

$(for n in $networks; do
    printf "iptables -t nat -A %s -p tcp -d %s -j REDIRECT --to-ports %d\n" $TSW_TNAME $n $TSW_PORT
done)

$(for r in $ranges; do
    printf "iptables -t nat -A %s -p tcp -d %s -j REDIRECT --to-ports %d\n" $TSW_TNAME $r $TSW_PORT
done)

$(for h in $hosts; do
    printf "iptables -t nat -A %s -p tcp -d %s -j REDIRECT --to-ports %d\n" $TSW_TNAME $h $TSW_PORT
done)

iptables -t nat -A OUTPUT -p tcp -m owner --uid-owner $TSW_USER -j $TSW_TNAME
EOF
}

# ---------------------------------------------------------------------------- #
TSW_TNAME='TSWARP'
TSW_USER=$(whoami)
TSW_PORT=${2-10800}

[ $(id -u) -eq 0 ] && {
    printf "Fatal: You must not be root to proceed\n"
    exit 1
}

cfg=$(cat $tsw_ini |
    cut -f 1 -d ';' |
    cut -f 1 -d '#' |
    tr -d '\t ' |
    tr -s '\n\n' '\n')

make_networks networks
make_ranges ranges
make_hosts hosts
make_domains domains
make_proxy sservers

case $(uname -s) in
    Darwin|FreeBSD)
        pf_rules="rdr pass log on lo0 inet proto tcp from any to { <$TSW_TNAME> } -> 127.0.0.1 port $TSW_PORT
pass out on !lo0 route-to lo0 inet proto tcp from any to { <$TSW_TNAME> } keep state user $TSW_USER"
        make_conf_pf
        ;;

    OpenBSD)
        pf_rules="pass in log on lo0 inet proto tcp from any to { <$TSW_TNAME> } rdr-to 127.0.0.1 port $TSW_PORT
pass out on !lo0 inet proto tcp from any to { <$TSW_TNAME> } route-to lo0 keep state user $TSW_USER"
        make_conf_pf
        ;;

    Linux)
        iptables -V > /dev/null &&
            make_conf_iptables || {
                nft -v > /dev/null &&
                    make_conf_nftables ||
                        printf "FATAL: Unable to find iptables nor nftables"
                        exit 1
            }
        ;;

    *)
        printf "FATAL: Unsupported OS\n"
        exit 1
        ;;
esac
