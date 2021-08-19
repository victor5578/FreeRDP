/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * FreeRDP Proxy Server
 *
 * Copyright 2019 Kobi Mizrachi <kmizrachi18@gmail.com>
 * Copyright 2019 Idan Freiberg <speidy@gmail.com>
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

#ifndef FREERDP_SERVER_PROXY_MODULES_H
#define FREERDP_SERVER_PROXY_MODULES_H

#include <winpr/wtypes.h>
#include <winpr/collections.h>

#include <freerdp/api.h>

#include <freerdp/server/proxy/proxy_modules_api.h>

enum _PF_FILTER_TYPE
{
	FILTER_TYPE_KEYBOARD,
	FILTER_TYPE_MOUSE,
	FILTER_TYPE_CLIENT_PASSTHROUGH_CHANNEL_DATA,

	FILTER_TYPE_SERVER_PASSTHROUGH_CHANNEL_DATA,
	FILTER_TYPE_SERVER_FETCH_TARGET_ADDR,

	FILTER_LAST
};
typedef enum _PF_FILTER_TYPE PF_FILTER_TYPE;

enum _PF_HOOK_TYPE
{
	HOOK_TYPE_CLIENT_PRE_CONNECT,
	HOOK_TYPE_CLIENT_POST_CONNECT,
	HOOK_TYPE_CLIENT_LOGIN_FAILURE,
	HOOK_TYPE_CLIENT_END_PAINT,

	HOOK_TYPE_SERVER_POST_CONNECT,
	HOOK_TYPE_SERVER_CHANNELS_INIT,
	HOOK_TYPE_SERVER_CHANNELS_FREE,
	HOOK_TYPE_SERVER_SESSION_END,

	HOOK_LAST
};
typedef enum _PF_HOOK_TYPE PF_HOOK_TYPE;

typedef struct proxy_module proxyModule;

#ifdef __cplusplus
extern "C"
{
#endif

	FREERDP_API proxyModule* pf_modules_new(const char* root_dir, const char** modules,
	                                        size_t count);

	/**
	 * @brief pf_modules_add Registers a new plugin
	 * @param ep A module entry point function, must NOT be NULL
	 * @return TRUE for success, FALSE otherwise
	 */
	FREERDP_API BOOL pf_modules_add(proxyModule* module, proxyModuleEntryPoint ep);

	FREERDP_API BOOL pf_modules_is_plugin_loaded(proxyModule* module, const char* plugin_name);
	FREERDP_API void pf_modules_list_loaded_plugins(proxyModule* module);

	FREERDP_API BOOL pf_modules_run_filter(proxyModule* module, PF_FILTER_TYPE type,
	                                       proxyData* pdata, void* param);
	FREERDP_API BOOL pf_modules_run_hook(proxyModule* module, PF_HOOK_TYPE type, proxyData* pdata);

	FREERDP_API void pf_modules_free(proxyModule* module);

#ifdef __cplusplus
}
#endif

#endif /* FREERDP_SERVER_PROXY_MODULES_H */
