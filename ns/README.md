# NS-Warp

## DNS responder/proxy for TS-Warp

NS-Warp allows to perform remote name resolution via SOCKS proxy server for transparent TS-Warp initiated connections
and solve the issue if a target hostname is not known to a local DNS and you have to connect using an IP-address.

NS-Warp uses an IP subnet to define Name to IP translation Table (NIT), to temporary associate hostnames with
IP-addresses from NIT before supplying them to TS-Warp.

**Please note, NS-Warp is on early testing stage. Use it with extra caution in a testing environment**.

### Installation

```sh
# sudo make install
```

Install NS-Warp under the default `PREFIX=/usr/local`

### Configuration

* Edit /etc/rc.conf to add local IP address `127.0.0.1` as the first name server in the list:

  ```sh
  vi /etc/resolv.conf
  # ...
  nameserver 127.0.0.1
  nameserver 192.168.1.1
  # ...
  ```

  Here, `127.0.0.1` is the address where NS-Warp runs and `192.168.1.1` is the address of original DNS-server.
  Make sure, that in the `resolf.conf` NS-Warp server is defind upper than the original DNS-server.

* Edit `ts-warp.ini` to add a NIT-pool and a related target network to a desired section:

  ```sh
  vi  /usr/local/etc/ts-warp.ini
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

Start NS-Warp with options to listen on the localhost IP-address `127.0.0.1` and forward requests to the original
DNS-server, which is `192.168.1.1` in the example:

```sh
# ns-warp -i 127.0.0.1:53 -s 192.168.1.1:53 -c /usr/local/etc/ts-warp.ini -d
```

Notes:

* NS-Warp logs to `/usr/local/var/log/ns-warp.log`
* Adding `-v 4` option enables more verbose logging
* There is no start/stop script yet, use `kill <PID>` to stop NS-Warp

### Contacts

This is a very early development stage, NS-Warp contains bugs, there are many things to implement. If you have an idea,
a question, or have found a problem, do not hesitate to open an [issue](https://github.com/mezantrop/ts-warp/issues/new/choose)
or mail me: Mikhail Zakharov <zmey20000@yahoo.com>
