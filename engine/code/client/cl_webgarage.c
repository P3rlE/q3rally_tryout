#include "../game/garage_unlocks.h"
/* Optional Windows x64 CEF garage pilot. The web document is bundled locally. */
#include "client.h"
#include "../qcommon/json.h"

#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#ifdef USE_LOCAL_HEADERS
#include "../SDL2-2.32.8/include/SDL.h"
#include "../SDL2-2.32.8/include/SDL_syswm.h"
#else
#include <SDL.h>
#include <SDL_syswm.h>
#endif
#include "include/capi/cef_app_capi.h"
#include "include/capi/cef_client_capi.h"
#include "include/capi/cef_scheme_capi.h"

static cef_browser_t *webBrowser;
static HWND webParent, webChild;
static qboolean webInitialized, webContextReady, webOpenRequested, webCloseRequested;
#define WEB_ORIGIN L"https://q3rally.invalid/"
static const wchar_t webPage[] = WEB_ORIGIN L"index.html";
static wchar_t webRoot[MAX_PATH];
#define WEB_ASSET_ID 49
static struct {
	char vehicle[WEB_ASSET_ID];
	char paint[WEB_ASSET_ID];
	char wheels[WEB_ASSET_ID];
    char head[WEB_ASSET_ID];
    char plate[WEB_ASSET_ID];
    int slot;
	qboolean pending;
} webSetup;
static cvar_t *webRenderBehind;
static cvar_t *webTiming;
static cvar_t *webFrontend;
static qboolean webGarageMode;
static int webMenuAction;
static int webDriverAction = -1;
static qboolean webDriverPublished;
static int webOfflineScreen;
static struct { int action, index, value; } webOfflineQueue[64];
static int webOfflineRead, webOfflineWrite;
static struct { int action, index, value; char text[256]; } webOnlineQueue[64];
static int webOnlineRead, webOnlineWrite, webLastOnlinePoll, webOnlineRevision;
static qboolean webOnlinePublished;
static struct { int action, index, value; char text[128]; } webConfigQueue[64];
static int webConfigRead, webConfigWrite;
static qboolean webConfigPublished;
static int webConfigRevision;
static qboolean webHUDPublished, webHUDRectValid;
static RECT webHUDRect, webHUDPreviousRect;
static qboolean webHUDRegion;
static int webHUDAction = -1, webHUDIndex, webHUDValue, webSessionAction;
static int webLastState = -1;
static int webLastReport;
static int webDemoAction = -1, webDemoIndex;
static RECT webDemoRect;
static qboolean webDemoRectValid, webDemoRegion, webDemoRectChanged;
static int webItemsScreen;
static int webCatalogScreen, webCatalogState = -1, webCatalogRevision = -1;
static int webCatalogAction = -1, webCatalogIndex, webCatalogActionScreen;
static int webLastStatusReport, webLastDownloadPoll;
static wchar_t webPreviewRoot[MAX_PATH];

#define WEB_TIMING_BUCKETS 101
static struct {
	LARGE_INTEGER frequency;
	LARGE_INTEGER previous;
	double frameTotal;
	double pumpTotal;
	unsigned frames;
	unsigned skipped;
	unsigned buckets[WEB_TIMING_BUCKETS];
} webStats;

void CL_WebGarageInit( void ) {
	webRenderBehind = Cvar_Get( "cl_webGarageRenderBehind", "0", 0 );
	webTiming = Cvar_Get( "cl_webGarageTiming", "0", 0 );
	webFrontend = Cvar_Get( "cl_webUI", "1", CVAR_ARCHIVE );
	Cvar_Get( "cl_webDemoRect", "", CVAR_ROM );
    Cvar_Get( "cl_webHUDActive", "0", CVAR_ROM );
    Cvar_Get( "cl_webMapLoading", "0", CVAR_ROM );
    Cvar_Get( "cl_webIngameUI", "0", CVAR_ROM );
}

static qboolean Web_Visible( void ) {
	return webChild && !(Key_GetCatcher() & KEYCATCH_CONSOLE) && (clc.state == CA_DISCONNECTED || clc.demoPlayer || (clc.state >= CA_CONNECTING && clc.state <= CA_ACTIVE)) && IsWindowVisible( webChild ) &&
	       webParent && !IsIconic( webParent );
}

qboolean CL_WebGarageInputVisible( void ) { return Web_Visible(); }

qboolean CL_WebGarageDemoVisible( void ) {
    return Web_Visible() && clc.demoPlayer;
}

qboolean CL_WebGarageNativeInput( void ) {
    return webInitialized && webFrontend && webFrontend->integer && !webBrowser;
}

void CL_WebGarageFocus( void ) {
    if ( webChild && IsWindow( webChild ) ) SetFocus( webChild );
}

qboolean CL_WebGarageSkipBackground( void ) {
	if ( !webRenderBehind || webRenderBehind->integer || !Web_Visible() || clc.demoPlayer || clc.state == CA_ACTIVE ) return qfalse;
	if ( webTiming && webTiming->integer ) webStats.skipped++;
	return qtrue;
}

static void Web_RecordTiming( LARGE_INTEGER start, LARGE_INTEGER end ) {
	double elapsed;
	unsigned bucket;
	if ( !webStats.frequency.QuadPart || !webStats.previous.QuadPart ) {
		webStats.previous = start;
		return;
	}
	elapsed = 1000.0 * (start.QuadPart - webStats.previous.QuadPart) / webStats.frequency.QuadPart;
	webStats.previous = start;
	bucket = elapsed >= WEB_TIMING_BUCKETS - 1 ? WEB_TIMING_BUCKETS - 1 : (unsigned)elapsed;
	webStats.buckets[bucket]++;
	webStats.frameTotal += elapsed;
	webStats.pumpTotal += 1000.0 * (end.QuadPart - start.QuadPart) / webStats.frequency.QuadPart;
	webStats.frames++;
}

static void Web_PrintTiming( void ) {
	unsigned i, count = 0;
	if ( !webStats.frames ) return;
	for ( i = 0; i < WEB_TIMING_BUCKETS - 1; i++ ) {
		count += webStats.buckets[i];
		if ( count >= webStats.frames - webStats.frames / 20 ) break;
	}
	Com_Printf( "Web garage timing: %u frames, %u screen updates skipped, engine %.2f ms avg / %u%s ms p95 bucket, CEF pump %.2f ms avg.\n",
	            webStats.frames, webStats.skipped, webStats.frameTotal / webStats.frames,
	            i, i == WEB_TIMING_BUCKETS - 1 ? "+" : "", webStats.pumpTotal / webStats.frames );
}

typedef struct {
	const char *id;
	int rank;
} webUnlock_t;

#define WEB_UNLOCK_ENTRY( id, name, rank ) { id, rank },
static const webUnlock_t webCarUnlocks[] = { GARAGE_CAR_UNLOCKS( WEB_UNLOCK_ENTRY ) };
static const webUnlock_t webSkinUnlocks[] = { GARAGE_SKIN_UNLOCKS( WEB_UNLOCK_ENTRY ) };
#undef WEB_UNLOCK_ENTRY

static int Web_UnlockedRank( void ) {
	return uivm ? VM_Call( uivm, UI_GARAGE_UNLOCKED_RANK ) : -1;
}

static qboolean Web_AssetUnlocked( const webUnlock_t *entries, size_t count, const char *id, int rank ) {
	size_t i;
	for ( i = 0; i < count; i++ ) {
		if ( !strcmp( entries[i].id, id ) ) return rank >= entries[i].rank;
	}
	return qtrue;
}

static qboolean Web_AssetId( const char *id ) {
	return id[0] && strlen( id ) < WEB_ASSET_ID &&
	       id[strspn( id, "abcdefghijklmnopqrstuvwxyz0123456789_-" )] == '\0';
}

static const wchar_t *Web_ParseAssetId( const wchar_t *value, char out[WEB_ASSET_ID] ) {
	size_t i, count = wcsspn( value, L"abcdefghijklmnopqrstuvwxyz0123456789_-" );
	if ( !count || count >= WEB_ASSET_ID ) return NULL;
	for ( i = 0; i < count; i++ ) out[i] = (char)value[i];
	out[count] = 0;
	return value + count;
}

static qboolean Web_SetupInCatalog( void ) {
	wchar_t path[MAX_PATH];
	FILE *file;
	static char json[131072];
	const char *end, *vehicles, *vehicle, *paints, *wheels, *heads, *plates;
	size_t size;
	qboolean readOk;

	if ( swprintf( path, ARRAY_LEN( path ), L"%ls/catalog.json", webRoot ) < 0 ) return qfalse;
	file = _wfopen( path, L"rb" );
	if ( !file ) return qfalse;
	size = fread( json, 1, sizeof( json ) - 1, file );
	readOk = feof( file ) && !ferror( file );
	fclose( file );
	if ( !readOk || !size ) return qfalse;
	json[size] = 0;
	end = json + size;
	vehicles = JSON_ObjectGetNamedValue( json, end, "vehicles" );
	wheels = JSON_ObjectGetNamedValue( json, end, "wheels" );
	heads = JSON_ObjectGetNamedValue( json, end, "heads" );
    plates = JSON_ObjectGetNamedValue( json, end, "plates" );
    if ( !vehicles || !wheels || !heads || !plates ) return qfalse;
	vehicle = JSON_ObjectGetNamedValue( vehicles, end, webSetup.vehicle );
	if ( !vehicle ) return qfalse;
	paints = JSON_ObjectGetNamedValue( vehicle, end, "paints" );
	return paints && JSON_ObjectGetNamedValue( paints, end, webSetup.paint ) &&
	       JSON_ObjectGetNamedValue( wheels, end, webSetup.wheels ) &&
           JSON_ObjectGetNamedValue( heads, end, webSetup.head ) &&
           JSON_ObjectGetNamedValue( plates, end, webSetup.plate );
}

static void CEF_CALLBACK Web_AddRef( cef_base_ref_counted_t *self ) {}
static int CEF_CALLBACK Web_Release( cef_base_ref_counted_t *self ) { return 0; }
static int CEF_CALLBACK Web_OneRef( cef_base_ref_counted_t *self ) { return 0; }
static int CEF_CALLBACK Web_AnyRefs( cef_base_ref_counted_t *self ) { return 1; }

static void Web_StaticHandler( cef_base_ref_counted_t *base, size_t size ) {
	/* These handlers have process lifetime; CEF never owns their storage. */
	base->size = size;
	base->add_ref = Web_AddRef;
	base->release = Web_Release;
	base->has_one_ref = Web_OneRef;
	base->has_at_least_one_ref = Web_AnyRefs;
}

static qboolean Web_SetString( cef_string_t *out, const wchar_t *value ) {
	return cef_string_utf16_set( (const char16_t *)value, wcslen( value ), out, 1 ) != 0;
}

static qboolean Web_Equals( const cef_string_t *value, const wchar_t *expected ) {
	size_t length = wcslen( expected );
	return value && value->length == length &&
	       !memcmp( value->str, expected, length * sizeof( wchar_t ) );
}

typedef struct {
	cef_resource_handler_t handler;
	LONG references;
	FILE *file;
	long length;
	const wchar_t *mime;
	qboolean preview;
	wchar_t path[MAX_PATH];
} webResource_t;

static void CEF_CALLBACK Web_ResourceAddRef( cef_base_ref_counted_t *base ) {
	InterlockedIncrement( &((webResource_t *)base)->references );
}

static int CEF_CALLBACK Web_ResourceRelease( cef_base_ref_counted_t *base ) {
	webResource_t *resource = (webResource_t *)base;
	if ( InterlockedDecrement( &resource->references ) ) return 0;
	if ( resource->file ) fclose( resource->file );
	free( resource );
	return 1;
}

static int CEF_CALLBACK Web_ResourceOneRef( cef_base_ref_counted_t *base ) {
	return InterlockedCompareExchange( &((webResource_t *)base)->references, 0, 0 ) == 1;
}

static int CEF_CALLBACK Web_ResourceAnyRefs( cef_base_ref_counted_t *base ) {
	return InterlockedCompareExchange( &((webResource_t *)base)->references, 0, 0 ) > 0;
}

static int CEF_CALLBACK Web_ResourceOpen( cef_resource_handler_t *self, cef_request_t *request,
	int *handle, cef_callback_t *callback ) {
	webResource_t *resource = (webResource_t *)self;
	*handle = 1;
	if ( resource->path[0] ) resource->file = _wfopen( resource->path, L"rb" );
	if ( resource->file && ( fseek( resource->file, 0, SEEK_END ) ||
	     (resource->length = ftell( resource->file )) < 0 || resource->length > 32 * 1024 * 1024 ||
	     fseek( resource->file, 0, SEEK_SET ) ) ) {
		fclose( resource->file );
		resource->file = NULL;
	}
	return 1;
}

static void CEF_CALLBACK Web_ResourceHeaders( cef_resource_handler_t *self, cef_response_t *response,
	int64_t *length, cef_string_t *redirect ) {
	webResource_t *resource = (webResource_t *)self;
    if ( resource->preview ) {
        cef_string_t name = { 0 }, value = { 0 };
        Web_SetString( &name, L"Cache-Control" );
        Web_SetString( &value, L"no-store" );
        response->set_header_by_name( response, &name, &value, 1 );
        cef_string_utf16_clear( &name );
        cef_string_utf16_clear( &value );
    }
	cef_string_t mime = { 0 };
	response->set_status( response, resource->file ? 200 : 404 );
	*length = resource->file ? resource->length : 0;
	if ( resource->file && Web_SetString( &mime, resource->mime ) ) {
		response->set_mime_type( response, &mime );
		cef_string_utf16_clear( &mime );
	}
}

static int CEF_CALLBACK Web_ResourceRead( cef_resource_handler_t *self, void *out, int size,
	int *read, cef_resource_read_callback_t *callback ) {
	webResource_t *resource = (webResource_t *)self;
	*read = resource->file ? (int)fread( out, 1, size, resource->file ) : 0;
	return *read > 0;
}

static const wchar_t *Web_ResourceMime( const wchar_t *path ) {
	const wchar_t *ext = wcsrchr( path, L'.' );
	if ( !ext ) return NULL;
	if ( !wcscmp( ext, L".html" ) ) return L"text/html";
	if ( !wcscmp( ext, L".css" ) ) return L"text/css";
	if ( !wcscmp( ext, L".js" ) ) return L"text/javascript";
	if ( !wcscmp( ext, L".json" ) ) return L"application/json";
	if ( !wcscmp( ext, L".jpg" ) || !wcscmp( ext, L".jpeg" ) ) return L"image/jpeg";
	if ( !wcscmp( ext, L".gif" ) ) return L"image/gif";
	if ( !wcscmp( ext, L".png" ) ) return L"image/png";
	if ( !wcscmp( ext, L".webp" ) ) return L"image/webp";
	if ( !wcscmp( ext, L".glb" ) ) return L"model/gltf-binary";
	if ( !wcscmp( ext, L".ttf" ) ) return L"font/ttf";
	return NULL;
}

static cef_resource_handler_t *CEF_CALLBACK Web_CreateResource( cef_scheme_handler_factory_t *self,
	cef_browser_t *browser, cef_frame_t *frame, const cef_string_t *scheme, cef_request_t *request ) {
	webResource_t *resource = calloc( 1, sizeof( *resource ) );
	cef_string_userfree_t url, method;
	const wchar_t *path;
	size_t prefix = ARRAY_LEN( WEB_ORIGIN ) - 1;

	if ( !resource ) return NULL;
	resource->references = 1;
	resource->handler.base.size = sizeof( resource->handler );
	resource->handler.base.add_ref = Web_ResourceAddRef;
	resource->handler.base.release = Web_ResourceRelease;
	resource->handler.base.has_one_ref = Web_ResourceOneRef;
	resource->handler.base.has_at_least_one_ref = Web_ResourceAnyRefs;
	resource->handler.open = Web_ResourceOpen;
	resource->handler.get_response_headers = Web_ResourceHeaders;
	resource->handler.read = Web_ResourceRead;

	url = request->get_url( request );
	method = request->get_method( request );
	if ( Web_Equals( method, L"GET" ) && url && url->length > prefix &&
	     !wcsncmp( (const wchar_t *)url->str, WEB_ORIGIN, prefix ) ) {
		path = (const wchar_t *)url->str + prefix;
		/* Only serve safe paths beneath the bundled web root. */
		if ( *path != L'/' && !wcsstr( path, L".." ) &&
		     wcsspn( path, L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-/." ) == wcslen( path ) ) {
			resource->mime = Web_ResourceMime( path );
            if ( resource->mime ) {
                const wchar_t *root = webRoot;
                if ( !wcsncmp( path, L"download-preview/", 17 ) &&
                     (!wcscmp( resource->mime, L"image/jpeg" ) || !wcscmp( resource->mime, L"image/png" ) ||
                      !wcscmp( resource->mime, L"image/webp" ) || !wcscmp( resource->mime, L"image/gif" )) ) {
                    resource->preview = qtrue;
                    root = webPreviewRoot;
                    path += 17;
                }
                if ( swprintf( resource->path, ARRAY_LEN( resource->path ), L"%ls/%ls", root, path ) < 0 )
                    resource->path[0] = 0;
            }
		}
	}
	if ( url ) cef_string_userfree_utf16_free( url );
	if ( method ) cef_string_userfree_utf16_free( method );
	return &resource->handler;
}

static cef_scheme_handler_factory_t webFactory;

static qboolean Web_IsLocalFrame( cef_frame_t *frame ) {
	cef_string_userfree_t url = frame->get_url( frame );
	qboolean result = Web_Equals( url, webPage );
	if ( url ) cef_string_userfree_utf16_free( url );
	return result;
}

static void Web_RunScript( cef_frame_t *frame, const wchar_t *script ) {
	cef_string_t code = { 0 };
	if ( Web_SetString( &code, script ) ) {
		frame->execute_java_script( frame, &code, NULL, 0 );
		cef_string_utf16_clear( &code );
	}
}

/* Strict percent decoding; payloads never enter the console command parser. */
static int Web_Hex( wchar_t c ) {
    if ( c >= L'0' && c <= L'9' ) return c - L'0';
    if ( c >= L'a' && c <= L'f' ) return c - L'a' + 10;
    if ( c >= L'A' && c <= L'F' ) return c - L'A' + 10;
    return -1;
}
static qboolean Web_DecodeText( const wchar_t *source, char *text, size_t capacity ) {
    int count = 0, a, b, value;
    while ( *source ) {
        if ( count >= capacity - 1 ) return qfalse;
        if ( *source == L'%' ) {
            if ( !source[1] || !source[2] ) return qfalse;
            a = Web_Hex( source[1] ); b = Web_Hex( source[2] );
            if ( a < 0 || b < 0 ) return qfalse;
            value = a * 16 + b; source += 3;
        } else { value = *source++; if ( value > 127 ) return qfalse; }
        if ( value < 32 || value == 127 ) return qfalse;
        text[count++] = value;
    }
    text[count] = 0;
    if ( !count ) return qfalse;
    return qtrue;
}
static qboolean Web_DriverPayload( const wchar_t *source ) {
    char text[1024];
    if ( !Web_DecodeText( source, text, sizeof( text ) ) ) return qfalse;
    Cvar_Set( "ui_webdriver_payload", text ); return qtrue;
}

static int Web_CurrentScreen( void ) {
    if ( !(Key_GetCatcher() & KEYCATCH_CONSOLE) && clc.state >= CA_CONNECTING && clc.state <= CA_PRIMED && !clc.demoPlayer ) return 14;
    if ( Key_GetCatcher() & KEYCATCH_CONSOLE ) return 0;
    if ( clc.state == CA_ACTIVE && !clc.demoPlayer ) {
        if ( cgvm && Cvar_VariableIntegerValue( "cg_hudOptionsOpen" ) && (Key_GetCatcher() & KEYCATCH_CGAME) && !(Key_GetCatcher() & KEYCATCH_UI) ) return 13;
        return (Key_GetCatcher() & KEYCATCH_UI) && uivm ? (int)VM_Call( uivm, UI_WEB_SCREEN ) : 0;
    }
    return (clc.state == CA_DISCONNECTED || clc.demoPlayer) && uivm ? (int)VM_Call( uivm, UI_WEB_SCREEN ) : 0;
}

static int CEF_CALLBACK Web_BeforeBrowse( cef_request_handler_t *self,
	cef_browser_t *browser, cef_frame_t *frame, cef_request_t *request,
	int userGesture, int redirect ) {
	cef_string_userfree_t url = request->get_url( request );
	qboolean allow = frame->is_main( frame ) && Web_Equals( url, webPage );
	static const wchar_t action[] = L"q3rally-ui://menu/";
	static const wchar_t apply[] = L"q3rally-ui://garage/apply/";
    static const wchar_t save[] = L"q3rally-ui://garage/save/";
    static const wchar_t sync[] = L"q3rally-ui://garage/sync/";
    static const wchar_t upload[] = L"q3rally-ui://garage/upload/";
	const wchar_t *path;

    if ( !allow && !redirect && frame->is_main( frame ) && Web_IsLocalFrame( frame ) && url ) {
        const wchar_t *uri = (const wchar_t *)url->str;
        if ( userGesture && Web_Equals( url, L"q3rally-ui://session/console" ) ) webSessionAction = 1;
        if ( Web_CurrentScreen() == 14 && userGesture && Web_Equals( url, L"q3rally-ui://loading/cancel" ) ) webSessionAction = 3;
        if ( Web_CurrentScreen() == 13 ) {
            int a, i, v, x, y, w, h, vw, vh; wchar_t tail;
            if ( userGesture && Web_Equals( url, L"q3rally-ui://hud/close" ) ) webSessionAction = 2;
            if ( userGesture && swscanf( uri, L"q3rally-ui://hud/%d/%d/%d%c", &a, &i, &v, &tail ) == 3 && a >= 0 && a <= 2 && i >= 0 && i < 128 && v >= 0 && v <= 200 ) {
                webHUDAction = a; webHUDIndex = i; webHUDValue = v;
            }
            if ( swscanf( uri, L"q3rally-ui://hud/viewport/%d/%d/%d/%d/%d/%d%c", &x, &y, &w, &h, &vw, &vh, &tail ) == 6 && x >= 0 && y >= 0 && w >= 0 && h >= 0 && vw > 0 && vh > 0 && vw <= 32768 && vh <= 32768 && x <= vw && y <= vh && w <= vw-x && h <= vh-y ) {
                RECT parent; GetClientRect( webParent, &parent );
                webHUDRect.left = (int)((double)x * parent.right / vw); webHUDRect.top = (int)((double)y * parent.bottom / vh);
                webHUDRect.right = (int)((double)(x+w) * parent.right / vw); webHUDRect.bottom = (int)((double)(y+h) * parent.bottom / vh); webHUDRectValid = qtrue;
            }
        }
    }
    if ( !allow && !redirect && userGesture && frame->is_main( frame ) && Web_IsLocalFrame( frame ) && uivm && url && VM_Call( uivm, UI_WEB_SCREEN ) == 12 && webConfigWrite - webConfigRead < ARRAY_LEN( webConfigQueue ) ) {
        const wchar_t *uri = (const wchar_t *)url->str;
        int action, index, value, consumed = 0, slot = webConfigWrite % ARRAY_LEN( webConfigQueue ); wchar_t tail;
        if ( swscanf( uri, L"q3rally-ui://config/%d/%d/%d%c", &action, &index, &value, &tail ) == 3 && action >= 0 && action <= 6 && index >= 0 && index < 512 && value >= 0 && value <= 100000 ) {
            webConfigQueue[slot].action = action; webConfigQueue[slot].index = index; webConfigQueue[slot].value = value; webConfigQueue[slot].text[0] = 0; webConfigWrite++;
        } else if ( swscanf( uri, L"q3rally-ui://config/key/%d/%n", &index, &consumed ) == 1 && consumed > 0 && index >= 0 && index < 512 && Web_DecodeText( uri + consumed, webConfigQueue[slot].text, sizeof( webConfigQueue[slot].text ) ) ) {
            value = Key_StringToKeynum( webConfigQueue[slot].text );
            if ( value >= 0 && value < MAX_KEYS ) { webConfigQueue[slot].action = 3; webConfigQueue[slot].index = index; webConfigQueue[slot].value = value; webConfigWrite++; }
        } else {
            consumed = 0;
            if ( swscanf( uri, L"q3rally-ui://config/text/%d/%n", &index, &consumed ) == 1 && consumed > 0 && index >= 0 && index < 512 ) {
                webConfigQueue[slot].text[0] = 0;
                if ( !uri[consumed] || Web_DecodeText( uri + consumed, webConfigQueue[slot].text, sizeof( webConfigQueue[slot].text ) ) ) {
                    webConfigQueue[slot].action = 4; webConfigQueue[slot].index = index; webConfigQueue[slot].value = 0; webConfigWrite++;
                }
            }
        }
    }
    if ( !allow && !redirect && userGesture && frame->is_main( frame ) && Web_IsLocalFrame( frame ) && uivm && url ) {
        int screen = VM_Call( uivm, UI_WEB_SCREEN ), action, index, value, consumed = 0;
        const wchar_t *uri = (const wchar_t *)url->str;
        wchar_t tail;
        if ( screen == 11 && webOnlineWrite - webOnlineRead < ARRAY_LEN( webOnlineQueue ) ) {
            int slot = webOnlineWrite % ARRAY_LEN( webOnlineQueue );
            if ( swscanf( uri, L"q3rally-ui://online/%d/%d/%d%c", &action, &index, &value, &tail ) == 3 &&
                 action >= 0 && action <= 12 && index >= 0 && index < 4096 && value >= 0 && value <= 99999 ) {
                webOnlineQueue[slot].action = action; webOnlineQueue[slot].index = index; webOnlineQueue[slot].value = value; webOnlineQueue[slot].text[0] = 0; webOnlineWrite++;
            } else if ( swscanf( uri, L"q3rally-ui://online/address/%d/%n", &action, &consumed ) == 1 && consumed > 0 && action >= 5 && action <= 9 &&
                        Web_DecodeText( uri + consumed, webOnlineQueue[slot].text, sizeof( webOnlineQueue[slot].text ) ) ) {
                webOnlineQueue[slot].action = action; webOnlineQueue[slot].index = webOnlineQueue[slot].value = 0; webOnlineWrite++;
            }
        }
        if ( screen == 10 && swscanf( uri, L"q3rally-ui://offline/text/%d/%n", &index, &consumed ) == 1 && consumed > 0 && index >= 0 && index < 4096 &&
             webOnlineWrite - webOnlineRead < ARRAY_LEN( webOnlineQueue ) ) {
            int slot = webOnlineWrite % ARRAY_LEN( webOnlineQueue );
            if ( Web_DecodeText( uri + consumed, webOnlineQueue[slot].text, sizeof( webOnlineQueue[slot].text ) ) ) {
                webOnlineQueue[slot].action = 100; webOnlineQueue[slot].index = index; webOnlineQueue[slot].value = 0; webOnlineWrite++;
            }
        }
    }
    if ( !allow && !redirect && userGesture && frame->is_main( frame ) && Web_IsLocalFrame( frame ) && uivm && url ) {
        int screen = VM_Call( uivm, UI_WEB_SCREEN ), action, index, value;
        wchar_t tail;
        if ( (screen == 9 || screen == 10) &&
             swscanf( (const wchar_t *)url->str, L"q3rally-ui://offline/%d/%d/%d%c", &action, &index, &value, &tail ) == 3 &&
             action >= 0 && action <= 6 && index >= 0 && index < 4096 && value >= 0 && value <= 99999 && webOfflineWrite - webOfflineRead < ARRAY_LEN( webOfflineQueue ) ) {
            int slot = webOfflineWrite++ % ARRAY_LEN( webOfflineQueue );
            webOfflineQueue[slot].action = action; webOfflineQueue[slot].index = index; webOfflineQueue[slot].value = value;
        }
    }
    if ( !allow && !redirect && frame->is_main( frame ) && Web_IsLocalFrame( frame ) &&
         uivm && VM_Call( uivm, UI_WEB_SCREEN ) == 4 && url ) {
        const wchar_t *value = (const wchar_t *)url->str;
        int x, y, w, h, vw, vh, actionId, index;
        wchar_t tail;
        if ( swscanf( value, L"q3rally-ui://demo/viewport/%d/%d/%d/%d/%d/%d%c",
                      &x, &y, &w, &h, &vw, &vh, &tail ) == 6 &&
             x >= 0 && y >= 0 && w > 0 && h > 0 && vw > 0 && vh > 0 &&
             vw <= 32768 && vh <= 32768 && x <= vw && y <= vh && w <= vw - x && h <= vh - y ) {
            RECT rect;
            GetClientRect( webParent, &rect );
            webDemoRect.left = (int)((double)x * rect.right / vw);
            webDemoRect.top = (int)((double)y * rect.bottom / vh);
            webDemoRect.right = (int)((double)(x + w) * rect.right / vw);
            webDemoRect.bottom = (int)((double)(y + h) * rect.bottom / vh);
            webDemoRectValid = webDemoRectChanged = qtrue;
        }
        if ( userGesture && swscanf( value, L"q3rally-ui://demo/action/%d/%d%c",
                                    &actionId, &index, &tail ) == 2 &&
             actionId >= 0 && actionId <= 6 && index >= 0 &&
             index < VM_Call( uivm, UI_WEB_ITEM_COUNT, 4 ) ) {
            webDemoAction = actionId;
            webDemoIndex = index;
        }
        if ( userGesture && !wcscmp( value, L"q3rally-ui://demo/back" ) ) {
            webDemoAction = 6;
        }
    }
    if ( !allow && !redirect && userGesture && frame->is_main( frame ) &&
         Web_IsLocalFrame( frame ) && uivm && url ) {
        int screen = VM_Call( uivm, UI_WEB_SCREEN ), actionId, index;
        wchar_t tail;
        if ( (screen == 5 || screen == 6) &&
             swscanf( (const wchar_t *)url->str, L"q3rally-ui://catalog/action/%d/%d%c", &actionId, &index, &tail ) == 2 &&
             actionId >= 0 && actionId <= 3 && index >= -1 &&
             (index == -1 || index < VM_Call( uivm, UI_WEB_CATALOG_COUNT, screen )) ) {
            webCatalogAction = actionId;
            webCatalogIndex = index;
            webCatalogActionScreen = screen;
        }
    }
    if ( !allow && !redirect && userGesture && frame->is_main( frame ) && Web_IsLocalFrame( frame ) &&
         uivm && VM_Call( uivm, UI_WEB_SCREEN ) == 8 && url ) {
        const wchar_t *value = (const wchar_t *)url->str;
        if ( !wcscmp( value, L"q3rally-ui://driver/back" ) ) webDriverAction = 0;
        if ( !wcscmp( value, L"q3rally-ui://driver/edit" ) ) webDriverAction = 1;
        if ( !wcscmp( value, L"q3rally-ui://driver/switch" ) ) webDriverAction = 2;
        if ( !wcscmp( value, L"q3rally-ui://driver/garage" ) ) webDriverAction = 3;
        {
            static const wchar_t *prefixes[] = { L"q3rally-ui://driver/save/", L"q3rally-ui://driver/create/", L"q3rally-ui://driver/select/" };
            int i;
            for ( i = 0; i < ARRAY_LEN( prefixes ); i++ ) {
                size_t n = wcslen( prefixes[i] );
                if ( !wcsncmp( value, prefixes[i], n ) && Web_DriverPayload( value + n ) ) webDriverAction = i + 4;
            }
        }
    }
    if ( !allow && !redirect && frame->is_main( frame ) && Web_IsLocalFrame( frame ) &&
         Web_Equals( url, L"q3rally-ui://menu/credits-exit" ) && uivm &&
         VM_Call( uivm, UI_WEB_SCREEN ) == 3 ) webMenuAction = 12;
	if ( !allow && !redirect && userGesture && frame->is_main( frame ) &&
	     Web_IsLocalFrame( frame ) && (clc.state == CA_DISCONNECTED || clc.demoPlayer || (clc.state == CA_ACTIVE && (Key_GetCatcher() & KEYCATCH_UI))) ) {
		if ( Web_Equals( url, L"q3rally-ui://garage/close" ) ) {
			if ( webFrontend->integer ) {
                if ( uivm && VM_Call( uivm, UI_WEB_SCREEN ) == 7 ) webMenuAction = 14;
                webGarageMode = qfalse;
                webLastState = -1;
            } else webCloseRequested = qtrue;
        } else if ( webGarageMode && Web_Equals( url, L"q3rally-ui://garage/link" ) ) {
            Cvar_Set( "ui_webprofile_action", "link" );
        } else if ( webGarageMode && url &&
                    !wcsncmp( (const wchar_t *)url->str, sync, ARRAY_LEN( sync ) - 1 ) ) {
            char id[WEB_ASSET_ID];
            path = Web_ParseAssetId( (const wchar_t *)url->str + ARRAY_LEN( sync ) - 1, id );
            if ( path && !*path && FS_ReadFile( va( "models/players/%s/body.md3", id ), NULL ) > 0 )
                Cvar_Set( "ui_webprofile_action", va( "sync:%s", id ) );
        } else if ( webGarageMode && url &&
                    !wcsncmp( (const wchar_t *)url->str, upload, ARRAY_LEN( upload ) - 1 ) ) {
            const wchar_t *slotText = (const wchar_t *)url->str + ARRAY_LEN( upload ) - 1;
            if ( slotText[0] >= L'0' && slotText[0] <= L'3' && !slotText[1] ) {
                char fields[5][WEB_ASSET_ID];
                int slot = slotText[0] - L'0', field, i;
                qboolean valid = qtrue;
                for ( field = 0; field < 5; field++ ) {
                    for ( i = 0; i < WEB_ASSET_ID - 1; i++ ) {
                        fields[field][i] = VM_Call( uivm, UI_WEB_FAVORITE_CHAR, slot, field, i );
                        if ( !fields[field][i] ) break;
                    }
                    fields[field][i] = 0;
                    if ( field >= 3 && !fields[field][0] )
                        Cvar_VariableStringBuffer( field == 3 ? "head" : "plate", fields[field], sizeof( fields[field] ) );
                    if ( !Web_AssetId( fields[field] ) ) valid = qfalse;
                }
                if ( valid ) Cvar_Set( "ui_webprofile_action", va( "favorite:%d %s %s %s %s %s",
                                      slot, fields[0], fields[1], fields[2], fields[3], fields[4] ) );
            }
        } else if ( Web_Equals( url, L"q3rally-ui://menu/garage" ) ) {
            if ( uivm && VM_Call( uivm, UI_WEB_SCREEN ) == 2 ) {
                webGarageMode = qtrue;
                webLastState = -1;
            }
        } else if ( url && url->length > ARRAY_LEN( action ) - 1 &&
                    !wcsncmp( (const wchar_t *)url->str, action, ARRAY_LEN( action ) - 1 ) ) {
            static const wchar_t *const actions[] = {
                L"offline", L"online", L"config", L"rivals", L"demos", L"downloads",
                L"profile", L"stats", L"quit", L"update", L"skip", L"credits-exit", L"credits-start"
            };
            int i;
            if ( !webGarageMode ) {
                for ( i = 0; i < ARRAY_LEN( actions ); i++ ) {
                    if ( !wcscmp( (const wchar_t *)url->str + ARRAY_LEN( action ) - 1, actions[i] ) )
                        webMenuAction = i + 1;
                }
            }
        } else if ( url &&
                    ((url->length >= ARRAY_LEN( apply ) &&
                      !wcsncmp( (const wchar_t *)url->str, apply, ARRAY_LEN( apply ) - 1 )) ||
                     (url->length >= ARRAY_LEN( save ) &&
                      !wcsncmp( (const wchar_t *)url->str, save, ARRAY_LEN( save ) - 1 ))) ) {
            qboolean saving = !wcsncmp( (const wchar_t *)url->str, save, ARRAY_LEN( save ) - 1 );
            Com_Memset( &webSetup, 0, sizeof( webSetup ) );
            webSetup.pending = qtrue;
            webSetup.slot = -1;
            path = (const wchar_t *)url->str + (saving ? ARRAY_LEN( save ) : ARRAY_LEN( apply )) - 1;
            if ( saving ) {
                if ( path[0] < L'0' || path[0] > L'3' || path[1] != L'/' ) path = NULL;
                else { webSetup.slot = path[0] - L'0'; path += 2; }
            }
            if ( path ) path = Web_ParseAssetId( path, webSetup.vehicle );
            if ( path && *path == L'/' ) path = Web_ParseAssetId( path + 1, webSetup.paint );
            else path = NULL;
            if ( path && *path == L'/' ) path = Web_ParseAssetId( path + 1, webSetup.wheels );
            else path = NULL;
            if ( path && *path == L'/' ) path = Web_ParseAssetId( path + 1, webSetup.head );
            else path = NULL;
            if ( path && *path == L'/' ) path = Web_ParseAssetId( path + 1, webSetup.plate );
            else path = NULL;
            if ( !path || *path ) webSetup.vehicle[0] = 0;
        }
	}
	if ( url ) cef_string_userfree_utf16_free( url );
	return !allow;
}

static void Web_PublishFavorites( cef_frame_t *frame );

static void CEF_CALLBACK Web_LoadEnd( cef_load_handler_t *self, cef_browser_t *browser,
	cef_frame_t *frame, int httpStatus ) {
	wchar_t script[640];
	char profile[64], escaped[400];
	size_t i, length = 0;
	char vehicle[MAX_QPATH], wheels[WEB_ASSET_ID], head[WEB_ASSET_ID], plate[WEB_ASSET_ID];
	char *paint;

	if ( !frame->is_main( frame ) || !Web_IsLocalFrame( frame ) ) return;
	Com_Printf( "Web UI: bundled frontend loaded (HTTP %d).\n", httpStatus );
	webLastState = -1;
	webItemsScreen = 0;
    webDriverPublished = qfalse;
    webOfflineScreen = 0;
    webOnlinePublished = qfalse;
    webConfigPublished = qfalse;
    webHUDPublished = webHUDRectValid = webHUDRegion = qfalse;
	webCatalogScreen = 0;
	webCatalogState = -1;
	webCatalogRevision = -1;
	Cvar_VariableStringBuffer( "model", vehicle, sizeof( vehicle ) );
	Cvar_VariableStringBuffer( "rim", wheels, sizeof( wheels ) );
    Cvar_VariableStringBuffer( "head", head, sizeof( head ) );
    Cvar_VariableStringBuffer( "plate", plate, sizeof( plate ) );
	paint = strchr( vehicle, '/' );
	if ( paint ) *paint++ = 0;
	if ( !Web_AssetId( vehicle ) ) vehicle[0] = 0;
	swprintf( script, ARRAY_LEN( script ), L"window.q3rallyGarage?.select('%hs','%hs','%hs',%d,'%hs','%hs')",
	          vehicle, paint && Web_AssetId( paint ) ? paint : "", Web_AssetId( wheels ) ? wheels : "", Web_UnlockedRank(),
              Web_AssetId( head ) ? head : "", Web_AssetId( plate ) ? plate : "" );
    Web_RunScript( frame, script );
    Web_PublishFavorites( frame );
    Cvar_VariableStringBuffer( "profile_active", profile, sizeof( profile ) );
    for ( i = 0; profile[i] && length + 6 < sizeof( escaped ); i++ ) {
        length += Com_sprintf( escaped + length, sizeof( escaped ) - length,
                               "\\x%02x", (unsigned char)profile[i] );
    }
    escaped[length] = 0;
    swprintf( script, ARRAY_LEN( script ), L"window.q3rallyFrontend?.profile('%hs',%d,%d)",
              escaped, Web_UnlockedRank(), uivm ? (int)VM_Call( uivm, UI_WEB_PROFILE_POINTS ) : 0 );
    Web_RunScript( frame, script );
}

static int CEF_CALLBACK Web_DoClose( cef_life_span_handler_t *self, cef_browser_t *browser ) {
	/* Close the browser child without sending WM_CLOSE to the game window. */
	if ( webChild && IsWindow( webChild ) ) {
		DestroyWindow( webChild );
		return 1;
	}
	return 0;
}

static void CEF_CALLBACK Web_BeforeClose( cef_life_span_handler_t *self, cef_browser_t *browser ) {
	Web_PrintTiming();
	if ( webBrowser ) webBrowser->base.release( &webBrowser->base );
	webBrowser = NULL;
	webChild = NULL;
	if ( webParent && IsWindow( webParent ) ) {
        SDL_Window *window = IN_GetWindow();
        SetFocus( webParent );
        if ( window ) {
            SDL_RaiseWindow( window );
            SDL_SetWindowInputFocus( window );
        }
        Key_ClearStates();
    }
	webParent = NULL;
	webDemoRectValid = webDemoRegion = qfalse;
    webHUDPublished = webHUDRectValid = webHUDRegion = qfalse; webHUDAction = -1;
    webConfigRead = webConfigWrite = 0;
    Cvar_Set( "cl_webIngameUI", "0" ); Cvar_Set( "cl_webHUDActive", "0" ); Cvar_Set( "cl_webMapLoading", "0" );
	Cvar_Set( "cl_webDemoRect", "" );
	SDL_ShowCursor( SDL_DISABLE );
}

static void CEF_CALLBACK Web_ContextInitialized( cef_browser_process_handler_t *self ) {
	cef_string_t scheme = { 0 }, domain = { 0 };
	if ( Web_SetString( &scheme, L"https" ) && Web_SetString( &domain, L"q3rally.invalid" ) ) {
		webContextReady = cef_register_scheme_handler_factory( &scheme, &domain, &webFactory ) != 0;
	}
	cef_string_utf16_clear( &scheme );
	cef_string_utf16_clear( &domain );
	if ( !webContextReady ) Com_Printf( "Web garage: could not register the local origin.\n" );
}

static cef_request_handler_t webRequest;
static cef_load_handler_t webLoad;
static cef_life_span_handler_t webLife;
static cef_browser_process_handler_t webProcess;
static cef_client_t webClient;
static cef_app_t webApp;

static cef_request_handler_t *CEF_CALLBACK Web_GetRequest( cef_client_t *self ) { return &webRequest; }
static cef_load_handler_t *CEF_CALLBACK Web_GetLoad( cef_client_t *self ) { return &webLoad; }
static cef_life_span_handler_t *CEF_CALLBACK Web_GetLife( cef_client_t *self ) { return &webLife; }
static cef_browser_process_handler_t *CEF_CALLBACK Web_GetProcess( cef_app_t *self ) { return &webProcess; }

static qboolean Web_Initialize( void ) {
	wchar_t directory[MAX_PATH], locales[MAX_PATH], file[MAX_PATH];
	wchar_t home[MAX_PATH], cache[MAX_PATH];
	wchar_t *slash;
	DWORD length;
	cef_main_args_t args = { GetModuleHandleW( NULL ) };
	cef_settings_t settings = { 0 };

	if ( webInitialized ) return qtrue;
	length = GetModuleFileNameW( NULL, directory, ARRAY_LEN( directory ) );
	if ( !length || length >= ARRAY_LEN( directory ) ) return qfalse;
	slash = wcsrchr( directory, L'\\' );
	if ( !slash ) return qfalse;
	*slash = 0;
	if ( swprintf( webRoot, ARRAY_LEN( webRoot ), L"%ls\\webui\\garage", directory ) < 0 ||
	     swprintf( file, ARRAY_LEN( file ), L"%ls\\index.html", webRoot ) < 0 ||
	     swprintf( locales, ARRAY_LEN( locales ), L"%ls\\locales", directory ) < 0 ||
	     GetFileAttributesW( file ) == INVALID_FILE_ATTRIBUTES ) {
		Com_Printf( "Web garage: bundled assets not found.\n" );
		return qfalse;
	}

	Web_StaticHandler( &webFactory.base, sizeof( webFactory ) );
	webFactory.create = Web_CreateResource;
	Web_StaticHandler( &webApp.base, sizeof( webApp ) );
	Web_StaticHandler( &webProcess.base, sizeof( webProcess ) );
	Web_StaticHandler( &webClient.base, sizeof( webClient ) );
	Web_StaticHandler( &webRequest.base, sizeof( webRequest ) );
	Web_StaticHandler( &webLoad.base, sizeof( webLoad ) );
	Web_StaticHandler( &webLife.base, sizeof( webLife ) );
	webApp.get_browser_process_handler = Web_GetProcess;
	webProcess.on_context_initialized = Web_ContextInitialized;
	webClient.get_request_handler = Web_GetRequest;
	webClient.get_load_handler = Web_GetLoad;
	webClient.get_life_span_handler = Web_GetLife;
	webRequest.on_before_browse = Web_BeforeBrowse;
	webLoad.on_load_end = Web_LoadEnd;
	webLife.do_close = Web_DoClose;
	webLife.on_before_close = Web_BeforeClose;

	settings.size = sizeof( settings );
	settings.no_sandbox = 1; /* MinGW pilot, restricted to the bundled local page. */
	settings.command_line_args_disabled = 1;
	if ( !MultiByteToWideChar( CP_ACP, 0, Cvar_VariableString( "fs_homepath" ), -1, home, ARRAY_LEN( home ) ) ||
	     swprintf( cache, ARRAY_LEN( cache ), L"%ls\\cef-garage", home ) < 0 ) return qfalse;
	Web_SetString( &settings.root_cache_path, cache );
	Web_SetString( &settings.resources_dir_path, directory );
	Web_SetString( &settings.locales_dir_path, locales );
	webInitialized = cef_initialize( &args, &settings, &webApp, NULL ) != 0;
	cef_string_utf16_clear( &settings.root_cache_path );
	cef_string_utf16_clear( &settings.resources_dir_path );
	cef_string_utf16_clear( &settings.locales_dir_path );
	if ( !webInitialized ) Com_Printf( "Web garage: CEF initialization failed.\n" );
	return webInitialized;
}

int CL_WebGarageSubprocess( int argc, char **argv ) {
	cef_main_args_t args = { GetModuleHandleW( NULL ) };
	int i;

	if ( !cef_api_hash( CEF_API_VERSION, 0 ) ) return 1;
	for ( i = 1; i < argc; i++ ) {
		if ( !strncmp( argv[i], "--type=", 7 ) ) {
			return cef_execute_process( &args, NULL, NULL );
		}
	}
	return -1;
}

void CL_WebGarage_f( void ) {
	if ( clc.state != CA_DISCONNECTED ) {
		Com_Printf( "Web garage requires the disconnected main menu.\n" );
		return;
	}
	webGarageMode = qtrue;
	webLastState = -1;
	if ( !webBrowser && Web_Initialize() ) webOpenRequested = qtrue;
}

static void Web_Open( void ) {
	SDL_Window *window = IN_GetWindow();
	SDL_SysWMinfo info;
	cef_window_info_t native = { 0 };
	cef_browser_settings_t settings = { 0 };
	cef_string_t url = { 0 };
	cef_browser_host_t *host;
	RECT rect;

	MultiByteToWideChar( CP_UTF8, 0, FS_BuildOSPath( Cvar_VariableString( "fs_homepath" ),
                          Cvar_VariableString( "fs_basegame" ), "q3rally_previews" ), -1, webPreviewRoot, ARRAY_LEN( webPreviewRoot ) );
    webOpenRequested = qfalse;
	webLastState = -1;
	SDL_VERSION( &info.version );
	if ( !window || !SDL_GetWindowWMInfo( window, &info ) || info.subsystem != SDL_SYSWM_WINDOWS ) {
		Com_Printf( "Web garage: could not find the SDL game window.\n" );
		return;
	}
	webParent = info.info.win.window;
	GetClientRect( webParent, &rect );
	native.size = sizeof( native );
	native.parent_window = webParent;
	native.style = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	native.bounds.width = rect.right;
	native.bounds.height = rect.bottom;
	native.runtime_style = CEF_RUNTIME_STYLE_ALLOY;
	settings.size = sizeof( settings );
	Web_SetString( &url, webPage );
	webBrowser = cef_browser_host_create_browser_sync( &native, &webClient, &url, &settings, NULL, NULL );
	cef_string_utf16_clear( &url );
	if ( !webBrowser ) {
		webParent = NULL;
		Com_Printf( "Web garage: browser creation failed.\n" );
		return;
	}

	host = webBrowser->get_host( webBrowser );
	webChild = host->get_window_handle( host );
	Com_Printf( "Web UI: browser attached (%s, %dx%d).\n",
	    SDL_GetWindowFlags( window ) & SDL_WINDOW_FULLSCREEN ? "fullscreen" : "windowed",
	    (int)rect.right, (int)rect.bottom );
	Com_Memset( &webStats, 0, sizeof( webStats ) );
	QueryPerformanceFrequency( &webStats.frequency );
	SDL_SetRelativeMouseMode( SDL_FALSE );
	SDL_ShowCursor( SDL_ENABLE );
	host->set_focus( host, 1 );
	host->base.release( &host->base );
}

static void Web_Close( void ) {
	cef_browser_host_t *host;
	if ( !webBrowser ) return;
	host = webBrowser->get_host( webBrowser );
	host->set_focus( host, 0 );
	host->close_browser( host, 1 );
	host->base.release( &host->base );
}

static void Web_QuotedBytes( const char *text, wchar_t *output, size_t capacity ) {
    size_t i, position = 0;
    for ( i = 0; text[i] && position + 3 < capacity; i++ ) {
        swprintf( output + position, capacity - position, L"%%%02X", (unsigned char)text[i] );
        position += 3;
    }
    output[position] = 0;
}

static void Web_PublishFavorites( cef_frame_t *frame ) {
    int slot, field, i;
    char text[64];
    wchar_t fields[5][200], script[1400];
    for ( slot = 0; slot < 4; slot++ ) {
        for ( field = 0; field < 5; field++ ) {
            for ( i = 0; i < sizeof( text ) - 1; i++ ) {
                text[i] = uivm ? VM_Call( uivm, UI_WEB_FAVORITE_CHAR, slot, field, i ) : 0;
                if ( !text[i] ) break;
            }
            text[i] = 0;
            Web_QuotedBytes( text, fields[field], ARRAY_LEN( fields[field] ) );
        }
        swprintf( script, ARRAY_LEN( script ),
                  L"window.q3rallyGarage?.favorite(%d,decodeURIComponent('%ls'),decodeURIComponent('%ls'),"
                  L"decodeURIComponent('%ls'),decodeURIComponent('%ls'),decodeURIComponent('%ls'))",
                  slot, fields[0], fields[1], fields[2], fields[3], fields[4] );
        Web_RunScript( frame, script );
    }
}

static void Web_PublishItems( int screen ) {
    cef_frame_t *frame = webBrowser->get_main_frame( webBrowser );
    wchar_t script[1400], encoded[1024];
    char text[256];
    int i, j, count = VM_Call( uivm, UI_WEB_ITEM_COUNT, screen );
    Web_RunScript( frame, screen == 3 ? L"window.q3rallyArchive?.creditsBegin()" : L"window.q3rallyArchive?.demosBegin()" );
    for ( i = 0; i < count; i++ ) {
        int size = 0, kind = 0;
        for ( j = 0; j < sizeof( text ) - 1; j++ ) {
            text[j] = VM_Call( uivm, UI_WEB_ITEM_CHAR, screen, i, j );
            if ( !text[j] ) break;
        }
        text[j] = 0;
        Web_QuotedBytes( text, encoded, ARRAY_LEN( encoded ) );
        if ( screen == 4 ) size = FS_ReadFile( va( "demos/%s", text ), NULL );
        else kind = VM_Call( uivm, UI_WEB_ITEM_KIND, i );
        swprintf( script, ARRAY_LEN( script ),
                  L"window.q3rallyArchive?.item(%d,%d,decodeURIComponent('%ls'),%d,%d)",
                  screen, i, encoded, size, kind );
        Web_RunScript( frame, script );
    }
    Web_RunScript( frame, screen == 3 ? L"window.q3rallyArchive?.creditsEnd()" : L"window.q3rallyArchive?.demosEnd()" );
    frame->base.release( &frame->base );
    webItemsScreen = screen;
}

static void Web_UpdateDemoRegion( qboolean show ) {
    if ( show && (!webDemoRegion || webDemoRectChanged) ) {
        RECT rect;
        HRGN outer, hole;
        GetClientRect( webParent, &rect );
        outer = CreateRectRgn( 0, 0, rect.right, rect.bottom );
        hole = CreateRectRgn( webDemoRect.left, webDemoRect.top, webDemoRect.right, webDemoRect.bottom );
        CombineRgn( outer, outer, hole, RGN_DIFF );
        DeleteObject( hole );
        if ( !SetWindowRgn( webChild, outer, TRUE ) ) DeleteObject( outer );
        Cvar_Set( "cl_webDemoRect", va( "%ld %ld %ld %ld", webDemoRect.left, webDemoRect.top,
                  webDemoRect.right - webDemoRect.left, webDemoRect.bottom - webDemoRect.top ) );
        webDemoRegion = qtrue;
        webDemoRectChanged = qfalse;
    } else if ( !show && webDemoRegion ) {
        SetWindowRgn( webChild, NULL, TRUE );
        Cvar_Set( "cl_webDemoRect", "" );
        webDemoRegion = qfalse;
    }
}

static void Web_PublishCatalog( int screen ) {
    cef_frame_t *frame = webBrowser->get_main_frame( webBrowser );
    wchar_t script[5600], fields[5][1000];
    char text[320];
    int i, field, j, count = VM_Call( uivm, UI_WEB_CATALOG_COUNT, screen );
    Web_RunScript( frame, screen == 5 ? L"window.q3rallyLibrary?.begin(5)" : L"window.q3rallyLibrary?.begin(6)" );
    for ( i = 0; i < count; i++ ) {
        for ( field = 0; field < 5; field++ ) {
            for ( j = 0; j < sizeof( text ) - 1; j++ ) {
                text[j] = VM_Call( uivm, UI_WEB_CATALOG_CHAR, screen, i, field, j );
                if ( !text[j] ) break;
            }
            text[j] = 0;
            Web_QuotedBytes( text, fields[field], ARRAY_LEN( fields[field] ) );
        }
        swprintf( script, ARRAY_LEN( script ),
                  L"window.q3rallyLibrary?.item(%d,%d,[decodeURIComponent('%ls'),decodeURIComponent('%ls'),"
                  L"decodeURIComponent('%ls'),decodeURIComponent('%ls'),decodeURIComponent('%ls')],%d,%d)",
                  screen, i, fields[0], fields[1], fields[2], fields[3], fields[4],
                  screen == 6 ? (int)VM_Call( uivm, UI_WEB_CATALOG_NUMBER, i, 0 ) : 0,
                  screen == 6 ? (int)VM_Call( uivm, UI_WEB_CATALOG_NUMBER, i, 1 ) : 0 );
        Web_RunScript( frame, script );
    }
    Web_RunScript( frame, screen == 5 ? L"window.q3rallyLibrary?.end(5)" : L"window.q3rallyLibrary?.end(6)" );
    frame->base.release( &frame->base );
    webCatalogScreen = screen;
    webCatalogState = Cvar_VariableIntegerValue( "ui_dl_state" );
    webCatalogRevision = screen == 6 ? VM_Call( uivm, UI_WEB_CATALOG_NUMBER, -1, 0 ) : -1;
}

static void Web_PublishDownloadState( void ) {
    cef_frame_t *frame = webBrowser->get_main_frame( webBrowser );
    wchar_t script[2500], error[1600], filename[400];
    char text[512];
    Cvar_VariableStringBuffer( "ui_dl_error", text, sizeof( text ) );
    Web_QuotedBytes( text, error, ARRAY_LEN( error ) );
    Cvar_VariableStringBuffer( "ui_dl_filename", text, sizeof( text ) );
    Web_QuotedBytes( text, filename, ARRAY_LEN( filename ) );
    swprintf( script, ARRAY_LEN( script ),
              L"window.q3rallyLibrary?.downloadState(%d,%d,decodeURIComponent('%ls'),decodeURIComponent('%ls'))",
              Cvar_VariableIntegerValue( "ui_dl_state" ), Cvar_VariableIntegerValue( "ui_dl_progress" ), filename, error );
    Web_RunScript( frame, script );
    frame->base.release( &frame->base );
}

static void Web_ProfileFrame( void ) {
    static int previousReport;
    if ( cls.realtime - previousReport < 100 ) return;
    previousReport = cls.realtime;
    cef_frame_t *frame = webBrowser->get_main_frame( webBrowser );
    char payload[400], *cursor, uuid[37], current[37], fields[5][WEB_ASSET_ID], message[256];
    wchar_t script[1100], encoded[800];
    int i;
    if ( uivm && VM_Call( uivm, UI_WEB_FAVORITES_IMPORT ) ) Web_PublishFavorites( frame );
    if ( !webGarageMode ) {
        frame->base.release( &frame->base );
        return;
    }
    Cvar_VariableStringBuffer( "ui_webprofile_setup", payload, sizeof( payload ) );
    if ( payload[0] ) {
        Cvar_Set( "ui_webprofile_setup", "" );
        cursor = payload;
        Q_strncpyz( uuid, COM_Parse( &cursor ), sizeof( uuid ) );
        Cvar_VariableStringBuffer( "cl_uuid", current, sizeof( current ) );
        for ( i = 0; i < 5; i++ ) Q_strncpyz( fields[i], COM_Parse( &cursor ), sizeof( fields[i] ) );
        if ( !fields[3][0] ) Cvar_VariableStringBuffer( "head", fields[3], sizeof( fields[3] ) );
        if ( !fields[4][0] ) Cvar_VariableStringBuffer( "plate", fields[4], sizeof( fields[4] ) );
        if ( !Q_stricmp( uuid, current ) && Web_AssetId( fields[0] ) && Web_AssetId( fields[1] ) &&
             Web_AssetId( fields[2] ) && Web_AssetId( fields[3] ) && Web_AssetId( fields[4] ) ) {
            swprintf( script, ARRAY_LEN( script ), L"window.q3rallyGarage?.cloudSetup('%hs','%hs','%hs','%hs','%hs')",
                      fields[0], fields[1], fields[2], fields[3], fields[4] );
            Web_RunScript( frame, script );
        }
    }
    Cvar_VariableStringBuffer( "ui_webprofile_status", message, sizeof( message ) );
    Web_QuotedBytes( message, encoded, ARRAY_LEN( encoded ) );
    swprintf( script, ARRAY_LEN( script ), L"window.q3rallyGarage?.cloudStatus(decodeURIComponent('%ls'))", encoded );
    Web_RunScript( frame, script );
    frame->base.release( &frame->base );
}

static void Web_DriverText( int category, int tier, int field, wchar_t *out, size_t capacity ) {
    char text[256];
    int i;
    for ( i = 0; i < sizeof( text ) - 1; i++ ) {
        text[i] = VM_Call( uivm, UI_WEB_ACHIEVEMENT_CHAR, category, tier, field, i );
        if ( !text[i] ) break;
    }
    text[i] = 0;
    Web_QuotedBytes( text, out, capacity );
}

static void Web_PublishDriver( void ) {
    cef_frame_t *frame = webBrowser->get_main_frame( webBrowser );
    wchar_t script[6400], fields[7][800];
    char name[64];
    void *data = NULL;
    int length = -1, category, tier, field;
    Web_RunScript( frame, L"window.q3rallyDriver?.begin()" );
    {
        char names[16384], *name;
        int count = FS_GetFileList( "profiles", ".json", names, sizeof( names ) ), i;
        for ( i = 0, name = names; i < count; i++, name += strlen( name ) + 1 ) {
            char clean[64];
            int n = strlen( name );
            if ( n < 6 || n >= sizeof( clean ) || FS_ReadFile( va( "profiles/%s", name ), NULL ) <= 0 ) continue;
            COM_StripExtension( name, clean, sizeof( clean ) );
            Web_QuotedBytes( clean, fields[0], ARRAY_LEN( fields[0] ) );
            swprintf( script, ARRAY_LEN( script ), L"window.q3rallyDriver?.available(decodeURIComponent('%ls'))", fields[0] );
            Web_RunScript( frame, script );
        }
    }
    Cvar_VariableStringBuffer( "profile_active", name, sizeof( name ) );
    if ( VM_Call( uivm, UI_WEB_DRIVER_VALUE, 0, 0 ) && name[0] &&
         !strchr( name, '/' ) && !strchr( name, '\\' ) && !strstr( name, ".." ) ) {
        length = FS_ReadFile( va( "profiles/%s.json", name ), &data );
    }
    if ( length > 0 && length <= 32768 ) {
        size_t capacity = (size_t)length * 3 + 128;
        wchar_t *json = calloc( capacity, sizeof( *json ) );
        if ( json ) {
            static const wchar_t prefix[] = L"window.q3rallyDriver?.profile(JSON.parse(decodeURIComponent('";
            wcscpy( json, prefix );
            Web_QuotedBytes( (const char *)data, json + ARRAY_LEN( prefix ) - 1, capacity - ARRAY_LEN( prefix ) );
            wcscat( json, L"')))" );
            Web_RunScript( frame, json );
            free( json );
        }
    }
    if ( data ) FS_FreeFile( data );
    swprintf( script, ARRAY_LEN( script ), L"window.q3rallyDriver?.rank(%d,%d,%d)",
              (int)VM_Call( uivm, UI_WEB_DRIVER_VALUE, 1, 0 ),
              (int)VM_Call( uivm, UI_WEB_DRIVER_VALUE, 2, 0 ),
              (int)VM_Call( uivm, UI_WEB_DRIVER_VALUE, 3, 0 ) );
    Web_RunScript( frame, script );
    for ( category = 0; category < VM_Call( uivm, UI_WEB_DRIVER_VALUE, 4, 0 ); category++ ) {
        Web_DriverText( category, -1, 0, fields[0], ARRAY_LEN( fields[0] ) );
        Web_DriverText( category, -1, 4, fields[4], ARRAY_LEN( fields[4] ) );
        swprintf( script, ARRAY_LEN( script ),
                  L"window.q3rallyDriver?.category(%d,decodeURIComponent('%ls'),decodeURIComponent('%ls'))",
                  category, fields[0], fields[4] );
        Web_RunScript( frame, script );
        for ( tier = 0; tier < VM_Call( uivm, UI_WEB_DRIVER_VALUE, 5, category ); tier++ ) {
            for ( field = 1; field < 7; field++ ) Web_DriverText( category, tier, field, fields[field], ARRAY_LEN( fields[field] ) );
            swprintf( script, ARRAY_LEN( script ),
                      L"window.q3rallyDriver?.tier(%d,decodeURIComponent('%ls'),decodeURIComponent('%ls'),"
                      L"decodeURIComponent('%ls'),decodeURIComponent('%ls'),decodeURIComponent('%ls'))",
                      category, fields[1], fields[2], fields[3], fields[5], fields[6] );
            Web_RunScript( frame, script );
        }
    }
    swprintf( script, ARRAY_LEN( script ), L"window.q3rallyDriver?.end(%d)",
              (int)VM_Call( uivm, UI_WEB_DRIVER_VALUE, 6, 0 ) );
    Web_RunScript( frame, script );
    frame->base.release( &frame->base );
    webDriverPublished = qtrue;
}

static void Web_OfflineText( int kind, int index, int choice, wchar_t *out, size_t capacity ) {
    char text[512]; int i;
    for ( i = 0; i < sizeof( text ) - 1; i++ ) {
        text[i] = VM_Call( uivm, UI_WEB_OFFLINE_CHAR, kind, index, choice, i );
        if ( !text[i] ) break;
    }
    text[i] = 0; Web_QuotedBytes( text, out, capacity );
}
static void Web_PublishOffline( int screen ) {
    cef_frame_t *frame = webBrowser->get_main_frame( webBrowser );
    wchar_t script[7600], a[1600], b[1600];
    int i, j, type, count;
    swprintf( script, ARRAY_LEN( script ), L"window.q3rallyOffline?.begin(%d,%d,%d,%d,%d,%d)", screen,
              (int)VM_Call( uivm, UI_WEB_OFFLINE_NUMBER, 1, 0 ), (int)VM_Call( uivm, UI_WEB_OFFLINE_NUMBER, 2, 0 ),
              (int)VM_Call( uivm, UI_WEB_OFFLINE_NUMBER, 12, 0 ), (int)VM_Call( uivm, UI_WEB_OFFLINE_NUMBER, 14, 0 ), (int)VM_Call( uivm, UI_WEB_OFFLINE_NUMBER, 15, 0 ) );
    Web_RunScript( frame, script );
    for ( i = 0; i < 16; i++ ) {
        Web_OfflineText( 3, i, 0, a, ARRAY_LEN( a ) );
        swprintf( script, ARRAY_LEN( script ), L"window.q3rallyOffline?.mode(%d,decodeURIComponent('%ls'))", i, a ); Web_RunScript( frame, script );
    }
    for ( i = 0; i < VM_Call( uivm, UI_WEB_OFFLINE_NUMBER, 0, 0 ); i++ ) {
        Web_OfflineText( 0, i, 0, a, ARRAY_LEN( a ) ); Web_OfflineText( 1, i, 0, b, ARRAY_LEN( b ) );
        swprintf( script, ARRAY_LEN( script ), L"window.q3rallyOffline?.map(%d,decodeURIComponent('%ls'),decodeURIComponent('%ls'))", i, a, b ); Web_RunScript( frame, script );
    }
    for ( i = 0; i < VM_Call( uivm, UI_WEB_OFFLINE_NUMBER, 11, 0 ); i++ ) {
        Web_OfflineText( 2, i, 0, a, ARRAY_LEN( a ) );
        swprintf( script, ARRAY_LEN( script ), L"window.q3rallyOffline?.detail(decodeURIComponent('%ls'))", a ); Web_RunScript( frame, script );
    }
    if ( screen == 10 ) {
        Web_OfflineText( 10, 0, 0, a, ARRAY_LEN( a ) );
        swprintf( script, ARRAY_LEN( script ), L"window.q3rallyOffline?.avatar(decodeURIComponent('%ls'))", a ); Web_RunScript( frame, script );
        for ( i = 0; i < VM_Call( uivm, UI_WEB_OFFLINE_NUMBER, 3, 0 ); i++ ) {
            type = VM_Call( uivm, UI_WEB_OFFLINE_NUMBER, 4, i ); if ( !type ) continue;
            Web_OfflineText( 4, i, 0, a, ARRAY_LEN( a ) ); Web_OfflineText( 5, i, 0, b, ARRAY_LEN( b ) );
            swprintf( script, ARRAY_LEN( script ), L"window.q3rallyOffline?.control(%d,%d,decodeURIComponent('%ls'),decodeURIComponent('%ls'),%d,%d,%d,%d,%d)",
                      i, type, a, b, (int)VM_Call( uivm, UI_WEB_OFFLINE_NUMBER, 5, i ), (int)VM_Call( uivm, UI_WEB_OFFLINE_NUMBER, 7, i ),
                      (int)VM_Call( uivm, UI_WEB_OFFLINE_NUMBER, 9, i ), (int)VM_Call( uivm, UI_WEB_OFFLINE_NUMBER, 10, i ), (int)VM_Call( uivm, UI_WEB_OFFLINE_NUMBER, 13, i ) );
            Web_RunScript( frame, script );
            count = VM_Call( uivm, UI_WEB_OFFLINE_NUMBER, 6, i );
            for ( j = 0; j < count; j++ ) {
                Web_OfflineText( 6, i, j, a, ARRAY_LEN( a ) );
                swprintf( script, ARRAY_LEN( script ), L"window.q3rallyOffline?.choice(%d,%d,decodeURIComponent('%ls'))", i, j, a ); Web_RunScript( frame, script );
            }
        }
        for ( i = 0; i < 12; i++ ) {
            Web_OfflineText( 8, i, 0, a, ARRAY_LEN( a ) );
            swprintf( script, ARRAY_LEN( script ), L"window.q3rallyOffline?.player(%d,decodeURIComponent('%ls'))", i, a ); Web_RunScript( frame, script );
        }
        for ( i = 0; i < VM_Call( uivm, UI_WEB_OFFLINE_NUMBER, 8, 0 ); i++ ) {
            Web_OfflineText( 7, i, 0, a, ARRAY_LEN( a ) ); Web_OfflineText( 9, i, 0, b, ARRAY_LEN( b ) );
            swprintf( script, ARRAY_LEN( script ), L"window.q3rallyOffline?.bot(%d,decodeURIComponent('%ls'),decodeURIComponent('%ls'))", i, a, b ); Web_RunScript( frame, script );
        }
    }
    Web_RunScript( frame, L"window.q3rallyOffline?.end()" );
    frame->base.release( &frame->base ); webOfflineScreen = screen;
}

static void Web_OnlineText( int kind, int index, wchar_t *out, size_t capacity ) {
    char text[256]; int i;
    for ( i = 0; i < sizeof( text ) - 1; i++ ) {
        text[i] = VM_Call( uivm, UI_WEB_ONLINE_CHAR, kind, index, i ); if ( !text[i] ) break;
    }
    text[i] = 0; Web_QuotedBytes( text, out, capacity );
}
static void Web_PublishOnline( void ) {
    cef_frame_t *frame = webBrowser->get_main_frame( webBrowser );
    wchar_t script[6000], text[4][800];
    int i, j, kind, count;
    swprintf( script, ARRAY_LEN( script ), L"window.q3rallyOnline?.begin(%d,%d,%d,%d,%d,%d,%d)",
              (int)VM_Call( uivm, UI_WEB_ONLINE_NUMBER, 2, 0 ), (int)VM_Call( uivm, UI_WEB_ONLINE_NUMBER, 3, 0 ),
              (int)VM_Call( uivm, UI_WEB_ONLINE_NUMBER, 4, 0 ), (int)VM_Call( uivm, UI_WEB_ONLINE_NUMBER, 5, 0 ),
              (int)VM_Call( uivm, UI_WEB_ONLINE_NUMBER, 6, 0 ), (int)VM_Call( uivm, UI_WEB_ONLINE_NUMBER, 7, 0 ), (int)VM_Call( uivm, UI_WEB_ONLINE_NUMBER, 8, 0 ) );
    Web_RunScript( frame, script );
    for ( kind = 4; kind <= 6; kind++ ) {
        count = kind == 4 ? 8 : kind == 5 ? VM_Call( uivm, UI_WEB_ONLINE_NUMBER, 14, 0 ) : 5;
        for ( i = 0; i < count; i++ ) {
            Web_OnlineText( kind, i, text[0], ARRAY_LEN( text[0] ) );
            swprintf( script, ARRAY_LEN( script ), L"window.q3rallyOnline?.option(%d,%d,decodeURIComponent('%ls'))", kind, i, text[0] ); Web_RunScript( frame, script );
        }
    }
    for ( i = 0; i < VM_Call( uivm, UI_WEB_ONLINE_NUMBER, 1, 0 ); i++ ) {
        for ( j = 0; j < 4; j++ ) Web_OnlineText( j, i, text[j], ARRAY_LEN( text[j] ) );
        swprintf( script, ARRAY_LEN( script ), L"window.q3rallyOnline?.server(decodeURIComponent('%ls'),decodeURIComponent('%ls'),decodeURIComponent('%ls'),decodeURIComponent('%ls'),%d,%d,%d,%d,%d)",
                  text[0], text[1], text[2], text[3], (int)VM_Call( uivm, UI_WEB_ONLINE_NUMBER, 9, i ), (int)VM_Call( uivm, UI_WEB_ONLINE_NUMBER, 10, i ),
                  (int)VM_Call( uivm, UI_WEB_ONLINE_NUMBER, 11, i ), (int)VM_Call( uivm, UI_WEB_ONLINE_NUMBER, 12, i ), (int)VM_Call( uivm, UI_WEB_ONLINE_NUMBER, 13, i ) ); Web_RunScript( frame, script );
    }
    for ( i = 0; i < 3; i++ ) Web_OnlineText( 7 + i, 0, text[i], ARRAY_LEN( text[i] ) );
    swprintf( script, ARRAY_LEN( script ), L"window.q3rallyOnline?.end(decodeURIComponent('%ls'),decodeURIComponent('%ls'),decodeURIComponent('%ls'))", text[0], text[1], text[2] ); Web_RunScript( frame, script );
    frame->base.release( &frame->base ); webOnlinePublished = qtrue;
}

static void Web_ConfigText( int kind, int index, int choice, wchar_t *out, size_t capacity ) {
    char text[2048]; int i;
    for ( i = 0; i < sizeof( text ) - 1; i++ ) {
        text[i] = VM_Call( uivm, UI_WEB_CONFIG_CHAR, kind, index, choice, i ); if ( !text[i] ) break;
    }
    text[i] = 0; Web_QuotedBytes( text, out, capacity );
}
static void Web_PublishConfig( void ) {
    cef_frame_t *frame = webBrowser->get_main_frame( webBrowser );
    wchar_t script[15000], a[6400], b[6400]; int i, j, type;
    if ( VM_Call( uivm, UI_WEB_CONFIG_NUMBER, 0, 0 ) >= 16 && VM_Call( uivm, UI_WEB_CONFIG_NUMBER, 0, 0 ) <= 20 )
        Com_Printf( "Web UI: registration step %d.\n", (int)VM_Call( uivm, UI_WEB_CONFIG_NUMBER, 0, 0 ) );
    Web_ConfigText( 0, 0, 0, a, ARRAY_LEN( a ) ); Web_ConfigText( 4, 0, 0, b, ARRAY_LEN( b ) );
    swprintf( script, ARRAY_LEN( script ), L"window.q3rallyConfig?.begin(%d,decodeURIComponent('%ls'),decodeURIComponent('%ls'),%d)",
              (int)VM_Call( uivm, UI_WEB_CONFIG_NUMBER, 0, 0 ), a, b, (int)VM_Call( uivm, UI_WEB_CONFIG_NUMBER, 8, 0 ) ); Web_RunScript( frame, script );
    for ( i = 0; i < VM_Call( uivm, UI_WEB_CONFIG_NUMBER, 1, 0 ); i++ ) {
        type = VM_Call( uivm, UI_WEB_CONFIG_NUMBER, 2, i ); if ( !type ) continue;
        Web_ConfigText( 1, i, 0, a, ARRAY_LEN( a ) ); Web_ConfigText( 2, i, 0, b, ARRAY_LEN( b ) );
        swprintf( script, ARRAY_LEN( script ), L"window.q3rallyConfig?.item(%d,%d,decodeURIComponent('%ls'),decodeURIComponent('%ls'),%d,%d,%d,%d)", i, type, a, b,
                  (int)VM_Call( uivm, UI_WEB_CONFIG_NUMBER, 3, i ), (int)VM_Call( uivm, UI_WEB_CONFIG_NUMBER, 4, i ), (int)VM_Call( uivm, UI_WEB_CONFIG_NUMBER, 5, i ), (int)VM_Call( uivm, UI_WEB_CONFIG_NUMBER, 6, i ) ); Web_RunScript( frame, script );
        for ( j = 0; j < VM_Call( uivm, UI_WEB_CONFIG_NUMBER, 7, i ); j++ ) {
            Web_ConfigText( 3, i, j, a, ARRAY_LEN( a ) ); swprintf( script, ARRAY_LEN( script ), L"window.q3rallyConfig?.choice(%d,%d,decodeURIComponent('%ls'))", i, j, a ); Web_RunScript( frame, script );
        }
    }
    Web_RunScript( frame, L"window.q3rallyConfig?.end()" ); frame->base.release( &frame->base ); webConfigPublished = qtrue;
    webConfigRevision = VM_Call( uivm, UI_WEB_CONFIG_NUMBER, 9, 0 );
}

static void Web_HUDText( int index, int field, int choice, wchar_t *out, size_t capacity ) {
    char text[256]; int i;
    for ( i = 0; i < sizeof( text ) - 1; i++ ) { text[i] = VM_Call( cgvm, CG_WEB_HUD_CHAR, index, field, choice, i ); if ( !text[i] ) break; }
    text[i] = 0; Web_QuotedBytes( text, out, capacity );
}
static void Web_PublishHUD( void ) {
    cef_frame_t *frame = webBrowser->get_main_frame( webBrowser ); wchar_t script[1600], text[800]; int i,j;
    Web_RunScript( frame, L"window.q3rallyHUD?.begin()" );
    for ( i = 0; i < VM_Call( cgvm, CG_WEB_HUD_NUMBER, 0, 0 ); i++ ) {
        Web_HUDText( i, 0, 0, text, ARRAY_LEN( text ) );
        swprintf( script, ARRAY_LEN( script ), L"window.q3rallyHUD?.item(%d,decodeURIComponent('%ls'),%d,%d)", i, text, (int)VM_Call( cgvm, CG_WEB_HUD_NUMBER, 1, i ), (int)VM_Call( cgvm, CG_WEB_HUD_NUMBER, 3, i ) ); Web_RunScript( frame, script );
        for ( j = 0; j < VM_Call( cgvm, CG_WEB_HUD_NUMBER, 2, i ); j++ ) {
            Web_HUDText( i, 1, j, text, ARRAY_LEN( text ) ); swprintf( script, ARRAY_LEN( script ), L"window.q3rallyHUD?.choice(%d,decodeURIComponent('%ls'))", i, text ); Web_RunScript( frame, script );
        }
    }
    swprintf( script, ARRAY_LEN( script ), L"window.q3rallyHUD?.end(%d,%d)", (int)(Cvar_VariableValue( "cg_mmap_size" )*100 + .5f), Cvar_VariableIntegerValue( "cg_mmap_fov" ) ); Web_RunScript( frame, script );
    frame->base.release( &frame->base ); webHUDPublished = qtrue;
}

/* Read the loading snapshot directly: the cgame VM can be inside CG_INIT. */
static void Web_PublishMapLoading( void ) {
    const char *info = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO];
    const char *title = cl.gameState.stringData + cl.gameState.stringOffsets[CS_MESSAGE];
    wchar_t map[512], name[1024], server[2048], detail[2048], download[2048], script[10000];
    const char *status = clc.state == CA_CONNECTING ? "Awaiting challenge" :
        clc.state == CA_CHALLENGING ? "Awaiting connection" :
        clc.state == CA_CONNECTED ? "Receiving game state" :
        clc.state == CA_LOADING ? Cvar_VariableString( "cl_mapLoadingDetail" ) : "Starting race";
    cef_frame_t *frame = webBrowser->get_main_frame( webBrowser );
    Web_QuotedBytes( Info_ValueForKey( info, "mapname" ), map, ARRAY_LEN( map ) );
    Web_QuotedBytes( title, name, ARRAY_LEN( name ) );
    Web_QuotedBytes( clc.servername, server, ARRAY_LEN( server ) );
    Web_QuotedBytes( status, detail, ARRAY_LEN( detail ) );
    Web_QuotedBytes( Cvar_VariableString( "cl_downloadName" ), download, ARRAY_LEN( download ) );
    swprintf( script, ARRAY_LEN( script ), L"window.q3rallyMapLoading?.state(decodeURIComponent('%ls'),decodeURIComponent('%ls'),decodeURIComponent('%ls'),decodeURIComponent('%ls'),decodeURIComponent('%ls'),%d,%d,%d)",
        map, name, server, detail, download, Cvar_VariableIntegerValue( "cl_downloadCount" ),
        Cvar_VariableIntegerValue( "cl_downloadSize" ), clc.connectPacketCount );
    Web_RunScript( frame, script ); frame->base.release( &frame->base );
}

void CL_WebGarageFrame( void ) {
	RECT parent, child;
	LARGE_INTEGER start, end;
	qboolean measure;
    if ( webFrontend->integer && uivm && Web_CurrentScreen() && !webBrowser ) {
        int screen = Web_CurrentScreen();
        if ( screen >= 1 && screen <= 14 ) {
            if ( screen == 7 ) webGarageMode = qtrue;
            if ( Web_Initialize() ) webOpenRequested = qtrue;
        }
    }
    if ( !webInitialized ) return;
	measure = webTiming && webTiming->integer && Web_Visible();
	if ( measure ) QueryPerformanceCounter( &start );
	else webStats.previous.QuadPart = 0;
	cef_do_message_loop_work();
	if ( measure && Web_Visible() ) {
		QueryPerformanceCounter( &end );
		Web_RecordTiming( start, end );
	}
	if ( webOpenRequested && webContextReady && Web_CurrentScreen() ) Web_Open();
	if ( !webBrowser ) return;
    if ( webSessionAction == 3 ) { webSessionAction = 0; Cbuf_AddText( "disconnect\n" ); }
    if ( webSessionAction == 1 ) { webSessionAction = 0; Con_ToggleConsole_f(); }
    else if ( webSessionAction == 2 ) {
        webSessionAction = 0; Cvar_Set( "cg_hudOptionsOpen", "0" );
        Key_SetCatcher( Key_GetCatcher() & ~KEYCATCH_CGAME );
        if ( cgvm ) VM_Call( cgvm, CG_EVENT_HANDLING, CGAME_EVENT_NONE );
    }
	if ( webCloseRequested || !Web_CurrentScreen() || !IsWindow( webParent ) ) {
		webCloseRequested = qfalse;
		webSetup.pending = qfalse;
        Cvar_Set( "cl_webIngameUI", "0" ); Cvar_Set( "cl_webHUDActive", "0" ); Cvar_Set( "cl_webMapLoading", "0" );
        if ( webChild ) ShowWindow( webChild, SW_HIDE );
		Web_Close();
		return;
	}
	if ( webChild && GetClientRect( webParent, &parent ) && GetClientRect( webChild, &child ) &&
	     (parent.right != child.right || parent.bottom != child.bottom) ) {
		SetWindowPos( webChild, NULL, 0, 0, parent.right, parent.bottom, SWP_NOACTIVATE | SWP_NOZORDER );
	}
    if ( Web_CurrentScreen() == 14 ) {
        cef_frame_t *frame;
        Cvar_Set( "cl_webMapLoading", "1" );
        Cvar_Set( "cl_webIngameUI", "0" ); Cvar_Set( "cl_webHUDActive", "0" );
        webGarageMode = qfalse;
        Web_UpdateDemoRegion( qfalse );
        if ( webHUDRegion ) { SetWindowRgn( webChild, NULL, TRUE ); webHUDPublished = webHUDRectValid = webHUDRegion = qfalse; }
        if ( webLastState != 14 ) {
            frame = webBrowser->get_main_frame( webBrowser );
            Web_RunScript( frame, L"window.q3rallyFrontend?.state(14,0,0,0)" );
            frame->base.release( &frame->base ); webLastState = 14;
        }
        Web_PublishMapLoading();
        return;
    }
    Web_ProfileFrame();
    if ( uivm && VM_Call( uivm, UI_WEB_SCREEN ) == 7 && webMenuAction != 14 ) webGarageMode = qtrue;
    if ( !webGarageMode && uivm ) {
        if ( webCatalogAction >= 0 ) {
            VM_Call( uivm, UI_WEB_CATALOG_ACTION, webCatalogActionScreen, webCatalogAction, webCatalogIndex );
            webCatalogAction = -1;
        }
        int screen, progress = 0, stage = 0, prompt = 0, state;
        while ( webConfigRead < webConfigWrite ) {
            int slot = webConfigRead++ % ARRAY_LEN( webConfigQueue );
            Cvar_Set( "ui_webconfig_text", webConfigQueue[slot].text );
            VM_Call( uivm, UI_WEB_CONFIG_ACTION, webConfigQueue[slot].action, webConfigQueue[slot].index, webConfigQueue[slot].value ); webConfigPublished = qfalse;
        }
        webConfigRead = webConfigWrite = 0;
        while ( webOnlineRead < webOnlineWrite ) {
            int slot = webOnlineRead++ % ARRAY_LEN( webOnlineQueue );
            if ( webOnlineQueue[slot].action == 100 ) {
                Cvar_Set( "ui_weboffline_text", webOnlineQueue[slot].text );
                VM_Call( uivm, UI_WEB_OFFLINE_ACTION, 7, webOnlineQueue[slot].index, 0 ); webOfflineScreen = 0;
            } else {
                Cvar_Set( "ui_webonline_address", webOnlineQueue[slot].text );
                VM_Call( uivm, UI_WEB_ONLINE_ACTION, webOnlineQueue[slot].action, webOnlineQueue[slot].index, webOnlineQueue[slot].value ); webOnlinePublished = qfalse;
            }
        }
        webOnlineRead = webOnlineWrite = 0;
        while ( webOfflineRead < webOfflineWrite ) {
            int slot = webOfflineRead++ % ARRAY_LEN( webOfflineQueue );
            VM_Call( uivm, UI_WEB_OFFLINE_ACTION, webOfflineQueue[slot].action, webOfflineQueue[slot].index, webOfflineQueue[slot].value );
            webOfflineScreen = 0;
        }
        webOfflineRead = webOfflineWrite = 0;
        if ( webDriverAction >= 0 ) {
            int action = webDriverAction;
            VM_Call( uivm, UI_WEB_DRIVER_ACTION, action );
            webDriverAction = -1;
            if ( action >= 4 ) {
                cef_frame_t *frame;
                char message[256];
                wchar_t encoded[800], script[1000];
                frame = webBrowser->get_main_frame( webBrowser );
                if ( action >= 5 && Cvar_VariableIntegerValue( "ui_webdriver_ok" ) ) Web_LoadEnd( NULL, webBrowser, frame, 200 );
                Web_PublishDriver();
                Cvar_VariableStringBuffer( "ui_webdriver_message", message, sizeof( message ) );
                Web_QuotedBytes( message, encoded, ARRAY_LEN( encoded ) );
                swprintf( script, ARRAY_LEN( script ), L"window.q3rallyDriver?.result(decodeURIComponent('%ls'),%d)", encoded, Cvar_VariableIntegerValue( "ui_webdriver_ok" ) );
                Web_RunScript( frame, script );
                frame->base.release( &frame->base );
            }
        }
        if ( webMenuAction ) {
            VM_Call( uivm, UI_WEB_ACTION, webMenuAction );
            webMenuAction = 0;
        }
        if ( webDemoAction >= 0 ) {
            VM_Call( uivm, UI_WEB_DEMO_ACTION, webDemoAction, webDemoIndex );
            webDemoAction = -1;
        }
        screen = Web_CurrentScreen();
        if ( screen < 1 || screen > 14 ) {
            Web_Close();
            return;
        }
        Cvar_Set( "cl_webIngameUI", clc.state == CA_ACTIVE && screen == 12 ? "1" : "0" );
        Cvar_Set( "cl_webHUDActive", screen == 13 ? "1" : "0" );
        Cvar_Set( "cl_webMapLoading", screen == 14 ? "1" : "0" );
        if ( screen == 14 ) Web_PublishMapLoading();
        if ( screen == 12 ) { if ( !webConfigPublished || webConfigRevision != VM_Call( uivm, UI_WEB_CONFIG_NUMBER, 9, 0 ) ) Web_PublishConfig(); }
        else webConfigPublished = qfalse;
        if ( screen == 11 ) {
            if ( !webOnlinePublished || cls.realtime - webLastOnlinePoll >= 100 ) {
                VM_Call( uivm, UI_WEB_ONLINE_TICK, cls.realtime ); webLastOnlinePoll = cls.realtime;
                if ( !webOnlinePublished || webOnlineRevision != VM_Call( uivm, UI_WEB_ONLINE_NUMBER, 0, 0 ) ) {
                    Web_PublishOnline(); webOnlineRevision = VM_Call( uivm, UI_WEB_ONLINE_NUMBER, 0, 0 );
                }
            }
        } else webOnlinePublished = qfalse;
        if ( screen != 9 && screen != 10 ) webOfflineScreen = 0;
        else if ( screen != webOfflineScreen ) Web_PublishOffline( screen );
        if ( screen != 8 ) webDriverPublished = qfalse;
        if ( screen == 8 && !webDriverPublished ) Web_PublishDriver();
        if ( screen == 1 ) {
            if ( CL_WebGarageSkipBackground() ) VM_Call( uivm, UI_WEB_TICK, cls.realtime );
            screen = VM_Call( uivm, UI_WEB_SCREEN );
            progress = VM_Call( uivm, UI_WEB_LOAD_PROGRESS );
            stage = VM_Call( uivm, UI_WEB_LOAD_STAGE );
            prompt = VM_Call( uivm, UI_WEB_UPDATE_PROMPT );
        }
        if ( (screen == 3 || screen == 4) && webItemsScreen != screen ) Web_PublishItems( screen );
        Web_UpdateDemoRegion( screen == 4 && webDemoRectValid && clc.demoPlayer &&
                              clc.state == CA_ACTIVE && !clc.demoSeeking );
        if ( screen == 13 ) {
            if ( webHUDAction >= 0 && cgvm ) { VM_Call( cgvm, CG_WEB_HUD_ACTION, webHUDAction, webHUDIndex, webHUDValue ); webHUDAction = -1; webHUDPublished = qfalse; }
            if ( !webHUDPublished ) Web_PublishHUD();
            if ( webHUDRectValid && (!webHUDRegion || memcmp( &webHUDPreviousRect, &webHUDRect, sizeof( RECT ) )) ) { HRGN region = CreateRectRgn( webHUDRect.left, webHUDRect.top, webHUDRect.right, webHUDRect.bottom ); if ( !SetWindowRgn( webChild, region, TRUE ) ) DeleteObject( region ); webHUDPreviousRect = webHUDRect; webHUDRegion = qtrue; }
        } else if ( webHUDPublished ) { SetWindowRgn( webChild, NULL, TRUE ); webHUDPublished = webHUDRectValid = webHUDRegion = qfalse; }
        if ( screen == 4 && cls.realtime - webLastStatusReport >= 100 ) {
            cef_frame_t *frame = webBrowser->get_main_frame( webBrowser );
            wchar_t script[2200], encoded[1800];
            char message[512];
            Cvar_VariableStringBuffer( "cl_demoError", message, sizeof( message ) );
            if ( !message[0] ) Cvar_VariableStringBuffer( "cl_demoNotice", message, sizeof( message ) );
            Web_QuotedBytes( message, encoded, ARRAY_LEN( encoded ) );
            swprintf( script, ARRAY_LEN( script ),
                      L"window.q3rallyArchive?.playback(%d,%d,%d,%d,decodeURIComponent('%ls'))",
                      clc.demoPlayer, Cvar_VariableIntegerValue( "cl_freezeDemo" ), clc.demoSeeking,
                      Cvar_VariableIntegerValue( "cl_demoTime" ), encoded );
            Web_RunScript( frame, script );
            frame->base.release( &frame->base );
            webLastStatusReport = cls.realtime;
        }
        if ( screen == 6 && cls.realtime - webLastDownloadPoll >= 100 ) {
            VM_Call( uivm, UI_WEB_DOWNLOAD_TICK );
            webLastDownloadPoll = cls.realtime;
        }
        if ( (screen == 5 || screen == 6) &&
             (webCatalogScreen != screen || (screen == 6 && (webCatalogState != Cvar_VariableIntegerValue( "ui_dl_state" ) ||
                webCatalogRevision != VM_Call( uivm, UI_WEB_CATALOG_NUMBER, -1, 0 )))) )
            Web_PublishCatalog( screen );
        if ( screen == 6 && cls.realtime - webLastStatusReport >= 100 ) {
            Web_PublishDownloadState();
            webLastStatusReport = cls.realtime;
        }
        state = screen + progress * 4 + stage * 512 + prompt * 8192;
        if ( state != webLastState && cls.realtime - webLastReport >= 50 ) {
            cef_frame_t *frame = webBrowser->get_main_frame( webBrowser );
            wchar_t script[180];
            swprintf( script, ARRAY_LEN( script ), L"window.q3rallyFrontend?.state(%d,%d,%d,%d)",
                      screen, progress, stage, prompt );
            Web_RunScript( frame, script );
            frame->base.release( &frame->base );
            webLastState = state;
            webLastReport = cls.realtime;
        }
    } else if ( webGarageMode && webLastState != -2 ) {
        cef_frame_t *frame = webBrowser->get_main_frame( webBrowser );
        Web_RunScript( frame, L"window.q3rallyFrontend?.garage()" );
        frame->base.release( &frame->base );
        webLastState = -2;
    }
	if ( webSetup.pending ) {
		cef_frame_t *frame = webBrowser->get_main_frame( webBrowser );
		char model[MAX_QPATH];
		qboolean ok = qfalse;
		int rank = Web_UnlockedRank();
		webSetup.pending = qfalse;
		if ( webSetup.vehicle[0] && strlen( webSetup.vehicle ) + strlen( webSetup.paint ) + 1 < sizeof( model ) &&
		     Web_AssetUnlocked( webCarUnlocks, ARRAY_LEN( webCarUnlocks ), webSetup.vehicle, rank ) &&
		     Web_AssetUnlocked( webSkinUnlocks, ARRAY_LEN( webSkinUnlocks ), webSetup.paint, rank ) &&
		     Web_SetupInCatalog() ) {
			Com_sprintf( model, sizeof( model ), "%s/%s", webSetup.vehicle, webSetup.paint );
			ok = FS_ReadFile( va( "models/players/%s.skin", model ), NULL ) > 0 &&
			     FS_ReadFile( va( "models/players/wheels/%s.skin", webSetup.wheels ), NULL ) > 0 &&
                 FS_ReadFile( va( "models/players/heads/%s.md3", webSetup.head ), NULL ) > 0 &&
                 (FS_ReadFile( va( "models/players/plates/%s.tga", webSetup.plate ), NULL ) > 0 ||
                  FS_ReadFile( va( "models/players/plates/%s.jpg", webSetup.plate ), NULL ) > 0);
            if ( ok && webSetup.slot >= 0 ) {
                Cvar_Set( "ui_webFavoriteSetup", va( "%s %s %s %s %s", webSetup.vehicle,
                          webSetup.paint, webSetup.wheels, webSetup.head, webSetup.plate ) );
                ok = uivm && VM_Call( uivm, UI_WEB_FAVORITE_SAVE, webSetup.slot );
                Cvar_Set( "ui_webFavoriteSetup", "" );
            } else if ( ok ) {
                Cvar_Set( "model", model );
                Cvar_Set( "rim", webSetup.wheels );
                Cvar_Set( "head", webSetup.head );
                Cvar_Set( "plate", webSetup.plate );
            }
		}
		if ( frame ) {
            if ( webSetup.slot >= 0 ) {
                wchar_t script[120];
                swprintf( script, ARRAY_LEN( script ), L"window.q3rallyGarage?.savedFavorite(%d,%d)", webSetup.slot, ok );
                Web_RunScript( frame, script );
                if ( ok ) Web_PublishFavorites( frame );
            } else Web_RunScript( frame, ok ? L"window.q3rallyGarage?.result(true)" : L"window.q3rallyGarage?.result(false)" );
			frame->base.release( &frame->base );
		}
	}
}

void CL_WebGarageShutdown( void ) {
	if ( !webInitialized ) return;
	Web_Close();
	while ( webBrowser ) {
		cef_do_message_loop_work();
		Sleep( 1 );
	}
	cef_shutdown();
	webInitialized = qfalse;
}
