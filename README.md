# TS-Warp

## Transparent SOCKS protocol Wrapper

**Warning!** Early stage of development, don't expect everything to work properly

### Goals and TODO list

- [x] Create a transparent firewall-based redirector of TCP/IP connections to
a SOCKS server

- Support platforms:
  - [x] macOS
  - [ ] OpenBSD, FreeBSD with PF
  - [x] Linux optionally

- [x] IPv6 stack support
- [x] Maintain simple configuraion structure as INI-file
- [x] Support basic SOCKS authentication methods
- [x] Password encoding (obfuscation) in configuration files
- [ ] SOCKS proxy chains
- [x] Daemon mode
- [ ] Front-end UI
- [ ] Installation script

### Compilation

Regular version: `$ make`

If required, debug binary can also be compiled: `$ make debug`

### Installation

Automatic installation script is still to be done, you have to place and edit files yourself:

- `$ cp ts-warp /usr/local/bin`
- `$ cp ts-warp.sh /usr/local/etc`
- Create `/usr/local/etc/ts-warp.ini` file using [ts-warp.ini](examples/ts-warp.ini) as an example
  
- *On macOS*:
  - Create `/usr/local/etc/ts-warp_pf.conf` using [ts-warp_pf.conf](examples/ts-warp_pf.conf) as an example
  - Enable packet redirection and start ts-warp daemon: `$ sudo /usr/local/etc/ts-warp.sh start`

- *On Linux*:
  - Create `/usr/local/etc/ts-warp_iptables.sh` using [ts-warp_iptables.sh](examples/ts-warp_iptables.sh) as an example
  - Enable packet redirection: `$ sudo /usr/local/etc/ts-warp_iptables.sh`
  - Start ts-warp daemon: `$ ts-warp -d`

### Usage

*On macOS*:

- Enable packet redirection and start ts-warp daemon: `$ sudo /usr/local/etc/ts-warp.sh start`

*On Linux*:

- Set packet redirection to localhost port 10800: `$ sudo /usr/local/etc/ts-warp_iptables.sh`
- Start ts-warp daemon: `$ ts-warp -d`

### Contacts

Do not hesitate to email me: Mikhail Zakharov <zmey20000@yahoo.com>
