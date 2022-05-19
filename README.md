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

- Just run:
  
  ```sh
  # make install
  ```
  
  This will install all the program files under the `/usr/local` tree. If a
  different installation path is required, set a `PREFIX`:
  
  ```sh
  # make install PREFIX=/path/to/install
  ```

- Create `<PREFIX>/etc/ts-warp.ini` based on `<PREFIX>/etc/ts-warp.ini.sample`
file to suite your needs
- *Optional*. Edit `<PREFIX>/etc/ts-warp.sh` to customize PID-, LOG- and INI-
files location
- *On macOS and \*BSD*. Create `<PREFIX>/etc/ts-warp_pf.conf` based on appropriate
`<PREFIX>/etc/ts-warp_pf_*.conf.sample` to configure the packet filter
- *On Linux*. Create `<PREFIX>/etc/ts-warp_iptables.sh` based on
`<PREFIX>/etc/ts-warp_iptables.sh.sample` to configure firewall

### Usage

Under root privileges start, control or get status of ts-warp:

```sh
# <PREFIX>/etc/ts-warp.sh start|restart [options]
# <PREFIX>/etc/ts-warp.sh stop|reload|status
```

All the ts-warp command-line options can be listed using `ts-warp -h`.
For example, to temporary enable more verbose logs, restart ts-warp with
`-v 4` option:

```sh
# <PREFIX>/etc/ts-warp.sh restart -v 4
```

ts-warp understands `SIGHUP` signal as command to reload configuration and
`SIGINT` to stop the daemon.

Use `ts-pass` to encode passwords if requred. See examples in [ts-warp.ini](examples/ts-warp.ini)

### GUI front-end

![gui-warp.py](gui/gui-warp_py.png)

- Experimental GUI front-end application to control ts-warp daemon can be installed
from the `gui` directory:

  ```sh
  # cd gui
  # make install
  ```

  *Optionally*. Set `PREFIX`, to use a different installation target in the
  `make` command above:

  ``` sh
  # make install PREFIX=/path/to/install
  ```

- To start the GUI run:
  `# /<PREFIX>/bin/gui-warp.py`

Note, Python 3 interpreter with `tkinter` support is required to run the GUI frontend.

### Contacts

Not so early stage of development, yet don't expect everything to work properly.
If you have an idea, a question, or have found a problem, do not hesitate to
open an [issue](https://github.com/mezantrop/ts-warp/issues/new/choose) or mail
me: Mikhail Zakharov <zmey20000@yahoo.com>

Many thanks to [contributors](CONTRIBUTORS.md) of the project
