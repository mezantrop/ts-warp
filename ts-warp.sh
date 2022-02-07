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


# ------------------------------------------------------------------------------
# Start/stop rc-script
# Requires sudo or root priveleges to run
# ------------------------------------------------------------------------------


# -- CONFIGURATION VARIABLES ---------------------------------------------------
PREFIX="/usr/local"
PID_FILE="/var/run/ts-warp.pid"

# ------------------------------------------------------------------------------
start() {
    status || {
        check_root
        printf "Starting ts-warp\n"
        sudo /sbin/pfctl -f "$PREFIX"/etc/ts-warp_pf.conf > /dev/null
        sudo "$PREFIX"/bin/ts-warp -d > /dev/null
    }
}

status() {
    [ -f "$PID_FILE" -a -r "$PID_FILE" ] && {
        printf "ts-warp is running PID: %s\n" `cat "$PID_FILE"`;
        return 0;
    } || {
        printf "ts-warp is not running\n";
        return 1;
    }
}

stop() {
    status && {
        check_root
        printf "Stopping ts-warp\n"
        cat "$PID_FILE" | xargs sudo kill -TERM
    }
}

reload() {
    # Re-read configuration file on HUP signal
    # TODO: Implement the signal in ts-warp daemon
    check_root
    printf "Fatal: reload is not implemented yet; Use restart\n" && exit

    printf "Reload ts-warp\n"
    sudo cat "$PID_FILE" | xargs kill -HUP
}

restart() {
    check_root
    printf "Restarting ts-warp\n"
    stop
    start
}

check_root() {
    [ `id -u` -ne 0 ] && {

        which sudo > /dev/null || {
            printf "Fatal: sudo not found\n" && exit 1; 
        }

        sudo -l /sbin/pfctl > /dev/null || {
            printf "Fatal: not allowed to run pfctl as root\n" && exit 1;
        }
        sudo -l "$PREFIX"/bin/ts-warp > /dev/null || {
            printf "Fatal: not allowed to run ts-warp as root\n" && exit 1;
        }
    }
}

[ $# -ne  1 ] && 
    printf "Usage: %s start|stop|restart|reload|status\n" `basename "$0"` &&
    exit 1

case "$1" in
    [sS][tT][aA][rR][tT])           start   ;;
    [sS][tT][aA][tT][uU][sS])       status  ;;
    [sS][tT][oO][pP])               stop    ;;
    [rR][eE][lL][oO][aA][dD])       reload  ;;
    [rR][eE][sS][tT][aA][rR][tT])   restart ;;
    *) printf "Unknown command\n"; exit 1;  ;;
esac

printf "Done\n"
exit 0
