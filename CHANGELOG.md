# CHANGELOG

* **2025.06.05    Current - ts-warp-1.5.8, gui-warp-1.0.27 (gui-warp-v1.0.34-mac), ns-warp-1.0.7**
  * `ts-warp.c, pidlist.c`: better `PID`-list management
  * `inifile.c`: suppress negative name lookup messages
  * `gui-warp.py`: INI-tab Text-field resizing corrected

* **2025.06.04    ts-warp-1.5.8, gui-warp-1.0.27 (gui-warp-v1.0.34-mac), ns-warp-1.0.7**
  * `build.sh`: `libssl/libssh` section tweaks
  * `ts-warp.c`: `SIGHUP` hooked to recreate/reopen the log-file
  * `ts-warp.c`: trap signals for the `console` mode as well
  * `minmiconf:` Updated to version `1.1.1.1`
  * `Makefile`: updated to fail `make install` if `./configure` and `make all` were not run
  * `configure`: Do not fail on macOS SDK absence, if `gcc` is detected on `macOS`
  * `inifile.c`: "Long" dotted-decimal netmasks style returned, so `255.255.0.0` and `/16` both working
  * `inifile.c`: ignore netmasks for `target_host`
  * `inifile.c`: correct `/32` netmask for `target_network`
  * `gui-warp.py`/`gui-warp.py.in`: 'Password hash' caption
  * `CONTRIBUTORS.md`: New contributor Marcin Nowicki <marcin.nowicki@kyndryl.com>. Thanks for your ideas and testing!
  * `build.sh`: `-no-tests` added for OpenSSL `Configure`; `-fembed-bitcode` removed for both OpenSSL and libssh2
  * `gui-warp.py`: Highlight "Save" button for INI/FW if config has been edited. Thanks Radek Sipka for the request

* **2024.11.08    ts-warp-1.5.7, gui-warp-1.0.26 (gui-warp-v1.0.33-mac), ns-warp-1.0.7**
  * `ts-warp.sh`: Log-file truncating moved here
  * `gui-warp.py`: Minor changes about log-file size and FW-file selection under `Linux`
  * `ts-warp.c`: Null-pointer dereference in `PIDS` table processing fixed
  * `ts-warp.c`: `ACT` reporting `[SECTIONS]`
  * `inifile.c`: Correct usage of multiple links of a proxy-chain
  * `inifile.c`: Potential buffer-overflow fixed in parsing `[SECTIONS]`
  * `ts-warp.c`: Proxy server selection optimization
  * `pidlist.c`: `Uninitialised value was created by a heap allocation` fixed

* **2024.09.07    ts-warp-1.5.6, gui-warp-1.0.25 (gui-warp-v1.0.32-mac), ns-warp-1.0.7**
  * `GUI-Warp.app`: Better handling of `starter`, `starter.c` updated to v1.3
  * `GUI-Warp.app`: `starter.c` updated to v1.2
  * `inifile.c`: `proxy_server` variable parsing. Thanks <jesussanchezvalle@gmail.com> for the bug-hunting and testing
  * `GUI-Warp.app`: `starter.c` to handle environment variables
  * `minmiconf`: Updated to version `1.1.1.0`
  * `configure`: Added lost paths check for macOS SDK directories
  * `GUI-Warp.app`: Added an internal launcher - `starter.c`
  * `ts-warp.sh`: Enable `PF`-firewall in `ts-warp.sh` `start`; Minor fixes for `PF` on `macOS` in `GUI-Warp` on `stop`
  * `pidlist.c`: `ACT` shows destination IP-address, when internal proxy servers are used
  * `ts-warp.c`: `-D 0..512` DPI bypass fragment size by default 0 - disabled; To enable use a positive value, e.g., 2

* **2024.08.15    ts-warp-1.5.5, gui-warp-1.0.25 (gui-warp-v1.0.30-mac), ns-warp-1.0.7**
  * `ts-warp.c`, `http.c`: Deep Packet Inspections bypass. Option `-D` to disable it.
  * `ts-warp.c`: Internal proxy servers allowed making direct connection to destinations

* **2024.07.23    ts-warp-1.5.4, gui-warp-1.0.25 (gui-warp-v1.0.30-mac), ns-warp-1.0.7**
  * `ts-warp.c`: `ACT`-file created as `RUNAS_USER` user owner
  * `ts-warp.c`: On `macOS` delayed `setuid()`/`setgid()` disabled as almost useless

* **2024.07.23    ts-warp-1.5.3, gui-warp-1.0.25 (gui-warp-v1.0.30-mac), ns-warp-1.0.7**
  * `ts-warp.c`: Enable on `macOS` delayed `setuid()`/`setgid()` to run as non-privileged user
  * `ts-warp.c`: SSH2 proxy connection fixed, thanks Stefan Hildebrandt for the bug-hunting
  * `ssh2.c`: `password` for `keyboard-interactive` and `password` now always required
  * `examples\ts-warp.ini`: Highlight info about accessing `proxy_key` (the private key) permissions/location

* **2024.07.12    ts-warp-1.5.2, gui-warp-1.0.25 (gui-warp-v1.0.30-mac), ns-warp-1.0.7**
  * `GUI-Warp`: `About`-tab displays `CHANGELOG.md`-file
  * `Makefile`: installs/deinstalls text-files files into/from `<PREFIX>/share/ts-warp` directory
  * `GUI-Warp.app`: `ts-warp.sh` can be run from `CLI` to control `ts-warp` daemon e.g.:
      `sudo /Applications/gui-warp.app/Contents/Resources/ts-warp.sh start /Users/$USER/ts-warp`
  * `GUI-Warp.app`: `build.sh` uses current `openssl` and `libssh2` versions
  * `Makefile`: `WITH_LIBSSH2?=0` to disable `LIBSSH2` by default
  * `ts-warp_autofw.sh` Ignore domain names if they are not resolveable to IP-addresses
  * Typos fixed again
  * `gui-warp.py`: Minor cosmetic changes
  * `minmiconf`: `v1.1.0.2` - OpenBSD support; `ts-warp_auto.sh` modernization; `gui-warp.py`: Save INI button fix
  * `gui-warp.py`: `v1.0.30-mac`, `gui-warp.py.in`: `v1.0.25`, `minmiconf`: `v1.1.0.1`
  * `configure`: updated to `minmiconf-1.1.0.0`
  * Ignore a section if `proxy_server` value is not specified

* **2024.04.09    ts-warp-1.5.1, gui-warp-1.0.24 (gui-warp-v1.0.29-mac), ns-warp-1.0.7**
  * `ts-warp_autofw.sh`: Support for domains in response to the [issue](https://github.com/mezantrop/ts-warp/issues/8)
  * `configure`: Better `PREFIX` handling
  * `README.md`: Rewritten for `configure`
  * `configure`: Configure script introduced
  * GUI-Warp application window title caption update
  * `ts-warp.c`: `WITH_TCP_NODELAY` proper processing

* **2024.03.12    ts-warp-1.5.0, gui-warp-1.0.23 (gui-warp-v1.0.28-mac), ns-warp-1.0.7**
  * `gui-warp.app`, `gui-warp.py`: Correcting paths in `gui-warp.ini`
  * `gui-warp.app`: Symlink to `/Applications` in `DMG`
  * `gui-warp.py`, `gui-warp.app`: `gui-warp.ini` back to macOS; `daemon_options` var controls `ts-warp` startup options
  * `ssh2.c`: Deny SSH2-proxy connections with no username
  * `ssh2.c`: Do not try manual authentication is agent-authentication was successful
  * `ssh2.c`: Variable `proxy_ssh_force_auth` in `ts-warp.ini` to force or negotiate SSH2 authenticaton methods
  * `ts-warp.c`: CLI options to disable `TS-Warp` port listeners: `-T 0:0`, `-S 0:0` and `-H 0:0`
  * `ssh2.c`: SSH-agent support added
  * `ts-warp.c`, `ssh2.c`: Fix SSH2 related memory leaks, proper SSH session closure
  * `inifile.c`: Hide SSH privatekey key passphrase in logs
  * `ssh2.c`: Authentication methods reworked. `proxy_key_passphrase` INI-entry appeared
  * `inifile.c`: Allow `proxy_key` to be specified in the INI-file
  * `build.sh`: MacOS application build fix for `LIBSSH2`
  * `ts-warp.c`, `Makefile`: `LIBSSH2` compilation issues under Linux
  * `inifile.c`: Enable default proxy port-numbers based on `proxy_type`: Socks4/5: 1080, HTTPS: 3128, SSH2: 22
  * `examples\ts-warp.ini`: Ports specified for `HTTPS` and `SSH2` proxies
  * `ts-warp.c`: Prevent `HTTP`-port overwriting `TRANSPARENT`-port
  * `build.sh` when makind `gui-warp.app` downloads/builds static SSL and SSH2 libraries `WITH_LIBSSH2=1 sh ./build.sh`
  * `SSH2` proxy supported in any position of a `CHAIN`
  * `gui-warp.app`: on macOS deiconify from Dock; `LICENSE` file added to the application
  * ~~`ts-warp.c`: Drop SSH2 proxy connection if it's in a mid of a `CHAIN`. `Single SSH2-proxy` and~~
    ~~`SSH2-proxy-as-a-target` of a `CHAIN` are supported~~
  * `http.c`: `http_client_request()` tolerate fail on `send()` and `recv()`
  * `ts-warp.c`: `NIT lookup` lifted up the code to work for all proxies
  * `ts-warp.c`: minor logging bugfix, `README.md` updates
  * SSH2 proxy-client introduction
  * `ts-warp.c`: Be silent on `LIBSSH2_ERROR_EAGAIN` in `libssh2_channel_read()`
  * `gui-warp.app`: `Makefile` and `build.sh` prepred for static `LIBSSH2`
  * SSH2 proxy first successful run
  * Added SSH2 to the main loop
  * `ssh2.c`: `libssh2_channel_direct_tcpip()` added
  * `ssh2.c`: `libssh2_userauth_*()` added
  * `gui-warp.app`: `build.sh` Stage `clean` removes correct `dmg` at right time

* **2024.01.16    ts-warp-1.4.8, gui-warp-1.0.22 (gui-warp-v1.0.27-mac), ns-warp-1.0.7**
  * `gui-warp.app`: update link points to `GUI-Warp.dmg` image
  * `gui-warp.app`: `build.sh` creates DMG image
  * `Makefile`: `ts-warp-ssh2` special target (for development) with `LIBSSH2`
  * `ssh2.c`: Started `ssh2_client_request()`
  * `Makefile`: Install manpages
  * `LIBSSH2` init/deinit
  * SSH2 proxy preparation
  * `ts-warp.c`: Cleaner code
  * `ts-warp.ini`: Examples are commented
  * `ts-warp.sh.in`: on `stop` flush only `TS-Warp` related anchors
  * Minor cosmetic changes

* **2023.12.22    ts-warp-1.4.7, gui-warp-1.0.22 (gui-warp-v1.0.26-mac), ns-warp-1.0.7**
  * `ts-warp.sh.in`: remove unused variables
  * `ts-warp.c`: Drop direct requests to `Transparent` port
  * `Makefile`: `examples-*` target user-name check fix
  * `ts-warp.sh.in`, `ts-warp_autofw.sh.in`: On Linux try Iptables firts, then Nftables; fix execution.
  * `socks.c`: Typo fix
  * `gui-warp.py`, `gui-warp.app`: Links to releases; better PID-file checking
  * `Makefile`, `README.md`: Better custom installation `PREFIX`
  * `gui/Makefile`: images go to `share/ts-warp` directory
  * `gui-warp.py`: Better parsing of `gui-warp.ini` - key error fix

* **2023.11.22    ts-warp-1.4.6, gui-warp-1.0.19 (gui-warp-v1.0.24-mac), ns-warp-1.0.7**
  * `gui-warp.app`: Move precompiled app out of sources
  * `examples/*.in`: Table names replaced `SOCKS` -> `TSWARP`
  * `Makefile`: `FreeBSD` `pkg` build as `root` creates config files for `nobody` user. Check and modify if needed.
  * FreeBSD port updated and moved out of sources
  * `pidlist.c`: `warn_unused_result` on GCC suppressed
  * `build.sh`: Setting current `python3` interpreter when building macOS app
  * `gui-warp`: some remediations for the macOS app on Sonoma
  * `gui-warp.py`: Fix FW-tab
  * `gui-warp`: Lost contents of the INI-tab return, thanks Luis Pontes <luis.pontes@kyndryl.com> for reporting
  * `gui-warp`: smoother LOG-file refresh, less CPU utilization
  * `gui-warp`: Better Unicode support
  * FreeBSD port added
  * `README.md`: minor updates
  * `gui-warp`: `About` tab, many little fixes
  * `gui-warp.app`: `sudo` fix
  * `xedec.c`: Hex hash conversion fix

* **2023.10.10    ts-warp-1.4.0, gui-warp-1.0.11 (gui-warp-v1.0.18-mac), ns-warp-1.0.7**
  * `ts-warp`: Incompatible to previous versions, CLI options for connections: `-T` Transparent, `-S` Socks, `-H` HTTP
  * `ts-warp`: Transparent connections port: `10800`, Internal Socks server port: `7080`, Internal HTTP server: `8080`
  * `http.c`: Internal HTTP proxy fixes
  * `http.c`: `http_server_request()` special buffer for replies
  * `gui-warp.app`: Create missed leafs of configuration directory tree if they are not exist
  * `socks.c`: Finish only client processes on Socks errors
  * `http.c`: `http_server_request()` reply the same protocol as in the request

* **2023.09.28    ts-warp-1.3.9, gui-warp-1.0.11 (gui-warp-1.0.17-mac), ns-warp-1.0.7**
  * `NS-Warp`: rolled-back to stable; fixed many errors; version bump; `README.md` update
  * MacOS GUI-WARP app: remove references to CLI configuration
  * ACT in `GUI-Warp` macOS port
  * `gui-warp.py`: ACT tab introduced; minor ACT pipe fixes
  * Timestamps added to ACT; option to unlink ACT pipe
  * ACT data requested via `SIGUSR2` available in `<PREFIX>/var/spool/ts-warp.act`
  * Deprecated CLI options removed: `-i`, `-P`
  * `USR2` signal to report active connections and traffic (ACT)
  * Traffic counters, minor decorative changes
  * `ts-warp.c`: Minor optimization
  * `ns-warp`: Build fix, `goto` removed
  * `inifile.c`: `chk_inivar()` less verbosity level
  * `gui-warp`: UI minor tweaks
  * `Makefile`: compiler -O3 optimization
  * `xedec.c`: `xdecrypt()` fix memory issue - NULL-terminated string
  * `gui-warp.py`: Strip extra newline char when saving INI-file

* **2023.08.18    ts-warp-1.3.2, gui-warp-1.0.8 (gui-warp-1.0.14-mac), ns-warp-1.0.4**
  * `Makefile`: Text formatting; `README.md` update
  * `gui\gui-warp.py`: Encode password dialog
  * `gui-warp.app`: Better configuration paths
  * `gui-warp.app`: Options input box
  * `ts-warp_autofw.sh`: ranges creation fix
  * `gui-warp.app`: `~/ts-warp.ini` restrict file permissions
  * `gui-warp.app`: `~/ts-warp_pf.conf` auto-generation, minor tweaks
  * `gui-warp.app`: `ts-warp.sh` script permissions and typo
  * `gui-warp.app`: Repack, build.sh script, better layout
  * `gui-warp.app`: macOS standalone all-in-one application: GUI-Warp + TS-Warp introduced
  * Various minor changes
  * `gui\gui-warp.py`: Password dialog as modal window. Thanks Sławomir Koper for discovering the menu click issue
  * `gui\gui-warp.py`: Don't ask password under root
  * `Makefile`: Set permissions on config files
  * `gui\gui-warp.py`: Password authentication dialog for `sudo`
  * `gui\gui-warp.py`: macOS Application with `py2app`. See build instuctions in [setup.py](gui\setup.py)
  * `README.md`: `Quick installation` section added

* **2023.06.29    ts-warp-1.3.0, gui-warp-1.0.3, ns-warp-1.0.4**
  * `http.c`: Correct HTTP proxy reply
  * `socks.c`, `ts-warp.c`:  `socks5_server_reply()` created for the innternal Socks5 server
  * `inifile.c`: `ini_look_server()` performs namelookup only if section has target_host or target_domain
  * `ts-warp_autofw.sh`: Include `target_hosts` and `proxy_servers` with port-addresses,
      thanks Gleb Reys <gleb@reys.net> for the bug-hunt
  * `Base64.c`: `base64_strdec()` created
  * HTTPS proxy authentication added
  * `Base64.c`: `base64_strenc()` created
  * `ts-warp.c`: HTTP proxy chains
  * `http.c`: `http_client_request()` created, method `CONNECT`
  * `proxy_*` replaced `socks_*` in examples and scripts
  * Unifying proxy definitions and various routines for Socks and HTTP protocols
  * Fix direct TCP connections
  * Internal HTTP the first try: no checks, no external proxy, no proxy-chains
  * Socks5 server related tweaks; `daddr` refactoring
  * `struct uvaddr` to combine `sockaddr_storage` and `char *` to replace `daddr` and `dname`
  * `http.c`, `http.h`: included; `http_server_request()` in progress
  * `ts-warp.c`: `process_socks()` removed due to unnecessary complexity
  * `ts-warp`: `-i IP:Port` will be deprecated in the future releases in favour of `-S IP:Port` and `-H IP:Port`
  * `ts-warp.c`: Make clients-to-ts-warp connections non-blocking
  * `ts-warp.c`: Socks processing moved into `process_socks()`
  * `network.c`: `str2inet()` correct `struct addrinfo` into `struct sockaddr_storage` mapping
  * The project renamed to "Transparent Socks Proxy and Traffic Wrapper" to match internal Socks server functionality

* **2023.05.16    ts-warp-1.2.0, gui-warp-1.0.3, ns-warp-1.0.4**
  * Internal Socks5 server example in `ts-warp.ini`; [README.md](README.md) update
  * `ts-warp.sh`: `pkill -x` for correct `restart`
  * `-P` flag to disable internal Socks5 server
  * `socks.c`: `socks5_server_request()` fix `SA_FAMILY()` for `AF_INET` and `AF_INET6`; Trimming redundant spaces
  * Basic Socks5 server-side functions are implemented; `socks5_atype()` removed; Socks related functions refactored
  * `ns\ns-warp.c` Outgoing socket fix
  * `socks.c`: `socks5_server_request()` added; `inifile.c`: minor fixes
  * `socks.c`: `socks5_serve_hello()` added
  * `inifile.c`, `inifile.h`: `ini_look_server()` supports hostnames
  * `Makefile`: detect configuration targets are completed
  * `Makefile`: prevent `examples-general` and `examples-special` from running as `root`
  * `struct sockaddr` -> `struct sockaddr_storage` to handle IPv6 correctly
  * `Makefile`: create `etc` on `install-configs`/`install-examples` stage
  * `ts-warp_autofw.sh` block using as `root`; Added to `make all`/`make clean`

* **2023.04.22    ts-warp-1.1.7, gui-warp-1.0.2, ns-warp-1.0.3**
  * `ts-warp_autofw.sh` makes and prints out sample firewall configuration based on `ts-warp.ini` contents
  * Switch back default to special firewall configuration
  * Starting loglevel changed to `LOG_WARN`; Minor changes and `README.md` update
  * Simplify firewall configuration. `make install`/`make install-configs` installs **general** simplified firewall
    example- and configuration- files to forward all TCP traffic via TS-Warp. See [README.md](README.md) for details
    and examples of more complex **special** firewall configuration files.
  * `inifile.c`: `show_ini()`: captions instead of IDs for targets and balansing
  * Updated `-h` option
  * `network.c` Reduce timeout for a new not yet established connections
  * `ts-warp.c` faster `failover` switch. Details in [#6](https://github.com/mezantrop/ts-warp/issues/6)
  * `ts-warp.sh stop` ensures the daemon is killed
  * Updated `README.md` files

* **2023.03.06    ts-warp-1.1.6, gui-warp-1.0.2, ns-warp-1.0.3**
  * Release preparation
  * `ns-warp.sh`: Startup script; `README.md` update; Minor changes
  * `ns-warp`: Run as different user
  * `ts-warp.sh`: Check if a process from the pid-file really exists
  * [Manpages formatting](https://github.com/mezantrop/ts-warp/issues/5)
  * `Makefile`: Multiple minor changes
  * `README.md`: `gui-warp.py` with `sudo`; a new screenshot
  * `gui-warp.py`: Minor updates
  * `gui-warp.py`: Many small changes: gui-warp-1.0.3

* **2023.01.23    ts-warp-1.1.5, gui-warp-1.0.2, ns-warp-1.0.2**
  * `gui-warp.py`: The most of the widgets changed from `tk` to `ttk` for better theme (light/dark) support
  * `gui-warp.py`: Correct `Save changes` Button references on `INI` and `FW` tabs
  * `gui-warp.py`: `App()` class `fwfile` argument to override default `ts-warp_pf.conf` location, thanks Gema Robles
  * `ts-warp.sh.in`: Do not replace original PF-rules
  * `network.c`: `inet2str()` Returns `IP:PORT`
  * `inifile.c`: Many minor logfile tweaks
  * `ts-warp.c`: `read_ini()` added more allowed characters in sections: `a-zA-Z0-9_\t -+()`
  * `inifile.c`, `pidlist.c`: `show_ini()`, `pidlist_show()` logs using `LOG_CRIT`
  * `socks.c`, `socks.h`: Extended Socks status codes
  * `ns-warp.c`: Cosmetic chanes
  * `ns-warp`:  `dns.c/forward_ip()` rewritten for speed; Many fixes and improvements
  * ~~`ns-warp.c`: `fork()` to reduce possible resolve timeout~~

* **2022.12.23    ts-warp-1.1.4, gui-warp-1.0, ns-warp-1.0.2**
  * `ns-warp`: memory usage fix
  * `ts-warp.c`: TCP nodelay enabled
  * `network.c`, `str2inet()`: Return INADDR_NONE if resolve fails
  * `mk_pidfile()`: fix for NULL pwd structure
  * `ns-warp`: CIDR addresses for NIT-pools
  * `ts-warp.c`: Correct clients exit procedure
  * `ts-warp.c`: Use empty Socks section name in the PID list for direct connections
  * On Linux `nftables` rules and usage examples added

* **2022.11.26    ts-warp-1.1.3, gui-warp-1.0, ns-warp-1.0.1**
  * Socks servers section Failover/Roundrobin/None modes enabled
  * `README.md`, `examples\ts-warp.ini`: updated
  * Minor decorative formatting
  * `ts-warp.c`: Accepts `SIGUSR1` to show configuration and clients
  * `pidlist.c`, `pidlist.h`: Clients processes monitoring
  * `inifile.c`: `target_network` IPv4 addresses in CIDR notation
  * `ts-warp.sh`: `pkill` utilized in `stop -f` routine
  * Proper PID logging
  * `ts-warp.sh stop`: no redundant messages when autodetecting PID

* **2022.11.18    ts-warp-1.1.2, gui-warp-1.0, ns-warp-1.0.1**
  * Workload balance crashes ts-warp under high workload. The option is disabled now.
  * `examples\ts-warp.ini`: updated for section Failover mode
  * `ts-warp.c`: Return configuration reload on SIGHUP

* **2022.11.13    ts-warp-1.1.1, gui-warp-1.0, ns-warp-1.0.1**
  * `examples\ts-warp.ini`: Failover/Roundrobin/None modes
  * Minor text/typos fixes

* **2022.11.13    ts-warp-1.1.0, gui-warp-1.0, ns-warp-1.0.1**
  * Socks servers section Failover/Roundrobin/None modes
  * Minor description changes
  * Finally section names allowed characters are: a-zA-Z0-9_\t -
  * Client's processes tracking - for future usage
  * `inifile.c`: String filter for non-ASCII, control characters
  * `ts-warp.c`: Offer `AUTH_METHOD_UNAME` only if the section contains `socks_user` value

* **2022.10.31    ts-warp-1.0.15, gui-warp-1.0, ns-warp-1.0.1**
  * `natlook.c`, `natlook.h`: Unified *BSD data structures
  * `ns-warp`: `str2inet():`: updated; `malloc` issues fixed
  * `network.c/str2inet():` simplified; `inifile.c` memory optimization; `CHANGELOG.md` minor changes
  * `TCP_KEEPALIVE`: with defaults for OpenBSD
  * `socks.c`, `socks.h`: Drop connection (not exit) if a Socks server replies an error
  * `TCP_KEEPALIVE` enabled and respective options added where possible

* **2022.10.19    ts-warp-1.0.14, gui-warp-1.0, ns-warp-1.0.0**
  * The first Github Release
  * `examples/ts-warp_pf*.conf`: PF-rules optimization, thanks Marcin Biczan
  * `TCP_NODELAY`: enabled for macOS
  * `Makefile`: `make release` target to build releases
  * `version.sh`: Compatibility with gawk

* **2022.10.17    ts-warp-1.0.12, gui-warp-1.0, ns-warp-1.0.0**
  * `version.h`, `version.sh`: Auto-increment build number for the release or skip for normal build
  * `inifile.c`: C99 compatibility, thanks [@alitalaghat](https://github.com/alitalaghat)
  * `inifile.c`: `free()` INI-entries on `reload` initiated by `SIGHUP`
  * `WITH_TCP_NODELAY` option to speedup connections
  * `ns-warp.c` `usage()` update

* **2022.10.01    ts-warp-1.0.11, gui-warp-1.0, ns-warp-1.0.0**
  * `NS-Warp` published
  * `inifile.c`: create_chains(): Chain list traversal fix; Thanks Bart Couvreur for the bug-hunting
  * `inifile.c`: `socks5_atype()`: Correct Socks5 address type selection
  * `inifile.c`: `socks5_atype()`: Socks5 Address type selector: IPv4/IPv6/Name
  * `socks5_request()`: `SOCKS5_ATYPE_NAME` added; `inifile.h`: NS_INI_ENTRY_NIT_POOL
  * `Makefile`: make uninstal
  * Minor decorative changes
  * `ts-warp.sh`: stop -f for macOS update
  * `ts-warp.sh`: Force mode for start/stop procedures rewritten
  * `examples/ts-warp_pf_macos.conf`: `en7` added as often used
  * `inifile.c`: `free(entry.val)` if unused
  * `inifile.c`: Skip variables not in sections
  * Chains & Socks4 rewritten

* **2022.08.08    ts-warp-1.0.10, gui-warp-1.0**
  * `ts-warp.c`: Socks Chains improvement
  * `inifile.c`: Lowcase in section names and better domain names parsing
  * `utility.c`: `toint()` wrapping
  * `usage()`: moved from `utility.c` to `ts-warp.c`
  * `utility.h`: Cleanup

* **2022.08.06    ts-warp-1.0.9, gui-warp-1.0**
  * `logfile.c`: Logging moved out of `utility.c`
  * `README.md`: TODO: UDP redirection; Resolve remote names via Socks
  * `inifile.c`: `ini_look_server()`: `host[0] = 0` prevents garbage output if `getnameinfo()` fails. Thanks Juha Nurmela
  * `README.md` updated
  * `man\ts-warp.8`: updated
  * `man\ts-warp.5`: draft created
  * `README.md`: typos fixed
  * 120-chars width formatting
  * `ts-warp.c`: Log to STDOUT if TS-Warp is started in foreground: no `-d` option and no log-file `-l` is specified
  * `README.md`: updated

* **2022.06.11    ts-warp-1.0.8, gui-warp-1.0**
  * Better logging
  * Socks4 added
  * `socks.c/socks.h`: `SOCKS5_ATYPE_*_LEN` for precise address-type matching
  * `utility.c`: mesg[256] -> mesg[STR_SIZE]
  * `iniline.c`: Set smaller buffers in `ini_look_server()`
  * `ts-warp.sh.in`: Force-start option '-f' removed from the default config
  * `network.c`: fixed `memset()` buffer overflow in `inet2str()`
  * `ts-warp.c`: fixed `printf()` -> `fprintf()` typo in `,main()` on mac
  * Close `/dev/pf` on exit
  * Github CodeQL vulnerability scan integration workflow
  * Documentation: manpages creation started under `man` directory

* **2022.06.02    ts-warp-1.0.7, gui-warp-1.0**
  * `README.md` update
  * Minor issues fixed:
    * fixed unused variable warning on macOS for the `-u` switch
    * correced ts_pass program name in the help
  * README.md update
  * CLI option: `-h` reworked to show defaults
  * CLI option: `-u` - specify a `user` to run `ts-warp`

* **2022.05.23    ts-warp-1.0.6, gui-warp-1.0**
  * `setuid()` to lower privileges for the daemon runtime to the level of the user `nobody`. Still `root` privileges are
    required to start the daemon. Note! On macOS `ts-warp` always runs under `root` user, because of the OS "features".
  * `natlook.c`: `pf_open()`, `pf_close` to optimize `nat_lookup()`
  * `README.md`: better wording
  * `inifile.c`: `str2inet()` buffer usage fix

* **2022.05.17    ts-warp-1.0.5, gui-warp-1.0**
  * Logging enhanced
  * `ts-warp.sh`: `uname -o` -> `uname -s` for macOS compatibility
  * `ts-warp.sh`: custom CLI options
  * `ts-warp.sh`: updated for Linux
  * Examples updated for OpenBSD
  * OpenBSD compatibility and examples
  * Minor decorative changes
  * `utils` -> `utility` continue

* **2022.04.11    ts-warp-1.0.4, gui-warp-1.0**
  * Code restructurization:
    * `utils` module renamed into `utility`
    * Network routines moved from `utility` and `socks` modules into `network`
    * Various minor cleanups and refactoring
  * Correct IPv6 address comparison
  * Enable ts-warp daemon acting as a Socks-server when NAT/redirection enabled
  * Manage non-NAT/redirected connnections with ts-warp daemon, so it's possible to specify ts-warp daemon as
    a Socks-server in userland. Don't forget to add the respective section with a real Socks server in `ts-warp.ini` or
    uncomment the `[DEFAULT]` one to catch all the unhandled requests.
  * Skip Domain check in `ini_look_server()` if target IP doesn't resolve

* **2022.03.29    ts-warp-1.0.3, gui-warp-1.0**
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

* **2022.03.15    ts-warp-1.0.2**
  * `-i` option replaces `-I` in CLI arguments
  * Full Linux with iptable code integration
  * PID-file option added to CLI parameters
  * Support for domain and hostnames in target definitions of `ts-warp.ini`
  * `getnameinfo()` fix to work correctly on Linux
  * To prevent сonfiguration files and scripts from being overwritten,
    `make install` copies them into `<PREFIX>/etc` with `.sample` postfix

* **2022.03.11    ts-warp-1.0.1**
  * make install
  * `-I IP:Port` replaces `-I IP -i Port`; minor optimizations
  * INI-file parser to ignore variables without values
  * Fixed processed empty/nonexisting `socks_server` values in the INI-file
  * Remove processed `chain_list` elements in `create_chains()`
  * Better messaging in `ts-warp.sh` start/stop script
  * `natlook()` dest port printout on Mac, fixed; thanks Alicja Michalska <alka96@protonmail.com> for testing

* **2022.03.05    ts-warp-1.0.0**
  * Starting point to track versions
