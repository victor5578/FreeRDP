/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * FreeRDP Proxy Server modules API
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

#include <winpr/assert.h>

#include <winpr/file.h>
#include <winpr/wlog.h>
#include <winpr/library.h>
#include <freerdp/api.h>
#include <freerdp/build-config.h>

#include <freerdp/server/proxy/proxy_log.h>
#include <freerdp/server/proxy/proxy_modules_api.h>

#include "pf_context.h"
#include "proxy_modules.h"

#define TAG PROXY_TAG("modules")

#define MODULE_ENTRY_POINT "proxy_module_entry_point"

typedef struct proxy_plugin_internal
{
	proxyPlugin plugin;
	proxyModule* module;
	void* userdata;
} proxyPluginInternal;

struct proxy_module
{
	proxyPluginsManager mgr;
	wArrayList* plugins;
	wArrayList* handles;
};

static const char* FILTER_TYPE_STRINGS[] = { "KEYBOARD_EVENT", "MOUSE_EVENT", "CLIENT_CHANNEL_DATA",
	                                         "SERVER_CHANNEL_DATA", "SERVER_FETCH_TARGET_ADDR" };

static const char* HOOK_TYPE_STRINGS[] = {
	"CLIENT_PRE_CONNECT",  "CLIENT_POST_CONNECT",  "CLIENT_LOGIN_FAILURE", "CLIENT_END_PAINT",
	"SERVER_POST_CONNECT", "SERVER_CHANNELS_INIT", "SERVER_CHANNELS_FREE", "SERVER_SESSION_END",
};

static const char* pf_modules_get_filter_type_string(PF_FILTER_TYPE result)
{
	if (result >= FILTER_TYPE_KEYBOARD && result < FILTER_LAST)
		return FILTER_TYPE_STRINGS[result];
	else
		return "FILTER_UNKNOWN";
}

static const char* pf_modules_get_hook_type_string(PF_HOOK_TYPE result)
{
	if (result >= HOOK_TYPE_CLIENT_PRE_CONNECT && result < HOOK_LAST)
		return HOOK_TYPE_STRINGS[result];
	else
		return "HOOK_UNKNOWN";
}

static BOOL pf_modules_proxy_ArrayList_ForEachFkt(void* data, size_t index, va_list ap)
{
	proxyPluginInternal* plugin = (proxyPluginInternal*)data;
	PF_HOOK_TYPE type;
	proxyData* pdata;
	BOOL ok = TRUE;

	WINPR_UNUSED(index);

	type = va_arg(ap, PF_HOOK_TYPE);
	pdata = va_arg(ap, proxyData*);

	WLog_VRB(TAG, "running hook %s.%s", plugin->plugin.name, pf_modules_get_hook_type_string(type));

	switch (type)
	{
		case HOOK_TYPE_CLIENT_PRE_CONNECT:
			IFCALLRET(plugin->plugin.ClientPreConnect, ok, pdata);
			break;

		case HOOK_TYPE_CLIENT_POST_CONNECT:
			IFCALLRET(plugin->plugin.ClientPostConnect, ok, pdata);
			break;

		case HOOK_TYPE_CLIENT_LOGIN_FAILURE:
			IFCALLRET(plugin->plugin.ClientLoginFailure, ok, pdata);
			break;

		case HOOK_TYPE_CLIENT_END_PAINT:
			IFCALLRET(plugin->plugin.ClientEndPaint, ok, pdata);
			break;

		case HOOK_TYPE_SERVER_POST_CONNECT:
			IFCALLRET(plugin->plugin.ServerPostConnect, ok, pdata);
			break;

		case HOOK_TYPE_SERVER_CHANNELS_INIT:
			IFCALLRET(plugin->plugin.ServerChannelsInit, ok, pdata);
			break;

		case HOOK_TYPE_SERVER_CHANNELS_FREE:
			IFCALLRET(plugin->plugin.ServerChannelsFree, ok, pdata);
			break;

		case HOOK_TYPE_SERVER_SESSION_END:
			IFCALLRET(plugin->plugin.ServerSessionEnd, ok, pdata);
			break;

		case HOOK_LAST:
		default:
			WLog_ERR(TAG, "invalid hook called");
	}

	if (!ok)
	{
		WLog_INFO(TAG, "plugin %s, hook %s failed!", plugin->plugin.name,
		          pf_modules_get_hook_type_string(type));
		return FALSE;
	}
	return TRUE;
}

/*
 * runs all hooks of type `type`.
 *
 * @type: hook type to run.
 * @server: pointer of server's rdpContext struct of the current session.
 */
BOOL pf_modules_run_hook(proxyModule* module, PF_HOOK_TYPE type, proxyData* pdata)
{
	WINPR_ASSERT(module);
	WINPR_ASSERT(module->plugins);
	return ArrayList_ForEach(module->plugins, pf_modules_proxy_ArrayList_ForEachFkt, type, pdata);
}

static BOOL pf_modules_ArrayList_ForEachFkt(void* data, size_t index, va_list ap)
{
	proxyPluginInternal* plugin = (proxyPluginInternal*)data;
	PF_FILTER_TYPE type;
	proxyData* pdata;
	void* param;
	BOOL result = TRUE;

	WINPR_UNUSED(index);

	type = va_arg(ap, PF_FILTER_TYPE);
	pdata = va_arg(ap, proxyData*);
	param = va_arg(ap, void*);

	WLog_VRB(TAG, "[%s]: running filter: %s", __FUNCTION__, plugin->plugin.name);

	switch (type)
	{
		case FILTER_TYPE_KEYBOARD:
			IFCALLRET(plugin->plugin.KeyboardEvent, result, pdata, param);
			break;

		case FILTER_TYPE_MOUSE:
			IFCALLRET(plugin->plugin.MouseEvent, result, pdata, param);
			break;

		case FILTER_TYPE_CLIENT_PASSTHROUGH_CHANNEL_DATA:
			IFCALLRET(plugin->plugin.ClientChannelData, result, pdata, param);
			break;

		case FILTER_TYPE_SERVER_PASSTHROUGH_CHANNEL_DATA:
			IFCALLRET(plugin->plugin.ServerChannelData, result, pdata, param);
			break;

		case FILTER_TYPE_SERVER_FETCH_TARGET_ADDR:
			IFCALLRET(plugin->plugin.ServerFetchTargetAddr, result, pdata, param);
			break;

		case FILTER_LAST:
		default:
			WLog_ERR(TAG, "invalid filter called");
	}

	if (!result)
	{
		/* current filter return FALSE, no need to run other filters. */
		WLog_DBG(TAG, "plugin %s, filter type [%s] returned FALSE", plugin->plugin.name,
		         pf_modules_get_filter_type_string(type));
	}
	return result;
}

/*
 * runs all filters of type `type`.
 *
 * @type: filter type to run.
 * @server: pointer of server's rdpContext struct of the current session.
 */
BOOL pf_modules_run_filter(proxyModule* module, PF_FILTER_TYPE type, proxyData* pdata, void* param)
{
	WINPR_ASSERT(module);
	WINPR_ASSERT(module->plugins);

	return ArrayList_ForEach(module->plugins, pf_modules_ArrayList_ForEachFkt, type, pdata, param);
}

/*
 * stores per-session data needed by a plugin.
 *
 * @context: current session server's rdpContext instance.
 * @info: pointer to per-session data.
 */
static BOOL pf_modules_set_plugin_data(proxyPluginsManager* mgr, const char* plugin_name,
                                       proxyData* pdata, void* data)
{
	union {
		const char* ccp;
		char* cp;
	} ccharconv;

	WINPR_ASSERT(plugin_name);

	ccharconv.ccp = plugin_name;
	if (data == NULL) /* no need to store anything */
		return FALSE;

	if (!HashTable_Insert(pdata->modules_info, ccharconv.cp, data))
	{
		WLog_ERR(TAG, "[%s]: HashTable_Insert failed!");
		return FALSE;
	}

	return TRUE;
}

/*
 * returns per-session data needed a plugin.
 *
 * @context: current session server's rdpContext instance.
 * if there's no data related to `plugin_name` in `context` (current session), a NULL will be
 * returned.
 */
static void* pf_modules_get_plugin_data(proxyPluginsManager* mgr, const char* plugin_name,
                                        proxyData* pdata)
{
	union {
		const char* ccp;
		char* cp;
	} ccharconv;
	WINPR_ASSERT(plugin_name);
	WINPR_ASSERT(pdata);
	ccharconv.ccp = plugin_name;

	return HashTable_GetItemValue(pdata->modules_info, ccharconv.cp);
}

static void pf_modules_abort_connect(proxyPluginsManager* mgr, proxyData* pdata)
{
	WINPR_ASSERT(pdata);
	WLog_DBG(TAG, "%s is called!", __FUNCTION__);
	proxy_data_abort_connect(pdata);
}

static BOOL pf_modules_register_ArrayList_ForEachFkt(void* data, size_t index, va_list ap)
{
	proxyPluginInternal* plugin = (proxyPluginInternal*)data;
	proxyPlugin* plugin_to_register = va_arg(ap, proxyPlugin*);

	WINPR_UNUSED(index);

	if (strcmp(plugin->plugin.name, plugin_to_register->name) == 0)
	{
		WLog_ERR(TAG, "can not register plugin '%s', it is already registered!",
		         plugin->plugin.name);
		return FALSE;
	}
	return TRUE;
}

static BOOL pf_modules_register_plugin(proxyPluginsManager* mgr,
                                       const proxyPlugin* plugin_to_register, void* userdata)
{
	proxyPluginInternal internal = { 0 };
	proxyModule* module = (proxyModule*)mgr;
	WINPR_ASSERT(module);

	if (!plugin_to_register)
		return FALSE;

	internal.plugin = *plugin_to_register;
	internal.module = module;
	internal.userdata = userdata;
	/* make sure there's no other loaded plugin with the same name of `plugin_to_register`. */
	if (!ArrayList_ForEach(module->plugins, pf_modules_register_ArrayList_ForEachFkt, &internal))
		return FALSE;

	if (!ArrayList_Append(module->plugins, &internal))
	{
		WLog_ERR(TAG, "[%s]: failed adding plugin to list: %s", __FUNCTION__,
		         plugin_to_register->name);
		return FALSE;
	}

	return TRUE;
}

static BOOL pf_modules_load_ArrayList_ForEachFkt(void* data, size_t index, va_list ap)
{
	proxyPluginInternal* plugin = (proxyPluginInternal*)data;
	const char* plugin_name = va_arg(ap, const char*);

	WINPR_UNUSED(index);
	WINPR_UNUSED(ap);

	if (strcmp(plugin->plugin.name, plugin_name) == 0)
		return TRUE;
	return FALSE;
}

BOOL pf_modules_is_plugin_loaded(proxyModule* module, const char* plugin_name)
{
	WINPR_ASSERT(module);
	return ArrayList_ForEach(module->plugins, pf_modules_load_ArrayList_ForEachFkt, plugin_name);
}

static BOOL pf_modules_print_ArrayList_ForEachFkt(void* data, size_t index, va_list ap)
{
	proxyPluginInternal* plugin = (proxyPluginInternal*)data;

	WINPR_UNUSED(index);
	WINPR_UNUSED(ap);

	WLog_INFO(TAG, "\tName: %s", plugin->plugin.name);
	WLog_INFO(TAG, "\tDescription: %s", plugin->plugin.description);
	return TRUE;
}

void pf_modules_list_loaded_plugins(proxyModule* module)
{
	size_t count;

	WINPR_ASSERT(module);
	WINPR_ASSERT(module->plugins);

	count = ArrayList_Count(module->plugins);

	if (count > 0)
		WLog_INFO(TAG, "Loaded plugins:");

	ArrayList_ForEach(module->plugins, pf_modules_print_ArrayList_ForEachFkt);
}

static BOOL pf_modules_load_module(const char* module_path, proxyModule* module, void* userdata)
{
	HMODULE handle = NULL;
	proxyModuleEntryPoint pEntryPoint;
	WINPR_ASSERT(module);

	handle = LoadLibraryA(module_path);

	if (handle == NULL)
	{
		WLog_ERR(TAG, "[%s]: failed loading external library: %s", __FUNCTION__, module_path);
		return FALSE;
	}

	if (!(pEntryPoint = (proxyModuleEntryPoint)GetProcAddress(handle, MODULE_ENTRY_POINT)))
	{
		WLog_ERR(TAG, "[%s]: GetProcAddress failed while loading %s", __FUNCTION__, module_path);
		goto error;
	}
	if (!ArrayList_Append(module->handles, handle))
	{
		WLog_ERR(TAG, "ArrayList_Append failed!");
		goto error;
	}
	return pf_modules_add(module, pEntryPoint, userdata);

error:
	FreeLibrary(handle);
	return FALSE;
}

static void free_handle(void* obj)
{
	HANDLE handle = (HANDLE)obj;
	if (handle)
		FreeLibrary(handle);
}

static void free_plugin(void* obj)
{
	proxyPluginInternal* plugin = (proxyPluginInternal*)obj;
	WINPR_ASSERT(plugin);

	if (!IFCALLRESULT(TRUE, plugin->plugin.PluginUnload, &plugin->plugin, plugin->userdata))
		WLog_WARN(TAG, "PluginUnload failed for plugin '%s'", plugin->plugin.name);
}

proxyModule* pf_modules_new(const char* root_dir, const char** modules, size_t count)
{
	size_t i;
	proxyModule* module = calloc(1, sizeof(proxyModule));
	if (!module)
		return NULL;

	module->mgr.RegisterPlugin = pf_modules_register_plugin;
	module->mgr.SetPluginData = pf_modules_set_plugin_data;
	module->mgr.GetPluginData = pf_modules_get_plugin_data;
	module->mgr.AbortConnect = pf_modules_abort_connect;
	module->plugins = ArrayList_New(FALSE);

	if (module->plugins == NULL)
	{
		WLog_ERR(TAG, "[%s]: ArrayList_New failed!", __FUNCTION__);
		goto error;
	}
	ArrayList_Object(module->plugins)->fnObjectFree = free_plugin;

	module->handles = ArrayList_New(FALSE);
	if (module->handles == NULL)
	{

		WLog_ERR(TAG, "[%s]: ArrayList_New failed!", __FUNCTION__);
		goto error;
	}
	ArrayList_Object(module->handles)->fnObjectFree = free_handle;

	if (count > 0)
	{
		if (!PathFileExistsA(root_dir))
		{
			if (!CreateDirectoryA(root_dir, NULL))
			{
				WLog_ERR(TAG, "error occurred while creating modules directory: %s", root_dir);
				goto error;
			}
		}

		if (PathFileExistsA(root_dir))
			WLog_DBG(TAG, "modules root directory: %s", root_dir);

		for (i = 0; i < count; i++)
		{
			char* fullpath = GetCombinedPath(root_dir, modules[i]);
			pf_modules_load_module(fullpath, module, NULL);
			free(fullpath);
		}
	}

	return module;

error:
	pf_modules_free(module);
	return NULL;
}

void pf_modules_free(proxyModule* module)
{
	if (!module)
		return;

	ArrayList_Free(module->plugins);
	ArrayList_Free(module->handles);
}

BOOL pf_modules_add(proxyModule* module, proxyModuleEntryPoint ep, void* userdata)
{
	WINPR_ASSERT(module);
	WINPR_ASSERT(ep);

	return ep(&module->mgr, userdata);
}
