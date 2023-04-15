# TS-Warp

[![CodeQL](https://github.com/mezantrop/ts-warp/actions/workflows/codeql.yml/badge.svg)](https://github.com/mezantrop/ts-warp/actions/workflows/codeql.yml)

## Transparent SOCKS proxy Wrapper

<a href="https://www.buymeacoffee.com/mezantrop" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/default-orange.png" alt="Buy Me A Coffee" height="41" width="174"></a>

### Goals and TODO list

- [x] Create a socksifier service - transparent firewall-based redirector of TCP/IP connections to a SOCKS-proxy server

- Support platforms:
  - [x] macOS, FreeBSD and OpenBSD with PF
  - [x] Linux with nftables or iptables

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
- [x] Remote names resolution using [NS-Warp](https://github.com/mezantrop/ts-warp/tree/master/ns)
- [ ] (optional) HTTP proxy
- [x] SOCKS workload balance modes: `Failover`/`Roundrobin`/`None`

### Changelog

See it [here](CHANGELOG.md)

### Installation

#### Obtain source codes

- [Download](https://github.com/mezantrop/ts-warp/archive/refs/heads/master.zip) TS-Warp sources and unarchive them
- Or clone the repository running `git` in a terminal:
  
  ```sh
  $ git clone https://github.com/mezantrop/ts-warp
  ```

#### Build the appplication from sources

Using terminal, in the directory with TS-Warp source code run:

  ```sh
  $ make
  ```

#### Install the application

Typically, the operation requires root privileges, below we use `sudo` to achieve the goal, but on some operating
systems you may need to invoke `su` instead:
  
  ```sh
  $ sudo make install
  ```
  
  This installs all the files under the `/usr/local` tree. If a different installation path is required, set `PREFIX`:
  
  ```sh
  $ sudo make install PREFIX=/path/to/install
  ```

#### Configure TS-Warp

Based on `<PREFIX>/etc/ts-warp.ini.sample` file, create `<PREFIX>/etc/ts-warp.ini` to suite your SOCKS configuration.
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

Above mentioned `make install` command installs a **general** firewall configuration **sample**-file, that contains
forwards rules to forward all TCP traffic to TS-Warp. This example can be used to create a working configuration:

##### Using make

**Beware** the custom firewall configuration is overwritten by the newly installed one:

```sh
$ sudo make install-configs
```

Specify custom `PREFIX` if other the default `/usr/local/etc` directory is desired for the configuration files.

**Note!** Previously installed firewall configuration files are saved with the `old` filename extension.

##### Manually

- **On macOS and \*BSD**. Create `<PREFIX>/etc/ts-warp_pf.conf` based on appropriate `<PREFIX>/etc/ts-warp_pf.conf.sample`
to configure the packet filter:

  ```sh
  $ sudo cp /usr/local/etc/ts-warp_pf.conf.sample /usr/local/etc/ts-warp_pf.conf
  $ sudo nano /usr/local/etc/ts-warp_pf.conf
  ```

- **On Linux**. Create `<PREFIX>/etc/ts-warp_nftables.sh` or `<PREFIX>/etc/ts-warp_iptables.sh` based on
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

##### Advanced firewall configuration

If for some reasons **general** firewall configuration is not suitable i.e. you don't want all the TCP traffic to go
through TS-Warp, it is possible to use more complex **special** versions of the firewall configuration files.
To build them run:

```sh
$ make examples-special
```

Install the **special** versions of the **sample** files:

```
$ sudo make install-examples
```

Install the configuration files:

```sh
$ sudo make install-configs
```

Specify custom `PREFIX` if other the default `/usr/local/etc` directory is desired for the configuration files.

**Note!** Previously installed firewall configuration files are saved with the `old` filename extension.

Finally, edit either `ts-warp_pf.conf`, or if you are on Linux, `ts-warp_nftables.sh` or `ts-warp_iptables.sh` to define
firewall rules, suitable for your environment.

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
