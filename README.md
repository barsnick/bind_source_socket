LD_PRELOAD library to make bind and connect to use a virtual
IP address as localaddress. Specified via the enviroment
variable BIND_ADDR.

Compile on Linux with:
gcc -nostartfiles -fpic -shared bind.c -o bind.so -ldl -D_GNU_SOURCE


Example in bash to make inetd only listen to the localhost
lo interface, thus disabling remote connections and only
enable to/from localhost:

BIND_ADDR="127.0.0.1" LD_PRELOAD=./bind.so /sbin/inetd


Example in bash to use your virtual IP as your outgoing
sourceaddress for ircII:

BIND_ADDR="your-virt-ip" LD_PRELOAD=./bind.so ircII

Note that you have to set up your servers virtual IP first.


This program was made by Daniel Ryde
email: daniel@ryde.net
web:   http://www.ryde.net/

TODO: I would like to extend it to the accept calls too, like a
general tcp-wrapper. Also like an junkbuster for web-banners.
For libc5 you need to replace socklen_t with int.
