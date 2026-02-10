#!/bin/sh

# ---------------------------------------------------------------------------- #
# TS-Warp - rc script (macOS app)
# ---------------------------------------------------------------------------- #

# Copyright (c) 2021-2026, Mikhail Zakharov <zmey20000@yahoo.com>
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
# Start/stop rc-script
# Requires root priveleges to run
# ---------------------------------------------------------------------------- #

# Typically, the script is started by GUI-Warp application, but it can be run
# directly from CLI to control ts-warp daemon:
#
# sudo /Applications/gui-warp.app/Contents/Resources/ts-warp.sh start /Users/$USER/ts-warp
#

# -- CONFIGURATION VARIABLES ------------------------------------------------- #
[ -z "$2" -a "$USER" = "root" ] && _usage       # The USERNAME and prefix...
tswarp_prefix=${2-"/Users/$USER/ts-warp"}       # ...workaround for gui-warp.app

SCRIPTPATH=$(dirname $(readlink -f "$0"))
tswarp_inifile="$tswarp_prefix/etc/ts-warp.ini"
tswarp_pidfile="$tswarp_prefix/var/run/ts-warp.pid"
tswarp_logfile="$tswarp_prefix/var/log/ts-warp.log"
tswarp_actfile="$tswarp_prefix/var/spool/ts-warp/ts-warp.act"
tswarp_loglevel="2"
tswarp_logfile_maxsize=3145728
tswarp_options="-c $tswarp_inifile -l $tswarp_logfile -p $tswarp_pidfile -t $tswarp_actfile -d -v $tswarp_loglevel"

# ---------------------------------------------------------------------------- #
_act() {
    _status
    _check_root

    cat "$tswarp_pidfile" | xargs kill -USR2
}

# ---------------------------------------------------------------------------- #
_start() {
    _status
    case $? in
        0)
            printf "ts-warp is already running: "
            return 0
            ;;
        1)
            ;;
        2|3)
            printf "ts-warp status unknown: requesting 'stop' "
            _stop
            ;;
    esac

    _check_root
    printf "Starting ts-warp: "

    /sbin/pfctl -eq
    awk -v pf_conf="$tswarp_prefix"/etc/ts-warp_pf.conf '
            /ts-warp/       {next}
            /ns-warp/       {next}
            /rdr-anchor/    {rdrpos = NR}
                            {pf[NR] = $0}

        END {
            for (i = 0; i < NR; i++)
                if (i == rdrpos + 1) {
                    print("rdr-anchor \"ts-warp\"")
                    print("rdr-anchor \"ns-warp\"")
                } else
                    print(pf[i])
            print("anchor \"ts-warp\"")
            print("anchor \"ns-warp\"")
            printf("load anchor \"ts-warp\" from \"%s\"\n", pf_conf)
        }
    ' /etc/pf.conf | /sbin/pfctl -q -f -

    log_size=$(stat -f "%z" "$tswarp_logfile")

    # Truncate logfile if it's too big
    [ $tswarp_logfile_maxsize -lt $log_size ] && : > $tswarp_logfile

    echo $tswarp_options $* | xargs $SCRIPTPATH/ts-warp > /dev/null
}

# ---------------------------------------------------------------------------- #
_status() {
    # Set $1 if status message is required
    [ -s "$tswarp_pidfile" -a -r "$tswarp_pidfile" ] || {
        [ "$1" ] && printf "ts-warp is not running: "
        return 1
    }

    pgrep -F "$tswarp_pidfile" > /dev/null 2>&1
    case $? in
        0)
           [ "$1" ] && printf "ts-warp is running PID: %s: " $(cat "$tswarp_pidfile")
           return 0
           ;;
        1)
           [ "$1" ] && printf "ts-warp is NOT running PID: %s: " $(cat "$tswarp_pidfile")
           return 2
           ;;
        *)
           [ "$1" ] && printf "ts-warp is not found: "
           return 3
           ;;
    esac
}

# ---------------------------------------------------------------------------- #
_stop() {
    _check_root
    printf "Stopping ts-warp: "
    [ -f "$tswarp_pidfile" -a -s "$tswarp_pidfile" -a -r "$tswarp_pidfile" ] && {
        cat "$tswarp_pidfile" | xargs kill -TERM  > /dev/null 2>&1 || pkill -x ts-warp
    }

    [ -f "$tswarp_pidfile" ] && rm -f "$tswarp_pidfile"
    pkill -x ts-warp

        /sbin/pfctl -q -a ts-warp -F all
        /sbin/pfctl -q -a ns-warp -F all
}

# ---------------------------------------------------------------------------- #
_reload() {
    # Re-read configuration file on HUP signal
    _status && {
        _check_root
        printf "Reloading ts-warp: "
        cat "$tswarp_pidfile" | xargs kill -HUP
    } || {
        printf "Unable to reload: ts-warp is not running\n"
        exit 1
    }
}

# ---------------------------------------------------------------------------- #
_restart() {
    _check_root
    printf "Restarting ts-warp: "
    _stop
    _start $*
}

# ---------------------------------------------------------------------------- #
_check_root() {
    [ $(id -u) -ne 0 ] && {
        printf "Fatal: You must be root to proceed\n"
        exit 1
    }
}

# ---------------------------------------------------------------------------- #
_usage() {
    printf "Usage:\n\tts-warp.sh start|stop|reload|restart "/Users/USERNAME/ts-warp" [options]\n"
    printf "\tts-warp.sh status\n"
    exit 1
}

# ---------------------------------------------------------------------------- #
[ $# -eq 0 ] && _usage
case "$1" in
    [aA][cC][tT])                   _act                       ;;
    [sS][tT][aA][rR][tT])           shift; shift; _start $*    ;;
    [sS][tT][aA][tT][uU][sS])       _status 1                  ;;
    [sS][tT][oO][pP])               _stop                      ;;
    [rR][eE][lL][oO][aA][dD])       _reload                    ;;
    [rR][eE][sS][tT][aA][rR][tT])   shift; shift; _restart $*  ;;
    *) printf "Unknown command: %s\n" "$1"; _usage             ;;
esac

printf "done\n"
exit 0
