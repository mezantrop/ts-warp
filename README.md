# TS-Warp

[![CodeQL](https://github.com/mezantrop/ts-warp/actions/workflows/codeql.yml/badge.svg)](https://github.com/mezantrop/ts-warp/actions/workflows/codeql.yml)

<a href="https://www.buymeacoffee.com/mezantrop" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/default-orange.png" alt="Buy Me A Coffee" height="41" width="174"></a>

## Transparent SOCKS protocol Wrapper

### Goals and TODO list

- [x] Create a soxifier service - transparent firewall-based redirector of
TCP/IP connections to a SOCKS-proxy server

- Support platforms:
  - [x] macOS, FreeBSD and OpenBSD with PF
  - [x] Linux with Iptables

- [x] IPv6 stack support
- [x] Maintain simple configuration structure as INI-file
- [x] Support basic SOCKS authentication methods
- [x] Password encoding (obfuscation) in configuration files
- [x] SOCKS proxy chains
- [x] Daemon mode
- [x] Front-end UI
- [x] Installation script
- [ ] Documentation
- [ ] UDP support
- [ ] Resolve remote names via SOCKS

### Changelog

See it [here](CHANGELOG.md)

### Installation

- [Download](https://github.com/mezantrop/ts-warp/archive/refs/heads/master.zip) TS-Warp sources and unarchive them,
or just clone the repository running `git` in a terminal:
  
  ```sh
  $ git clone https://github.com/mezantrop/ts-warp
  ```

Typically the installation operations require root privileges, below we use `sudo` to achieve the goal, but on some
operating systems you may need to invoke `su` instead.

- Using terminal, in the directory with TS-Warp source code run:
  
  ```sh
  # sudo make install
  ```
  
  This will install all the files under the `/usr/local` tree. If a different installation path is required, set `PREFIX`:
  
  ```sh
  # sudo make install PREFIX=/path/to/install
  ```

- Create `<PREFIX>/etc/ts-warp.ini` based on `<PREFIX>/etc/ts-warp.ini.sample` file to suite your configuration. For example:

  ```sh
  # sudo cp /usr/local/etc/ts-warp.ini.sample /usr/local/etc/ts-warp.ini
  # sudo nano /usr/local/etc/ts-warp.ini
  ```
  
- *Optional*. Edit `<PREFIX>/etc/ts-warp.sh` to customize PID-, LOG- and INI-files location. For example:

  ```sh
  # sudo nano /usr/local/etc/ts-warp.sh
  ```

- **On macOS and \*BSD**. Create `<PREFIX>/etc/ts-warp_pf.conf` based on appropriate `<PREFIX>/etc/ts-warp_pf_*.conf.sample`
to configure the packet filter. For example:

  ```sh
  # sudo cp /usr/local/etc/ts-warp_pf.conf.sample /usr/local/etc/ts-warp_pf.conf
  # sudo nano /usr/local/etc/ts-warp_pf.conf
  ```

- **On Linux**. Create `<PREFIX>/etc/ts-warp_iptables.sh` based on `<PREFIX>/etc/ts-warp_iptables.sh.sample`
to configure firewall. For example:

  ```sh
  # sudo cp /usr/local/etc/ts-warp_iptables.sh.sample /usr/local/etc/ts-warp_iptables.sh
  # sudo nano /usr/local/etc/ts-warp_iptables.sh
  ```

### Usage

Under root privileges start, control or get status of ts-warp daemon:

```sh
# <PREFIX>/etc/ts-warp.sh start|restart [options]
# <PREFIX>/etc/ts-warp.sh stop|reload|status
```

All the ts-warp command-line options can be listed using `$ ts-warp -h`:

```sh
Usage:
  ts-warp -i IP:Port -c file.ini -l file.log -v 0-4 -d -p file.pid -f -u user -h

Version:
  TS-Warp-1.0.X

All parameters are optional:
  -i IP:Port        Incoming local IP address and port
  -c file.ini       Configuration file, default: /usr/local/etc/ts-warp.ini

  -l file.log       Log filename, default: /usr/local/var/log/ts-warp.log
  -v 0..4           Log verbosity level: 0 - off, default 3

  -d                Daemon mode
  -p file.pid       PID filename, default: /usr/local/var/run/ts-warp.pid
  -f                Force start

  -u user           User to run ts-warp from, default: nobody

  -h                This message

```

For example, to temporary enable more verbose logs, restart ts-warp with `-v 4` option:

```sh
# sudo /usr/local/etc/ts-warp.sh restart -v 4
```

ts-warp understands `SIGHUP` signal as the command to reload configuration and `SIGINT` to stop the daemon.

Use `ts-pass` to encode passwords if requred. See examples in [ts-warp.ini](examples/ts-warp.ini)

### GUI front-end

![gui-warp.py](gui/gui-warp_py.png)

Experimental GUI front-end application to control ts-warp daemon can be installed from the `gui` directory:

```sh
# cd gui
# sudo make install
```

*Optionally*. Set `PREFIX`, to use a different installation target in the `make` command above:

``` sh
# sudo make install PREFIX=/path/to/install
```

To start the GUI run:

``` sh
# sudo <PREFIX>/bin/gui-warp.py &
```

Note, Python 3 interpreter with `tkinter` support is required to run the GUI frontend.

### Contacts

Not so early stage of development, yet don't expect everything to work properly. If you have an idea, a question,
or have found a problem, do not hesitate to open an [issue](https://github.com/mezantrop/ts-warp/issues/new/choose)
or mail me: Mikhail Zakharov <zmey20000@yahoo.com>

Many thanks to [contributors](CONTRIBUTORS.md) of the project
