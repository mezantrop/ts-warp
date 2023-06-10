# TS-Warp

[![CodeQL](https://github.com/mezantrop/ts-warp/actions/workflows/codeql.yml/badge.svg)](https://github.com/mezantrop/ts-warp/actions/workflows/codeql.yml)
[![C/C++ CI - macOS](https://github.com/mezantrop/ts-warp/actions/workflows/c-cpp-macos.yml/badge.svg)](https://github.com/mezantrop/ts-warp/actions/workflows/c-cpp-macos.yml)
[![C/C++ CI - Ubuntu](https://github.com/mezantrop/ts-warp/actions/workflows/c-cpp-ubuntu.yml/badge.svg)](https://github.com/mezantrop/ts-warp/actions/workflows/c-cpp-ubuntu.yml)

## Transparent proxy server and traffic wrapper

<a href="https://www.buymeacoffee.com/mezantrop" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/default-orange.png" alt="Buy Me A Coffee" height="41" width="174"></a>

### Features

- Proxy services with TCP-traffic redirection to external Socks4/5 and HTTPS proxy servers
  - Transparent firewall-based traffic redirector
  - Internal Socks and HTTPS proxy server

- Supported platforms:
  - macOS, FreeBSD and OpenBSD with `PF`
  - Linux with `nftables` or `iptables`

- Main features
  | Transparent proxy                        | Socks5               | Socks4               | HTTPS                |
  | ---------------------------------------- | -------------------- | -------------------- | -------------------- |
  | Proxy protocol                           | :white_check_mark:   | :white_check_mark:   | :white_check_mark:   |
  | Proxy chains                             | :white_check_mark:   | :white_check_mark:   | :white_check_mark:   |
  | Proxy workload balancer                  | :white_check_mark:   | :white_check_mark:   | :white_check_mark:   |
  | Basic authentication (username/password) | :white_check_mark:   | :white_large_square: | :white_large_square: |
  | IPv6 stack support                       | :white_check_mark:   | :white_large_square: | :white_check_mark:   |
  | Remote names resolution: [NS-Warp](ns)   | :white_check_mark:   | :white_large_square: | :white_check_mark:   |

  | Internal proxy                           | Socks5               | Socks4               | HTTPS                |
  | ---------------------------------------- | -------------------- | -------------------- | -------------------- |
  | Proxy protocol                           | :white_check_mark:   | :white_large_square: | :white_check_mark:   |
  | Proxy chains                             | :white_check_mark:   | :white_check_mark:   | :white_check_mark:   |
  | Proxy workload balancer                  | :white_check_mark:   | :white_check_mark:   | :white_check_mark:   |
  | Basic authentication (username/password) | :white_large_square: | :white_large_square: | :white_large_square: |
  | IPv6 stack support                       | :white_check_mark:   | :white_large_square: | :white_check_mark:   |
  | Remote names resolution                  | :white_check_mark:   | :white_large_square: | :white_check_mark:   |

- Miscellaneous features
  - [x] Simple configuration structure as INI-like file
  - [x] Password encoding (obfuscation) in configuration files
  - [x] Daemon mode
  - [x] Front-end UI
  - [x] Installation script (via Makefile)

- TODO
  - [ ] UDP support
  - [ ] Inernal Socks proxy status repesponse
  - [ ] Inernal HTTPS proxy status repesponse
  - [ ] Inernal Socks4 proxy support
  - [ ] Socks4a protocol support
  - [ ] Internal HTTP proxy `GET` request support
  - [ ] Inernal Socks proxy basic authentication
  - [ ] Inernal HTTPS proxy basic authentication
  - [ ] OS specific `select` alternatives: `epol` / `kqueue`
  - [ ] Faster NS-Warp
  - [ ] Documentation

### Changelog

**Attention! To incorporate HTTP proxy service, deprecated `socks_*` variables will be replaced by `proxy_*` ones
in the upcoming 1.3.0 release! Be ready to replace them in `ts-warp.ini` file!**

See it [here](CHANGELOG.md)

### Installation

#### Obtain source codes

- [Download](https://github.com/mezantrop/ts-warp/archive/refs/heads/master.zip) TS-Warp sources and unarchive them
- Or clone the repository running `git` in a terminal:

  ```sh
  $ git clone https://github.com/mezantrop/ts-warp
  ```

#### Build the appplication from sources

Using terminal, in the directory with TS-Warp source code run as the normal user:

  ```sh
  $ make
  ```

#### Install the application

Typically, installation requires root privileges. Below we use `sudo` to achieve the goal, but on some operating
systems you may need to invoke `su` instead:

  ```sh
  $ sudo make install clean
  ```

  This installs all the files under the `/usr/local` tree and after that cleans source codes from object and temporary
  created files. If a different installation path is required, set `PREFIX`:

  ```sh
  $ sudo make install PREFIX=/path/to/install
  ```

#### Configure TS-Warp

Based on `<PREFIX>/etc/ts-warp.ini.sample` file create `<PREFIX>/etc/ts-warp.ini` to suite your Socks configuration.
For example:

  ```sh
  $ sudo cp /usr/local/etc/ts-warp.ini.sample /usr/local/etc/ts-warp.ini
  $ sudo nano /usr/local/etc/ts-warp.ini
  ```

*Optional*. Edit `<PREFIX>/etc/ts-warp.sh` to customize PID-, LOG- and INI-files location. For example:

```sh
$ sudo nano /usr/local/etc/ts-warp.sh
```

#### Setup firewall

- **On macOS** and **\*BSD** to configure the packet filter create `<PREFIX>/etc/ts-warp_pf.conf` based on
  `<PREFIX>/etc/ts-warp_pf.conf.sample`. For example:

  ```sh
  $ sudo cp /usr/local/etc/ts-warp_pf.conf.sample /usr/local/etc/ts-warp_pf.conf
  $ sudo nano /usr/local/etc/ts-warp_pf.conf
  ```

- **On Linux**. Create `<PREFIX>/etc/ts-warp_nftables.sh` or `<PREFIX>/etc/ts-warp_iptables.sh` using as templates
  `<PREFIX>/etc/ts-warp_nftables.sh.sample` or respectively `<PREFIX>/etc/ts-warp_iptables.sh.sample`
  to configure firewall. For example:

  ```sh
    $ sudo cp /usr/local/etc/ts-warp_nftables.sh.sample /usr/local/etc/ts-warp_nftables.sh
    $ sudo nano /usr/local/etc/ts-warp_nftables.sh
    ```

  or

    ```sh
    $ sudo cp /usr/local/etc/ts-warp_iptables.sh.sample /usr/local/etc/ts-warp_iptables.sh
    $ sudo nano /usr/local/etc/ts-warp_iptables.sh
    ```

- The helper script `<PREFIX>/bin/bin/ts-warp_autofw.sh` makes and prints out sample firewall configuration based on
  `ts-warp.ini` contents. It can be used to populate contents of `ts-warp_pf.conf`,  `ts-warp_iptables.sh` or
  `ts-warp_nftables.sh`

##### Advanced firewall configuration

There are two predefined sets of example firewall configuration files: **general** and **special**.

Simple **general** rulesets redirect all outgoing TCP traffic through TS-Warp, which in it's turn dispatches it to Socks
servers or to the destination targets. More complex **special** firewall configuration contains rules to only redirect
TCP traffic to TS-Warp that requires to be soxified. By default, to minimize system workload, `make` installs
**special** firewall rulesets, but it is possible to switch between both options using:

```sh
$ make examples-special
```

or

```sh
$ make examples-general
```

Then install the selected configuration examples:

```sh
$ sudo make install-examples
```

Specify custom `PREFIX` if other the default `/usr/local/etc` directory is desired for the configuration files.

### Usage

You can control, e.g. start, stop `ts-warp` daemon using `ts-warp.sh` script. Under root privileges or `sudo` run:

```sh
# <PREFIX>/etc/ts-warp.sh start|stop|reload|restart [options]
# <PREFIX>/etc/ts-warp.sh status
```

For example:

```sh
$ sudo /usr/local/etc/ts-warp.sh start
$ sudo /usr/local/etc/ts-warp.sh stop
```

### Low-level ts-warp daemon usage

All the ts-warp command-line options can be listed using `$ ts-warp -h`:

```sh
Usage:
  ts-warp -i IP:Port -c file.ini -l file.log -v 0-4 -d -p file.pid -f -u user -h

Version:
  TS-Warp-1.X.Y

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

 `ts-warp.sh` respects `ts-warp` daemon options. For example, to temporary enable more verbose logs, restart `ts-warp`
 with `-v 4` option:

```sh
$ sudo /usr/local/etc/ts-warp.sh restart -v 4
```

`ts-warp` understands `SIGHUP` signal as the command to reload configuration, `SIGUSR1` to display working configuration
and clients connection status and `SIGINT` to stop the daemon.

Use `ts-pass` to encode passwords if requred. See examples in [ts-warp.ini](examples/ts-warp.ini)

### GUI front-end

![gui-warp.py](gui/gui-warp_py.png)

The GUI front-end application to control `ts-warp` daemon can be installed from the `gui` directory:

```sh
$ cd gui
$ sudo make install
```

*Optionally*. Set `PREFIX`, to use a different installation target in the `make` command above:

``` sh
$ sudo make install PREFIX=/path/to/install
```

To start the GUI run:

``` sh
$ sudo -b <PREFIX>/bin/gui-warp.py
```

Note, Python 3 interpreter with `tkinter` support is required to run the GUI frontend.

### Contacts

Not so early stage of development, yet don't expect everything to work properly. If you have an idea, a question,
or have found a problem, do not hesitate to open an [issue](https://github.com/mezantrop/ts-warp/issues/new/choose)
or mail me: Mikhail Zakharov <zmey20000@yahoo.com>

Many thanks to [contributors](CONTRIBUTORS.md) of the project
