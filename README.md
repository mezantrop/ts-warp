# TS-Warp

## Transparent SOCKS protocol Wrapper

**Warning!** Early stage of development, don't expect everything to work properly

See the [Changelog](CHANGELOG.md)

### Goals and TODO list

- [x] Create a transparent firewall-based redirector of TCP/IP connections to
a SOCKS server

- Support platforms:
  - [x] macOS, FreeBSD with PF, OpenBSD (not tested)
  - [x] Linux

- [x] IPv6 stack support
- [x] Maintain simple configuraion structure as INI-file
- [x] Support basic SOCKS authentication methods
- [x] Password encoding (obfuscation) in configuration files
- [x] SOCKS proxy chains
- [x] Daemon mode
- [ ] Front-end UI
- [ ] Installation script

### Compilation

Regular version: `$ make`

If required, debug binary can also be compiled: `$ make debug`

### Installation

Automatic installation script (`make install`) is still to be done, you have to place and edit files yourself:

- `$ cp ts-warp /usr/local/bin`
- `$ cp ts-warp.sh /usr/local/etc`
- Create `/usr/local/etc/ts-warp.ini` file using [ts-warp.ini](examples/ts-warp.ini) as a template
  
- *On macOS and \*BSD*:
  - Create `/usr/local/etc/ts-warp_pf.conf` using [ts-warp_pf.conf](examples/ts-warp_pf.conf) as an example
  - Enable packet redirection and start ts-warp daemon as root: `# /usr/local/etc/ts-warp.sh start`

- *On Linux*:
  - Create `/usr/local/etc/ts-warp_iptables.sh` using [ts-warp_iptables.sh](examples/ts-warp_iptables.sh)
  - Enable packet redirection: `$ sudo /usr/local/etc/ts-warp_iptables.sh`
  - Start ts-warp daemon: `$ sudo ts-warp -d`

### Usage

*On macOS and \*BSD*:

- Enable packet redirection and start ts-warp daemon: `# /usr/local/etc/ts-warp.sh start`

*On Linux*:

- Set packet redirection: `$ sudo /usr/local/etc/ts-warp_iptables.sh`
- Start ts-warp daemon: `$ sudo ts-warp -d`

Use `ts-pass` to encode passwords if requred. See examples in [ts-warp.ini](examples/ts-warp.ini)

### Contacts

You are very welcome to email me: Mikhail Zakharov <zmey20000@yahoo.com>

Many thanks to [contributors](CONTRIBUTORS.md) of the project
