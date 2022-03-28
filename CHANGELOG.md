# CHANGELOG

* Current
  * `gui-warp.py` INI-, LOG-, FW- tabs with save/control buttons
  * `gui-warp.py` configuration tab started
  * `gui\gui-warp.py`: ts-warp daemon status, log-file listing, buttons work
  * `gui\ts-warp.py` added. Note: the most of the buttons/functions are unusable
  * `ts-warp.sh`: stop() waits until the PID-file is removed
  * Stay calm and don't exit() daemon on the most of client issues
  * Reject clients with no real-destination addresses
  * Documentation/examples update

* 2022.03.15    v1.0.2
  * -i option replaces -I in CLI arguments
  * Full Linux with iptable code integration
  * PID-file option added to CLI parameters
  * Support for domain and hostnames in target definitions of `ts-warp.ini`
  * getnameinfo() fix to work correctly on Linux
  * To prevent сonfiguration files and scripts from being overwritten,
    `make install` copies them into `<PREFIX>/etc` with `.sample` postfix

* 2022.03.11    v1.0.1
  * make install
  * `-I IP:Port` replaces `-I IP -i Port`; minor optimizations
  * INI-file parser to ignore variables without values
  * Fixed processed empty/nonexisting `socks_server` values in the INI file
  * Remove processed `chain_list` elements in `create_chains()`
  * Better messaging in `ts-warp.sh` start/stop script
  * `natlook()` dest port printout on Mac, fixed; thanks Alicja Michalska <alka96@protonmail.com> for testing

* 2022.03.05    v1.0.0
  * Starting point to track versions
