/*
   Copyright (C) 2000  Daniel Ryde

   Modified by Uranus Zhou (2014)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dlfcn.h>
#include <errno.h>
#include <string.h>

int (*real_bind)(int, const struct sockaddr *, socklen_t);
int (*real_connect)(int, const struct sockaddr *, socklen_t);

char *bind_addr_env, *connect_bind_addr_env, *bind_port_env, *bind_type_env, *exclude_ports_env, *connect_port_env, *connect_type_env, *connect_bind_port_env;
unsigned long int bind_addr_saddr;
struct sockaddr_in local_sockaddr_in[] = { 0 };
int bind_port_ns = -1, connect_port_ns = -1;
int bind_type = -1, connect_type = -1;

static int get_sock_type(const char *str)
{
	if (strcasecmp(str, "TCP") == 0)
		return SOCK_STREAM;
	else if (strcasecmp(str, "UDP") == 0)
		return SOCK_DGRAM;
	else if (strcasecmp(str, "RAW") == 0)
		return SOCK_RAW;
	else if (strcasecmp(str, "PACKET") == 0)
		return SOCK_PACKET;
	else
		return -1;
}

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

	if (bind_addr_env = getenv ("BIND_ADDR"))
		bind_addr_saddr = inet_addr (bind_addr_env);

	if (bind_port_env = getenv ("BIND_PORT"))
		bind_port_ns = htons (atoi(bind_port_env));

	if (bind_type_env = getenv ("BIND_TYPE"))
		bind_type = get_sock_type(bind_type_env);

	exclude_ports_env = getenv ("EXCLUDE_PORTS");

	if (connect_port_env = getenv ("CONNECT_PORT"))
		connect_port_ns = htons (atoi(connect_port_env));

	if (connect_type_env = getenv ("CONNECT_TYPE"))
		connect_type = get_sock_type(connect_type_env);

	if (connect_bind_addr_env = getenv ("CONNECT_BIND_ADDR")) {
		local_sockaddr_in->sin_family = AF_INET;
		local_sockaddr_in->sin_addr.s_addr = inet_addr (connect_bind_addr_env);
		local_sockaddr_in->sin_port = htons (0);
	}

	if (connect_bind_port_env = getenv ("CONNECT_BIND_PORT"))
		local_sockaddr_in->sin_port = htons (atoi(connect_bind_port_env));
}

static int check_port(int port)
{
	char *tmp = exclude_ports_env, *str = NULL;
	if (tmp == NULL) return 0;

	while (1) {
		char szPort[50] = {0};
		str = strchr(tmp, ',');
		if (str == NULL)
			strncpy(szPort, tmp, sizeof(szPort));
		else
			strncpy(szPort, tmp, str - tmp);
		if (atoi(szPort) == port) return 1;
		if (str == NULL) break;
		tmp = str + 1;
	}

	return 0;
}

int bind (int fd, const struct sockaddr *sk, socklen_t sl)
{
	static struct sockaddr_in *lsk_in;

	lsk_in = (struct sockaddr_in *)sk;
	if (lsk_in->sin_family == AF_INET || lsk_in->sin_family == AF_INET6) {
		int type, length = sizeof(int);

		getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &length);

		if (check_port(ntohs(lsk_in->sin_port)) == 0 && (bind_type_env == NULL || bind_type == type)) {
			// change bind address
			if (lsk_in->sin_addr.s_addr == htonl (INADDR_ANY) && bind_addr_env)
				lsk_in->sin_addr.s_addr = bind_addr_saddr;

			// change bind port
			if (bind_port_env)
				lsk_in->sin_port = bind_port_ns;
		}
	}
	return real_bind (fd, sk, sl);
}

int connect (int fd, const struct sockaddr *sk, socklen_t sl)
{
	static struct sockaddr_in *rsk_in;

	rsk_in = (struct sockaddr_in *)sk;
	if (rsk_in->sin_family == AF_INET || rsk_in->sin_family == AF_INET6) {
		int type, length = sizeof(int);

		getsockopt(fd, SOL_SOCKET, SO_TYPE, &type, &length);

		if ((connect_port_env == NULL || connect_port_ns == rsk_in->sin_port) && (connect_type_env == NULL || connect_type == type)) {
			// change connect bind address or bind port
			if (connect_bind_addr_env)
				real_bind (fd, (struct sockaddr *)local_sockaddr_in, sizeof (struct sockaddr));
		}
	}
	return real_connect (fd, sk, sl);
}
