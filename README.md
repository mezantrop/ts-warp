# TS-Warp

[![CodeQL](https://github.com/mezantrop/ts-warp/actions/workflows/codeql.yml/badge.svg)](https://github.com/mezantrop/ts-warp/actions/workflows/codeql.yml)
[![C/C++ CI - macOS](https://github.com/mezantrop/ts-warp/actions/workflows/c-cpp-macos.yml/badge.svg)](https://github.com/mezantrop/ts-warp/actions/workflows/c-cpp-macos.yml)
[![C/C++ CI - Ubuntu](https://github.com/mezantrop/ts-warp/actions/workflows/c-cpp-ubuntu.yml/badge.svg)](https://github.com/mezantrop/ts-warp/actions/workflows/c-cpp-ubuntu.yml)

## Transparent proxy server and traffic wrapper

<a href="https://www.buymeacoffee.com/mezantrop" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/default-orange.png" alt="Buy Me A Coffee" height="41" width="174"></a>

### Features

- Proxy services with TCP-traffic redirection to external Socks4/5, HTTPS and SSH2* proxy servers
  - Transparent firewall-based traffic redirector
  - Internal Socks and HTTPS proxy server

\* Requires [libssh2](https://libssh2.org) library

- Supported platforms:
  - macOS, FreeBSD and OpenBSD with `PF`
  - Linux with `nftables` or `iptables`, Windows WSL2 with `iptables`

- Main features
  | Transparent proxy                      | Socks5             | Socks4               | HTTPS              | SSH2                  |
  |----------------------------------------|--------------------|----------------------|--------------------|-----------------------|
  | Proxy protocol                         | :white_check_mark: | :white_check_mark:   | :white_check_mark: | :white_check_mark:    |
  | Proxy chains                           | :white_check_mark: | :white_check_mark:   | :white_check_mark: | :white_large_square:* |
  | Proxy workload balancer                | :white_check_mark: | :white_check_mark:   | :white_check_mark: | :white_check_mark:    |
  | Authentication                         | :white_check_mark: | :white_large_square: | :white_check_mark: | :white_check_mark:    |
  | IPv6 stack support                     | :white_check_mark: | :white_large_square: | :white_check_mark: | :white_check_mark:    |
  | Remote names resolution: [NS-Warp](ns) | :white_check_mark: | :white_large_square: | :white_check_mark: | :white_check_mark:    |

\* Only one SSH2 proxy server allowed per chain

  | Internal proxy                         | Socks5               | HTTPS                |
  |----------------------------------------|----------------------|----------------------|
  | Proxy protocol                         | :white_check_mark:   | :white_check_mark:   |
  | Proxy chains                           | :white_check_mark:   | :white_check_mark:   |
  | Proxy workload balancer                | :white_check_mark:   | :white_check_mark:   |
  | Authentication                         | :white_large_square: | :white_large_square: |
  | IPv6 stack support                     | :white_check_mark:   | :white_check_mark:   |
  | Remote names resolution                | :white_check_mark:   | :white_check_mark:   |

- Miscellaneous features
  - [x] Simple configuration structure as INI-like file
  - [x] Password encoding (obfuscation) in configuration files
  - [x] Daemon mode
  - [x] Front-end UI
  - [x] Installation script (via Makefile)

- TODO
  - [ ] UDP support
  - [ ] Internal Socks4 proxy support
  - [ ] Socks4a protocol support
  - [ ] OS specific `select` alternatives: `epol` / `kqueue`
  - [ ] Faster NS-Warp
  - [ ] Documentation

### Changelog

**Attention! To incorporate HTTP proxy service, `socks_*` variables in `ts-warp.ini` are replaced by `proxy_*` ones.
The deprecated variables will be eventually removed in the further releases**

See it [here](CHANGELOG.md)

### Quick Installation

```sh
git clone https://github.com/mezantrop/ts-warp && cd ts-warp
make && sudo make install clean

# If SSH2 proxy support is required, install https://libssh2.org library and instead of the above run:
make ts-warp-ssh2 sudo make install clean

# Copy and edit configuration files
sudo cp /usr/local/etc/ts-warp.ini.sample /usr/local/etc/ts-warp.ini && sudo vi /usr/local/etc/ts-warp.ini

# on *BSD and macOS
sudo cp /usr/local/etc/ts-warp_pf.conf.sample /usr/local/etc/ts-warp_pf.conf
sudo vi /usr/local/etc/ts-warp_pf.conf

# on Linux with nftables
sudo cp /usr/local/etc/ts-warp_nftables.sh.sample /usr/local/etc/ts-warp_nftables.sh
sudo vi /usr/local/etc/ts-warp_nftables.sh

# on Linux with iptables
sudo cp /usr/local/etc/ts-warp_iptables.sh.sample /usr/local/etc/ts-warp_iptables.sh
sudo vi /usr/local/etc/ts-warp_iptables.sh

# on Windows WSL2 (Ubuntu) with iptables
wsl --set-default-version 2

Packages required for CLI: gcc, make. For GUI-Warp: python3-tk

sudo cp /usr/local/etc/ts-warp_iptables.sh.sample /usr/local/etc/ts-warp_iptables.sh
sudo vi /usr/local/etc/ts-warp_iptables.sh
```

### Longer Installation

#### Obtain source codes

- [Download](https://github.com/mezantrop/ts-warp/archive/refs/heads/master.zip) TS-Warp sources and unarchive them
- Or clone the repository running `git` in a terminal:

  ```sh
  git clone https://github.com/mezantrop/ts-warp
  ```

#### Build the appplication from sources

Using terminal, in the directory with TS-Warp source code run as the normal user:

  ```sh
  make
  ```

If you aplanning to install into a diferent location, than the default `/usr/local`, you may considering to change the
built-in default paths as well:

  ```sh
  make PREFIX=/path/to/install
  ```

If SSH2 proxy support is required, install https://libssh2.org library first and run:

  ```sh
  make ts-warp-ssh2 sudo make install clean
  ```

#### Install the application

Typically, installation requires root privileges. Below we use `sudo` to achieve the goal, but on some operating
systems you may need to invoke `su` instead:

  ```sh
  sudo make install clean
  ```

  This installs all the files under the `/usr/local` tree and after that cleans source codes from object and temporary
  created files. If a different installation path is required, set `PREFIX`:

  ```sh
  sudo make install PREFIX=/path/to/install
  ```

#### Configure TS-Warp

Based on `<PREFIX>/etc/ts-warp.ini.sample` file create `<PREFIX>/etc/ts-warp.ini` to suite your Socks configuration.
For example:

  ```sh
  sudo cp /usr/local/etc/ts-warp.ini.sample /usr/local/etc/ts-warp.ini
  sudo nano /usr/local/etc/ts-warp.ini
  ```

Note, besides `nano` there could be other editors installed e.g. `pico`, `ee`, `vi`, etc.

*Optional*. Edit `<PREFIX>/etc/ts-warp.sh` to customize PID-, LOG- and INI-files location. For example:

```sh
sudo nano /usr/local/etc/ts-warp.sh
```

#### Setup firewall

- **On macOS** and **\*BSD** to configure the packet filter create `<PREFIX>/etc/ts-warp_pf.conf` based on
  `<PREFIX>/etc/ts-warp_pf.conf.sample`. For example:

  ```sh
  sudo cp /usr/local/etc/ts-warp_pf.conf.sample /usr/local/etc/ts-warp_pf.conf
  sudo nano /usr/local/etc/ts-warp_pf.conf
  ```

- **On Linux**. Create `<PREFIX>/etc/ts-warp_nftables.sh` or `<PREFIX>/etc/ts-warp_iptables.sh` using as templates
  `<PREFIX>/etc/ts-warp_nftables.sh.sample` or respectively `<PREFIX>/etc/ts-warp_iptables.sh.sample`
  to configure firewall. For example:

  ```sh
    sudo cp /usr/local/etc/ts-warp_nftables.sh.sample /usr/local/etc/ts-warp_nftables.sh
    sudo nano /usr/local/etc/ts-warp_nftables.sh
    ```

  or

    ```sh
    sudo cp /usr/local/etc/ts-warp_iptables.sh.sample /usr/local/etc/ts-warp_iptables.sh
    sudo nano /usr/local/etc/ts-warp_iptables.sh
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
make examples-special
```

or

```sh
make examples-general
```

Then install the selected configuration examples:

```sh
sudo make install-examples
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
sudo /usr/local/etc/ts-warp.sh start
sudo /usr/local/etc/ts-warp.sh stop
```

After succesfull start, TS-Warp transparently redirects traffic according to the configuration specified in
`ts-warp.ini` and firewall rules. Also, TS-Warp spawns Socks5 proxy server at `localhost:10800` and HTTPS proxy
(CONNECT method) at `localhost:8080`.

### Low-level ts-warp daemon usage

All the ts-warp command-line options can be listed using `$ ts-warp -h`:

```sh
Usage:
  ts-warp -T IP:Port -S IP:Port -H IP:Port -c file.ini -l file.log -v 0-4 -t file.act -d -p file.pid -f -u user -h

Version:
  TS-Warp-X.Y.Z

All parameters are optional:
  -T IP:Port      Local IP address and port for incoming Transparent requests
  -S IP:Port      Local IP address and port for internal Socks server
  -H IP:Port      Local IP address and port for internal HTTP server

  -l file.log     Main log filename
  -v 0..4         Log verbosity level: 0 - off, default: 3
  -t file.act     Active connections and traffic log

  -d              Daemon mode
  -p file.pid     PID filename
  -f              Force start

  -u user         A user to run ts-warp, default: nobody

  -h              This message
```

 `ts-warp.sh` respects `ts-warp` daemon options. For example, to temporary enable more verbose logs, restart `ts-warp`
 with `-v 4` option:

```sh
sudo /usr/local/etc/ts-warp.sh restart -v 4
```

`ts-warp` understands several signals:

- `SIGHUP` signal as the command to reload configuration
- `SIGUSR1` to display current configuration state. Note, load balancer can dynamically reorder configuration sections
- `SIGUSR2` to show active clients connection status and traffic stats
- `SIGINT` to stop the daemon.

Use `ts-pass` to encode passwords if requred. See examples in [ts-warp.ini](examples/ts-warp.ini)

### GUI front-end

![gui-warp.py](gui/gui-warp_py.png)

The GUI front-end application to control `ts-warp` daemon can be installed from the `gui` directory:

```sh
cd gui
sudo make install
```

*Optionally*. Set `PREFIX`, to use a different installation target in the `make` command above:

``` sh
sudo make install PREFIX=/path/to/install
```

To start the GUI run:

``` sh
sudo -b <PREFIX>/bin/gui-warp.py
```

Note, Python 3 interpreter with `tkinter` support is required to run the GUI frontend.

### macOS All-in-one TS-Warp + GUI-Warp App

Check [releases](https://github.com/mezantrop/ts-warp/releases) and download macOS standalone precompiled application.
Read related [README.md](gui/ports/macOS/README.md) for information and instructions.

### Contacts

Not so early stage of development, yet don't expect everything to work properly. If you have an idea, a question,
or have found a problem, do not hesitate to open an [issue](https://github.com/mezantrop/ts-warp/issues/new/choose)
or mail me: Mikhail Zakharov <zmey20000@yahoo.com>

Many thanks to [contributors](CONTRIBUTORS.md) of the project
