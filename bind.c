/*
   Copyright (C) 2000  Daniel Ryde

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
*/

/*
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
*/

/*
   This is an updated version of Daniel Ryde's bind.c shim for IPv6
   To override an IPv6 bind address, LD_PRELOAD the library as usual
   but also set BIND_ADDR6
*/


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dlfcn.h>
#include <errno.h>
#include <string.h>

int (*real_bind)(int, const struct sockaddr *, socklen_t);
int (*real_connect)(int, const struct sockaddr *, socklen_t);

char *bind_addr_env;
char *bind_addr6_env;
unsigned long int bind_addr_saddr;
unsigned char bind_addr_saddr6[16];
unsigned long int inaddr_any_saddr;
struct sockaddr_in local_sockaddr_in[] = { 0 };
struct sockaddr_in6 local_sockaddr_in6[] = { 0 };

void _init (void)
{
	const char *err;

	real_bind = dlsym (RTLD_NEXT, "bind");
	if ((err = dlerror ()) != NULL) {
		fprintf (stderr, "dlsym (bind): %s\n", err);
	}

	real_connect = dlsym (RTLD_NEXT, "connect");
	if ((err = dlerror ()) != NULL) {
		fprintf (stderr, "dlsym (connect): %s\n", err);
	}

	inaddr_any_saddr = htonl (INADDR_ANY);
	if (bind_addr_env = getenv ("BIND_ADDR")) {
		bind_addr_saddr = inet_addr (bind_addr_env);
		local_sockaddr_in->sin_family = AF_INET;
		local_sockaddr_in->sin_addr.s_addr = bind_addr_saddr;
		local_sockaddr_in->sin_port = htons (0);
	}
	if (bind_addr6_env = getenv ("BIND_ADDR6")) {
		if (inet_pton(AF_INET6, bind_addr6_env, bind_addr_saddr6) > 0) {
			local_sockaddr_in6->sin6_family = AF_INET6;
			memcpy(local_sockaddr_in6->sin6_addr.s6_addr, bind_addr_saddr6, 16);
			local_sockaddr_in6->sin6_port = htons (0);
		}
	}
}

int bind (int fd, const struct sockaddr *sk, socklen_t sl)
{

/*	printf("bind: %d %s:%d\n", fd, inet_ntoa (lsk_in->sin_addr.s_addr),
		ntohs (lsk_in->sin_port));*/
	if ((sk->sa_family == AF_INET) && (bind_addr_env)) {
		static struct sockaddr_in *lsk_in;
		lsk_in = (struct sockaddr_in *)sk;
		lsk_in->sin_addr.s_addr = bind_addr_saddr;
	} else if ((sk->sa_family == AF_INET6) && (bind_addr6_env)) {
		static struct sockaddr_in6 *lsk_in6;
		lsk_in6 = (struct sockaddr_in6 *)sk;
		memcpy(lsk_in6->sin6_addr.s6_addr, bind_addr_saddr6, 16);
	}
	return real_bind (fd, sk, sl);
}

int connect (int fd, const struct sockaddr *sk, socklen_t sl)
{

/*	printf("connect: %d %s:%d\n", fd, inet_ntoa (rsk_in->sin_addr.s_addr),
		ntohs (rsk_in->sin_port));*/
	if ((sk->sa_family == AF_INET) && (bind_addr_env)) {
		static struct sockaddr_in *rsk_in;
		rsk_in = (struct sockaddr_in *)sk;
		real_bind (fd, (struct sockaddr *)local_sockaddr_in, sizeof (struct sockaddr));
	} else if ((sk->sa_family == AF_INET6) && (bind_addr6_env)) {
		static struct sockaddr_in6 *rsk_in6;
		rsk_in6 = (struct sockaddr_in6 *)sk;
		real_bind (fd, (struct sockaddr *)local_sockaddr_in6, sizeof (struct sockaddr_in6));
	}
	return real_connect (fd, sk, sl);
}
