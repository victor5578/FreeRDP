/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * FreeRDP Proxy Server Demo C++ Module
 *
 * Copyright 2019 Kobi Mizrachi <kmizrachi18@gmail.com>
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

#include <iostream>

#include <freerdp/server/proxy/proxy_modules_api.h>

#define TAG MODULE_TAG("demo")

static constexpr char plugin_name[] = "demo";
static constexpr char plugin_desc[] = "this is a test plugin";

static BOOL demo_filter_keyboard_event(proxyPlugin* plugin, proxyData* pdata, void* param)
{
	proxyPluginsManager* mgr;
	auto event_data = static_cast<proxyKeyboardEventInfo*>(param);

	WINPR_ASSERT(plugin);
	WINPR_ASSERT(pdata);

	mgr = plugin->mgr;
	WINPR_ASSERT(mgr);

	if (event_data == NULL)
		return FALSE;

	if (event_data->rdp_scan_code == RDP_SCANCODE_KEY_B)
	{
		/* user typed 'B', that means bye :) */
		std::cout << "C++ demo plugin: aborting connection" << std::endl;
		mgr->AbortConnect(mgr, pdata);
	}

	return TRUE;
}

static BOOL demo_plugin_unload(proxyPlugin* plugin)
{
	std::cout << "C++ demo plugin: unloading..." << std::endl;

	/* Here we have to free up our custom data storage. */
	if (plugin)
		free(plugin->custom);

	return TRUE;
}

BOOL proxy_module_entry_point(proxyPluginsManager* plugins_manager, void* userdata)
{
	struct demo_custom_data
	{
		proxyPluginsManager* mgr;
		int somesetting;
	};
	struct demo_custom_data* custom;

	proxyPlugin demo_plugin = { plugin_name,                /* name */
		                        plugin_desc,                /* description */
		                        demo_plugin_unload,         /* PluginUnload */
		                        NULL,                       /* ClientPreConnect */
		                        NULL,                       /* ClientPostConnect */
		                        NULL,                       /* ClientLoginFailure */
		                        NULL,                       /* ClientEndPaint */
		                        NULL,                       /* ServerPostConnect */
		                        NULL,                       /* ServerChannelsInit */
		                        NULL,                       /* ServerChannelsFree */
		                        NULL,                       /* ServerSessionEnd */
		                        demo_filter_keyboard_event, /* KeyboardEvent */
		                        NULL,                       /* MouseEvent */
		                        NULL,                       /* ClientChannelData */
		                        NULL,                       /* ServerChannelData */
		                        NULL,                       /* ServerFetchTargetAddr */
		                        NULL,
		                        userdata,
		                        NULL };

	custom = (struct demo_custom_data*)calloc(1, sizeof(struct demo_custom_data));
	if (!custom)
		return FALSE;

	custom->mgr = plugins_manager;
	custom->somesetting = 42;

	demo_plugin.custom = custom;

	return plugins_manager->RegisterPlugin(plugins_manager, &demo_plugin);
}
