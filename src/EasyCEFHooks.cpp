#pragma once
#include "pch.h"
#include "EasyCEFHooks.h"

#define CAST_TO(target,to) reinterpret_cast<decltype(&to)>(target)

_cef_frame_t* EasyCEFHooks::frame = NULL;
cef_v8context_t* EasyCEFHooks::contextl = NULL;

struct _cef_client_t* EasyCEFHooks::cef_client = NULL;
PVOID EasyCEFHooks::origin_cef_browser_host_create_browser = NULL;
PVOID EasyCEFHooks::origin_cef_initialize = NULL;
PVOID EasyCEFHooks::origin_cef_get_keyboard_handler = NULL;
PVOID EasyCEFHooks::origin_cef_on_key_event = NULL;
PVOID EasyCEFHooks::origin_cef_v8context_get_current_context = NULL;
PVOID EasyCEFHooks::origin_cef_load_handler = NULL;
PVOID EasyCEFHooks::origin_cef_on_load_start = NULL;
PVOID EasyCEFHooks::origin_on_before_command_line_processing = NULL;
PVOID EasyCEFHooks::origin_command_line_append_switch = NULL;
PVOID EasyCEFHooks::origin_cef_register_scheme_handler_factory = NULL;
PVOID EasyCEFHooks::origin_cef_scheme_handler_create = NULL;

std::function<void(struct _cef_browser_t* browser, struct _cef_frame_t* frame, cef_transition_type_t transition_type)> EasyCEFHooks::onLoadStart = [](auto browser, auto frame, auto transition_type) {};
std::function<void(_cef_client_t*, struct _cef_browser_t*, const struct _cef_key_event_t*)> EasyCEFHooks::onKeyEvent = [](auto client, auto browser, auto key) {};
std::function<bool(string)> EasyCEFHooks::onAddCommandLine = [](string arg) { return true;  };
void EasyCEFHooks::executeJavaScript(_cef_frame_t* frame, string script, string url) {
	CefString exec_script = script;
	CefString purl = url;
	frame->execute_java_script(frame, exec_script.GetStruct(), purl.GetStruct(), 0);
}

cef_v8context_t* EasyCEFHooks::hook_cef_v8context_get_current_context() {
	cef_v8context_t* context = CAST_TO(origin_cef_v8context_get_current_context, hook_cef_v8context_get_current_context)();

	cef_browser_t* browser = context->get_browser(context);
	auto host = browser->get_host(browser);


	contextl = context;
	frame = browser->get_main_frame(browser);

	return context;
}

int CEF_CALLBACK EasyCEFHooks::hook_cef_on_key_event(struct _cef_keyboard_handler_t* self,
	struct _cef_browser_t* browser,
	const struct _cef_key_event_t* event,
	cef_event_handle_t os_event) {
	onKeyEvent(cef_client, browser, event);

	return CAST_TO(origin_cef_on_key_event, hook_cef_on_key_event)(self, browser, event, os_event);
}


struct _cef_keyboard_handler_t* CEF_CALLBACK EasyCEFHooks::hook_cef_get_keyboard_handler(struct _cef_client_t* self) {
	auto keyboard_handler = CAST_TO(origin_cef_get_keyboard_handler, hook_cef_get_keyboard_handler)(self);
	if (keyboard_handler) {
		cef_client = self;
		origin_cef_on_key_event = keyboard_handler->on_key_event;
		keyboard_handler->on_key_event = hook_cef_on_key_event;
	}
	return keyboard_handler;
}



struct _cef_load_handler_t* CEF_CALLBACK EasyCEFHooks::hook_cef_load_handler(struct _cef_client_t* self) {
	auto load_handler = CAST_TO(origin_cef_load_handler, hook_cef_load_handler)(self);
	if (load_handler) {
		cef_client = self;
		origin_cef_on_load_start = load_handler->on_load_start;
		load_handler->on_load_start = hook_cef_on_load_start;
	}
	return load_handler;
}

void CEF_CALLBACK EasyCEFHooks::hook_cef_on_load_start(struct _cef_load_handler_t* self,
	struct _cef_browser_t* browser,
	struct _cef_frame_t* frame,
	cef_transition_type_t transition_type) {
	CAST_TO(origin_cef_on_load_start, hook_cef_on_load_start)(self, browser, frame, transition_type);
	onLoadStart(browser, frame, transition_type);
}

int EasyCEFHooks::hook_cef_browser_host_create_browser(
	const cef_window_info_t* windowInfo,
	struct _cef_client_t* client,
	const cef_string_t* url,
	const struct _cef_browser_settings_t* settings,
	struct _cef_request_context_t* request_context) {


	origin_cef_get_keyboard_handler = client->get_keyboard_handler;
	client->get_keyboard_handler = hook_cef_get_keyboard_handler;

	origin_cef_load_handler = client->get_load_handler;
	client->get_load_handler = hook_cef_load_handler;

	int origin = CAST_TO(origin_cef_browser_host_create_browser, hook_cef_browser_host_create_browser)
		(windowInfo, client, url, settings, request_context);
	return origin;
}

int EasyCEFHooks::hook_cef_initialize(const struct _cef_main_args_t* args,
	const struct _cef_settings_t* settings,
	cef_app_t* application,
	void* windows_sandbox_info) {

	_cef_settings_t s = *settings;
	s.background_color = 0x000000ff;

	origin_on_before_command_line_processing = application->on_before_command_line_processing;
	application->on_before_command_line_processing = hook_on_before_command_line_processing;

	s.command_line_args_disabled = true;

	return CAST_TO(origin_cef_initialize, hook_cef_initialize)(args, &s, application, windows_sandbox_info);
}

void CEF_CALLBACK EasyCEFHooks::hook_on_before_command_line_processing(
	struct _cef_app_t* self,
	const cef_string_t* process_type,
	struct _cef_command_line_t* command_line) {

	CefString str = "--disable-web-security";
	command_line->append_switch(command_line, str.GetStruct());

	origin_command_line_append_switch = command_line->append_switch;
	command_line->append_switch = hook_command_line_append_switch;
	CAST_TO(origin_on_before_command_line_processing, hook_on_before_command_line_processing)(self, process_type, command_line);
}

void CEF_CALLBACK EasyCEFHooks::hook_command_line_append_switch(_cef_command_line_t* self, const cef_string_t* name) {
	if (onAddCommandLine(CefString(name).ToString())) {
		CAST_TO(origin_command_line_append_switch, hook_command_line_append_switch)(self, name);
	}
}

bool EasyCEFHooks::InstallHooks() {
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	origin_cef_v8context_get_current_context = DetourFindFunction("libcef.dll", "cef_v8context_get_current_context");
	origin_cef_browser_host_create_browser = DetourFindFunction("libcef.dll", "cef_browser_host_create_browser_sync");
	origin_cef_initialize = DetourFindFunction("libcef.dll", "cef_initialize");
	origin_cef_register_scheme_handler_factory = DetourFindFunction("libcef.dll", "cef_register_scheme_handler_factory");

	if (origin_cef_v8context_get_current_context)
		DetourAttach(&origin_cef_v8context_get_current_context, (PVOID)hook_cef_v8context_get_current_context);
	else
		return false;

	if (origin_cef_browser_host_create_browser)
		DetourAttach(&origin_cef_browser_host_create_browser, hook_cef_browser_host_create_browser);
	else
		return false;

	if (origin_cef_initialize)
		DetourAttach(&origin_cef_initialize, hook_cef_initialize);
	else
		return false;

	if (origin_cef_register_scheme_handler_factory)
		DetourAttach(&origin_cef_register_scheme_handler_factory, hook_cef_register_scheme_handler_factory);
	else
		return false;

	LONG ret = DetourTransactionCommit();
	return ret == NO_ERROR;
}

PVOID origin_read;

PVOID origin_get_headers;

void CEF_CALLBACK EasyCEFHooks::get_response_headers(struct _cef_resource_handler_t* self,
	struct _cef_response_t* response,
	int64* response_length,
	cef_string_t* redirectUrl) {
	*response_length = -1;
	CAST_TO(origin_get_headers, get_response_headers)(self, response, response_length, redirectUrl);
	*response_length = -1;
	//CefString name = "";

	//response->set_header_by_name(self, )
}

map<_cef_resource_handler_t*, string> urlMap;

int CEF_CALLBACK EasyCEFHooks::read(struct _cef_resource_handler_t* self,
	void* data_out,
	int bytes_to_read,
	int* bytes_read,
	struct _cef_callback_t* callback) {
	//
	int ret = 0;
	if (urlMap[self].ends_with(".js")) {
		if (!urlMap.contains(self)) {
			*bytes_read = 0;
			return 0;
		}
		cout << urlMap[self] << " || \n";
		urlMap.erase(self);
		
		
		// do stuff

		//*((char*)data_out) = '8';
		//((char*)data_out) = *s.c_str();
		//bytes_to_read = s->length();
		ret = CAST_TO(origin_read, read)(self, data_out ,bytes_to_read, bytes_read, callback);

		//
		auto s = string((char*)data_out, (size_t)*bytes_read);
		cout << s;
		s="console.log('codeeeee modifaction!!!!!');";
		char* cstr = new char[s.length() + 1];

		strcpy((char*)data_out, s.c_str());
		*bytes_read = s.length();
		//callback->cont(callback);
		return 1;
	}
	else {
		ret = CAST_TO(origin_read, read)(self, data_out, bytes_to_read, bytes_read, callback);
	}
	
	
	try {
	/*	
		if (s.starts_with("<!doctype html>")) {

		}*/
	}catch(PVOID e){}

	return ret;
}

_cef_resource_handler_t* CEF_CALLBACK EasyCEFHooks::hook_cef_scheme_handler_create(
	struct _cef_scheme_handler_factory_t* self,
	struct _cef_browser_t* browser,
	struct _cef_frame_t* frame,
	const cef_string_t* scheme_name,
	struct _cef_request_t* request) {
	_cef_resource_handler_t* ret = CAST_TO(origin_cef_scheme_handler_create, hook_cef_scheme_handler_create)(self, browser, frame, scheme_name, request);
	// scheme_name;
	//alert(ret->read);
	//origin_get_headers = ret->get_response_headers;
	CefString url = request->get_url(request);
	urlMap[ret] = url.ToString();
	origin_get_headers = ret->get_response_headers;
	ret->get_response_headers = get_response_headers;
	origin_read = ret->read_response;
	ret->read_response = read;


	return ret;
}

int EasyCEFHooks::hook_cef_register_scheme_handler_factory(
	const cef_string_t* scheme_name,
	const cef_string_t* domain_name,
	cef_scheme_handler_factory_t* factory) {

	origin_cef_scheme_handler_create = factory->create;
	factory->create = hook_cef_scheme_handler_create;

	CefString a = scheme_name;
	//alert(a.ToString());

	int ret = CAST_TO(origin_cef_register_scheme_handler_factory, hook_cef_register_scheme_handler_factory)(scheme_name, domain_name, factory);
	return ret;
}

bool EasyCEFHooks::UninstallHook()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&origin_cef_v8context_get_current_context, hook_cef_v8context_get_current_context);
	DetourDetach(&origin_cef_browser_host_create_browser, hook_cef_browser_host_create_browser);
	LONG ret = DetourTransactionCommit();
	return ret == NO_ERROR;
}

