# CHANGELOG

* 2022.11.24    Current
  * `ts-warp.sh`: pkill when stop -f
  * Proper PID logging
  * `ts-warp.sh stop`: no redundant messages when autodetecting PID

* 2022.11.18    ts-warp-1.1.2, gui-warp-1.0, ns-warp-1.0.1
  * Workload balance crashes ts-warp under high workload. The option is disabled now.
  * `examples\ts-warp.ini`: updated for section Failover mode
  * `ts-warp.c`: Return configuration reload on SIGHUP

* 2022.11.13    ts-warp-1.1.1, gui-warp-1.0, ns-warp-1.0.1
  * `examples\ts-warp.ini`: Failover/Roundrobin/None modes
  * Minor text/typos fixes

* 2022.11.13    ts-warp-1.1.0, gui-warp-1.0, ns-warp-1.0.1
  * SOCKS servers section Failover/Roundrobin/None modes
  * Minor description changes
  * Finally section names allowed characters are: a-zA-Z0-9_\t -
  * Client's processes tracking - for future usage
  * `inifile.c`: String filter for non-ASCII, control characters
  * `ts-warp.c`: Offer `AUTH_METHOD_UNAME` only if the section contains `socks_user` value

* 2022.10.31    ts-warp-1.0.15, gui-warp-1.0, ns-warp-1.0.1
  * `natlook.c`, `natlook.h`: Unified *BSD data structures
  * `ns-warp`: `str2inet():`: updated; `malloc` issues fixed
  * `network.c/str2inet():` simplified; `inifile.c` memory optimization; `CHANGELOG.md` minor changes
  * `TCP_KEEPALIVE`: with defaults for OpenBSD
  * `socks.c`, `socks.h`: Drop connection (not exit) if a SOCKS server replies an error
  * `TCP_KEEPALIVE` enabled and respective options added where possible

* 2022.10.19    ts-warp-1.0.14, gui-warp-1.0, ns-warp-1.0.0
  * The first Github Release
  * `examples/ts-warp_pf*.conf`: PF-rules optimization, thanks Marcin Biczan
  * `TCP_NODELAY`: enabled for macOS
  * `Makefile`: `make release` target to build releases
  * `version.sh`: Compatibility with gawk

* 2022.10.17    ts-warp-1.0.12, gui-warp-1.0, ns-warp-1.0.0
  * `version.h`, `version.sh`: Auto-increment build number for the release or skip for normal build
  * `inifile.c`: C99 compatibility, thanks [@alitalaghat](https://github.com/alitalaghat)
  * `inifile.c`: `free()` INI-entries on `reload` initiated by `SIGHUP`
  * `WITH_TCP_NODELAY` option to speedup connections
  * `ns-warp.c` `usage()` update

* 2022.10.01    ts-warp-1.0.11, gui-warp-1.0, ns-warp-1.0.0
  * NS-Warp published
  * `inifile.c`: create_chains(): Chain list traversal fix; Thanks Bart Couvreur for the bug-hunting
  * `inifile.c`: `socks5_atype()`: Correct SOCKS5 address type selection
  * `inifile.c`: `socks5_atype()`: SOCKS5 Address type selector: IPv4/IPv6/Name
  * `socks5_request()`: SOCKS5_ATYPE_NAME added; `inifile.h`: NS_INI_ENTRY_NIT_POOL
  * `Makefile`: make uninstall
  * Minor decorative changes
  * `ts-warp.sh`: stop -f for macOS update
  * `ts-warp.sh`: Force mode for start/stop procedures rewritten
  * `examples/ts-warp_pf_macos.conf`: `en7` added as often used
  * `inifile.c`: `free(entry.val)` if unused
  * `inifile.c`: Skip variables not in sections
  * Chains & SOCKSv4 rewritten

* 2022.08.08    ts-warp-1.0.10, gui-warp-1.0
  * `ts-warp.c`: SOCKS Chains improvement
  * `inifile.c`: Lowcase in section names and better domain names parsing
  * `utility.c`: `toint()` wrapping
  * `usage()`: moved from `utility.c` to `ts-warp.c`
  * `utility.h`: Cleanup

* 2022.08.06    ts-warp-1.0.9, gui-warp-1.0
  * `logfile.c`: Logging moved out of `utility.c`
  * `README.md`: TODO: UDP redirection; Resolve remote names via SOCKS
  * `inifile.c`: `ini_look_server()`: `host[0] = 0` prevents garbage output if `getnameinfo()` fails. Thanks Juha Nurmela
  * `README.md` updated
  * `man\ts-warp.8`: updated
  * `man\ts-warp.5`: draft created
  * `README.md`: typos fixed
  * 120-chars width formatting
  * `ts-warp.c`: Log to STDOUT if TS-Warp is started in foreground: no `-d` option and no log-file `-l` is specified
  * `README.md`: updated

* 2022.06.11    ts-warp-1.0.8, gui-warp-1.0
  * Better logging
  * SOCKS4 added
  * `socks.c/socks.h`: `SOCKS5_ATYPE_*_LEN` for precise address-type matching
  * `utility.c`: mesg[256] -> mesg[STR_SIZE]
  * `iniline.c`: Set smaller buffers in `ini_look_server()`
  * `ts-warp.sh.in`: Force-start option '-f' removed from the default config
  * `network.c`: fixed `memset()` buffer overflow in `inet2str()`
  * `ts-warp.c`: fixed `printf()` -> `fprintf()` typo in `,main()` on mac
  * Close `/dev/pf` on exit
  * Github CodeQL vulnerability scan integration workflow
  * Documentation: manpages creation started under `man` directory

* 2022.06.02    ts-warp-1.0.7, gui-warp-1.0
  * `README.md` update
  * Minor issues fixed:
    * fixed unused variable warning on macOS for the `-u` switch
    * correced ts_pass program name in the help
  * README.md update
  * CLI option: `-h` reworked to show defaults
  * CLI option: `-u` - specify a `user` to run `ts-warp`

* 2022.05.23    ts-warp-1.0.6, gui-warp-1.0
  * `setuid()` to lower privileges for the daemon runtime to the level of the user `nobody`. Still `root` privileges are
    required to start the daemon. Note! On macOS `ts-warp` always runs under `root` user, because of the OS "features".
  * `natlook.c`: `pf_open()`, `pf_close` to optimize `nat_lookup()`
  * `README.md`: better wording
  * `inifile.c`: `str2inet()` buffer usage fix

* 2022.05.17    ts-warp-1.0.5, gui-warp-1.0
  * Logging enhanced
  * `ts-warp.sh`: `uname -o` -> `uname -s` for macOS compatibility
  * `ts-warp.sh`: custom CLI options
  * `ts-warp.sh`: updated for Linux
  * Examples updated for OpenBSD
  * OpenBSD compatibility and examples
  * Minor decorative changes
  * `utils` -> `utility` continue

* 2022.04.11    ts-warp-1.0.4, gui-warp-1.0
  * Code restructurization:
    * `utils` module renamed into `utility`
    * Network routines moved from `utility` and `socks` modules into `network`
    * Various minor cleanups and refactoring
  * Correct IPv6 address comparison
  * Enable ts-warp daemon acting as a SOCKS-server when NAT/redirection enabled
  * Manage non-NAT/redirected connnections with ts-warp daemon, so it's possible to specify ts-warp daemon as
    a SOCKS-server in userland. Don't forget to add the respective section with a real SOCKS server in `ts-warp.ini` or
    uncomment the `[DEFAULT]` one to catch all the unhandled requests.
  * Skip Domain check in `ini_look_server()` if target IP doesn't resolve

* 2022.03.29    ts-warp-1.0.3, gui-warp-1.0
  * Examples updated
  * `gui-warp.py` v1.0 release with installation via `make install`
  * `gui-warp.py` INI-, LOG-, FW- tabs with save/control buttons
  * `gui-warp.py` configuration tab started
  * `gui\gui-warp.py`: ts-warp daemon status, log-file listing, buttons work
  * `gui\ts-warp.py` added. Note: the most of the buttons/functions are unusable
  * `ts-warp.sh`: `stop()` waits until the PID-file is removed
  * Stay calm and don't `exit()` daemon on the most of client issues
  * Reject clients with no real-destination addresses
  * Documentation/examples update

* 2022.03.15    ts-warp-1.0.2
  * `-i` option replaces `-I` in CLI arguments
  * Full Linux with iptable code integration
  * PID-file option added to CLI parameters
  * Support for domain and hostnames in target definitions of `ts-warp.ini`
  * `getnameinfo()` fix to work correctly on Linux
  * To prevent сonfiguration files and scripts from being overwritten,
    `make install` copies them into `<PREFIX>/etc` with `.sample` postfix

* 2022.03.11    ts-warp-1.0.1
  * make install
  * `-I IP:Port` replaces `-I IP -i Port`; minor optimizations
  * INI-file parser to ignore variables without values
  * Fixed processed empty/nonexisting `socks_server` values in the INI-file
  * Remove processed `chain_list` elements in `create_chains()`
  * Better messaging in `ts-warp.sh` start/stop script
  * `natlook()` dest port printout on Mac, fixed; thanks Alicja Michalska <alka96@protonmail.com> for testing

* 2022.03.05    ts-warp-1.0.0
  * Starting point to track versions
