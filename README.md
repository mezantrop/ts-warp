# TS-Warp

## Transparent SOCKS protocol Wrapper

**Warning!** Early stage of development, don't expect everything to work properly

See the [Changelog](CHANGELOG.md)

### Goals and TODO list

- [x] Create a transparent firewall-based redirector of TCP/IP connections to
a SOCKS-proxy server

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
- [x] Installation script
- [ ] Documentation

### Installation

- `make install` or `make install PREFIX=/path/to/install`. Default is `PREFIX=/usr/local`

- Edit `<PREFIX>/etc/ts-warp.ini` file to suite your needs
  
- *On macOS and \*BSD*:
  - Edit to configure packet filter `<PREFIX>/etc/ts-warp_pf.conf`

- *On Linux*:
  - Edit to configure firewall `<PREFIX>/etc/ts-warp_iptables.sh`

### Usage

*On macOS and \*BSD*:

- Start, control or get status of ts-warp: `# <PREFIX>/etc/ts-warp.sh start|stop|restart|reload|status`

*On Linux*:

- Enable packet redirection: `$ sudo <PREFIX>/etc/ts-warp_iptables.sh`
- Start ts-warp daemon: `$ sudo <PREFIX>/bin/ts-warp -d`

ts-warp understands `SIGHUP` signal as command to reload configuration and `SIGINT` to stop the daemon.

Use `ts-pass` to encode passwords if requred. See examples in [ts-warp.ini](examples/ts-warp.ini)

### Contacts

You are very welcome to email me: Mikhail Zakharov <zmey20000@yahoo.com>

Many thanks to [contributors](CONTRIBUTORS.md) of the project
