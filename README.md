# TS-Warp

## Transparent SOCKS protocol Wrapper

### Goals and TODO list

- [x] Create a soxifier service - transparent firewall-based redirector of
TCP/IP connections to a SOCKS-proxy server

- Support platforms:
  - [x] macOS, FreeBSD and OpenBSD with PF
  - [x] Linux with Iptables

- [x] IPv6 stack support
- [x] Maintain simple configuraion structure as INI-file
- [x] Support basic SOCKS authentication methods
- [x] Password encoding (obfuscation) in configuration files
- [x] SOCKS proxy chains
- [x] Daemon mode
- [x] Front-end UI
- [x] Installation script
- [ ] Documentation

### Changelog

See it [here](CHANGELOG.md)

### Installation

- Run `# make install` to install files under `PREFIX=/usr/local` base or `# make install PREFIX=/path/to/install`, to use a custom `PREFIX` directory
- Create `<PREFIX>/etc/ts-warp.ini` based on `<PREFIX>/etc/ts-warp.ini.sample` file to suite your needs
- *Optional*. Edit `<PREFIX>/etc/ts-warp.sh` to customize PID-, LOG- and INI- files location
  
- **On macOS and \*BSD**:
  Create `<PREFIX>/etc/ts-warp_pf.conf` based on appropriate `<PREFIX>/etc/ts-warp_pf_*.conf.sample` to configure the packet filter

- **On Linux**:
  - Create `<PREFIX>/etc/ts-warp_iptables.sh` based on `<PREFIX>/etc/ts-warp_iptables.sh.sample` to configure firewall

### Usage

Under root privileges start, control or get status of ts-warp:

`# <PREFIX>/etc/ts-warp.sh start|stop|restart|reload|status`

ts-warp understands `SIGHUP` signal as command to reload configuration and `SIGINT` to stop the daemon.

Use `ts-pass` to encode passwords if requred. See examples in [ts-warp.ini](examples/ts-warp.ini)

### GUI front-end

Experimental GUI front-end application to control ts-warp daemon can be installed from the `gui` directory:
`cd gui`

`make install` it into `PREFIX=/usr/local` or `make install PREFIX=/path/to/install`
to copy files under the different base directory

To start the GUI run:
`# /<PREFIX>/bin/gui-warp.py`

Note, Python 3 interpreter with `tkinter` support is required to run the GUI frontend.

![gui-warp.py](gui/gui-warp_py.png)

### Contacts

Not so early stage of development, yet don't expect everything to work properly.
If you have an idea, a question, or have found a problem, do not hesitate to
open an [issue](https://github.com/mezantrop/ts-warp/issues/new/choose) or write
me directly: Mikhail Zakharov <zmey20000@yahoo.com>

Many thanks to [contributors](CONTRIBUTORS.md) of the project
