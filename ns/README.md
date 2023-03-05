# NS-Warp

## DNS responder/proxy for TS-Warp

NS-Warp allows to perform remote name resolution via SOCKS proxy server for transparent TS-Warp initiated connections
and solve the issue if a target hostname is not known to a local DNS and you have to connect using an IP-address.

NS-Warp uses an IP subnet to define Name to IP translation Table (NIT), to temporary associate hostnames with
IP-addresses from NIT before supplying them to TS-Warp.

### Installation

```sh
# sudo make install
```

Install NS-Warp under the default `PREFIX=/usr/local`

### Configuration

* Edit `ts-warp.ini` to add a NIT-pool and a related target network to a desired section:

  ```sh
  vi /usr/local/etc/ts-warp.ini
  [EXAMPLE]
  # ...
  # socks_server ...
  # target_network ...
  target_network = 192.168.168.0/255.255.255.0
  nit_pool = example.net:192.168.168.0/255.255.255.0
  # ...
  ```

  Defining `nit_pool` and `target_network` for NS-Warp to work, make sure this subnet is not used for actual connections.

### Usage

Start NS-Warp:

```sh
# /usr/local/etc/ns-warp.sh start
```

NS-Warp understands also `stop` and `restart` commands

Notes:

* NS-Warp logs to `/usr/local/var/log/ns-warp.log`
* Adding `-v 4` option enables more verbose logging, e.g. `# /usr/local/etc/ns-warp.sh start -v 4`
* On Linux check contents of `ns-warp_iptables.sh` and `ns-warp_nftables.sh` for *BSD systems see the `ns-warp.sh` for the firewall rules.

### Contacts

This is a very early development stage, NS-Warp contains bugs, there are many things to implement. If you have an idea,
a question, or have found a problem, do not hesitate to open an [issue](https://github.com/mezantrop/ts-warp/issues/new/choose)
or mail me: Mikhail Zakharov <zmey20000@yahoo.com>
