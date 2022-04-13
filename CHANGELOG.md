# CHANGELOG

* Current
  * Minor decorative changes
  * `utils` -> `utility` continue

* 2022.04.11    ts-warp-1.0.4, gui-warp-1.0
  * Code restructurization:
    * `utils` module renamed into `utility`
    * Network routines moved from `utility` and `socks` modules into `network`
    * Various minor cleanups and refactoring
  * Correct IPv6 address comparison
  * ts-warp daemon as a SOCKS-server when NAT/redirection enabled
  * Manage non-NAT/redirected connnections with ts-warp daemon, so it's possible
    to specify ts-warp daemon as a SOCKS-server in userland. Don't forget to
    add the respective section with a real SOCKS server in `ts-warp.ini` or
    uncomment the `[DEFAULT]` one to catch all the unhandled requests.
  * Skip Domain check in ini_look_server() if target IP doesn't resolve

* 2022.03.29    ts-warp-1.0.3, gui-warp-1.0
  * Examples updated
  * `gui-warp.py` v1.0 release with installation via `make install`
  * `gui-warp.py` INI-, LOG-, FW- tabs with save/control buttons
  * `gui-warp.py` configuration tab started
  * `gui\gui-warp.py`: ts-warp daemon status, log-file listing, buttons work
  * `gui\ts-warp.py` added. Note: the most of the buttons/functions are unusable
  * `ts-warp.sh`: stop() waits until the PID-file is removed
  * Stay calm and don't exit() daemon on the most of client issues
  * Reject clients with no real-destination addresses
  * Documentation/examples update

* 2022.03.15    ts-warp-1.0.2
  * -i option replaces -I in CLI arguments
  * Full Linux with iptable code integration
  * PID-file option added to CLI parameters
  * Support for domain and hostnames in target definitions of `ts-warp.ini`
  * getnameinfo() fix to work correctly on Linux
  * To prevent сonfiguration files and scripts from being overwritten,
    `make install` copies them into `<PREFIX>/etc` with `.sample` postfix

* 2022.03.11    ts-warp-1.0.1
  * make install
  * `-I IP:Port` replaces `-I IP -i Port`; minor optimizations
  * INI-file parser to ignore variables without values
  * Fixed processed empty/nonexisting `socks_server` values in the INI file
  * Remove processed `chain_list` elements in `create_chains()`
  * Better messaging in `ts-warp.sh` start/stop script
  * `natlook()` dest port printout on Mac, fixed; thanks Alicja Michalska
    <alka96@protonmail.com> for testing

* 2022.03.05    ts-warp-1.0.0
  * Starting point to track versions
