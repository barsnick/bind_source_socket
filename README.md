# Bind a program's TCP source port to a certain address
## A library to assist Linux/glibc based programs in binding their TCP source port to a certain source address when creating a connection

This library assists Linux/glibc based programs in binding their TCP source port to a certain source address when creating a connection.

# Use cases
A common use case is when a computer has several outgoing network devices, such as:
- an Ethernet port and a VPN network device tunneling through it
- a standard network port and a virtual network device to a virtual machine

If you want your program to connect to a host, you may want it to use a certain network device (e.g. avoiding the VPN). Linux may not support this for you, deciding by itself which of the possibly routes to take.  The program itself can influence this decision when creating its TCP socket, but the program source may not be under your control, or you don't want to implement such special cases. You could reconfigure your routing to achieve this, but that is a massive manipulation of the system.

# How it works
This library is loaded into your program and manipulates its socket configuration. It uses the `LD_PRELOAD` mechanism to make `bind()` and `connect()` behave differently, by using the given address as local address.

# Building

Compile on Linux with:
```
gcc -nostartfiles -fpic -shared bind.c -o bind.so -ldl -D_GNU_SOURCE
```

For libc5 you need to replace `socklen_t` with `int` in the source.

# Usage
The path to the compiled library (shared object) needs to be available to your program via the `LD_PRELOAD` environment variable.

All sockets opened by the program will be affected. Here are the variables to manipulate their options:

- Listening sockets:
  - The address to be used is specified via the enviroment variable `BIND_ADDR` (default: chosen by operating system).
  - The port to be used is specified via the enviroment variable `BIND_PORT` (default: chosen by operating system).
  - The type of sockets to modify is specified via the enviroment variable `BIND_TYPE` (possible values: `TCP`, `UDP`, `RAW`, `PACKET`; default: all).
  - The ports *not* to be modified are specified via the enviroment variable `EXCLUDE_PORTS` (possible value: comma separated list of ports; default: none).
- Connecting sockets:
  - The address to be used is specified via the enviroment variable `CONNECT_BIND_ADDR` (default: chosen by operating system).
  - The port to be used is specified via the enviroment variable `CONNECT_BIND_PORT` (default: chosen by operating system).
  - The type of sockets to modify is specified via the enviroment variable `CONNECT_TYPE` (possible values: `TCP`, `UDP`, `RAW`, `PACKET`; default: all).
  - The ports *not* to be modified are specified via the enviroment variable `EXCLUDE_PORTS` (possible value: comma separated list of ports; default: none).

## Examples
Example in bash to make inetd only listen to the localhost `lo` interface, thus disabling remote connections and only enabling to/from localhost:

```
BIND_ADDR="127.0.0.1" LD_PRELOAD=./bind.so /sbin/inetd
```


Example in bash to use your virtual IP as your outgoing source address for the program `ircII`:

```
BIND_ADDR="your-virt-ip" CONNECT_ADDR="your-virt-ip" LD_PRELOAD=./bind.so ircII
```

Note that you have to set up your server's virtual IP first.

# TODO
TODO: I would like to extend it to the accept() calls too, like a general TCP wrapper. Also like a junkbuster for web banners.

# Credits
This program was made by Daniel Ryde  
email: `daniel@ryde.net`  
web:   `http://www.ryde.net/`

The additional binding options functionality was added by Uranus Zhou ([GitHub "zohead"](https://github.com/zohead)).
It is originally hosted here: https://gist.github.com/zohead/9950663ca01952c940eb.

This README was created from a comment in the original source, and modified by Moritz Barsnick.
