/* ------------------------------------------------------------------------------------------------------------------ */
/* TS-Warp - Transparent Socks proxy server and traffic Wrapper                                                       */
/* ------------------------------------------------------------------------------------------------------------------ */

/*
* Copyright (c) 2021-2023, Mikhail Zakharov <zmey20000@yahoo.com>
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
* following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
*    disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*    the following disclaimer in the documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* ------------------------------------------------------------------------------------------------------------------ */
#include "version.h"

#define PROXY_PROTO_HTTP            'H'

#define HTTP_PROXY_REPLY_200        "HTTP/1.1 200 OK\r\nProxy-agent: "PROG_NAME_FULL"\r\n\r\n"

#define HTTP_REQUEST_METHOD_CONNECT "CONNECT"

#define HTTP_REQEST_PROTOCOL        "HTTP/1.1"

#define HTTP_RESPONSE_200           "200"


/* ------------------------------------------------------------------------------------------------------------------ */
int http_server_request(int socket, struct uvaddr *daddr);
int http_client_request(int socket, struct sockaddr_storage *daddr);