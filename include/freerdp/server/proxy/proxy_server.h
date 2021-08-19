/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * FreeRDP Proxy Server
 *
 * Copyright 2021 Armin Novak <armin.novak@thincast.com>
 * Copyright 2021 Thincast Technologies GmbH
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef FREERDP_SERVER_PROXY_SERVER_H
#define FREERDP_SERVER_PROXY_SERVER_H

#include <freerdp/api.h>
#include <freerdp/server/proxy/proxy_config.h>

typedef struct proxy_server proxyServer;

#ifdef __cplusplus
extern "C"
{
#endif

	FREERDP_API proxyServer* pf_server_new(proxyConfig* config);
	FREERDP_API void pf_server_free(proxyServer* server);
	FREERDP_API BOOL pf_server_start(proxyServer* server);
	FREERDP_API void pf_server_stop(proxyServer* server);

#ifdef __cplusplus
}
#endif

#endif /* FREERDP_SERVER_PROXY_SERVER_H */
