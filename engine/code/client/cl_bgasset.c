#include "client.h"
#include "cl_bgasset.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define CL_BGASSET_CACHE_DIR BASEGAME "/ui_cache"
#define CL_BGASSET_CACHE_INDEX CL_BGASSET_CACHE_DIR "/cache_index.txt"
#define CL_BGASSET_CACHE_PREFIX CL_BGASSET_CACHE_DIR "/"
#define CL_BGASSET_MAX_URL 512
#define CL_BGASSET_MAX_ETAG 128
#define CL_BGASSET_MAX_LASTMOD 128
#define CL_BGASSET_MAX_TIMESTAMP 32
#define CL_BGASSET_MAX_ENTRIES 64
#define CL_BGASSET_MAX_EXT 8
#define CL_BGASSET_MAX_CACHE_FILE_SIZE 32768
#define CL_BGASSET_DEFAULT_MAX_SIZE (5 * 1024 * 1024)

typedef enum {
	CL_BGASSET_JOB_NONE,
	CL_BGASSET_JOB_DOWNLOAD
} clBgAssetJobType_t;

typedef struct {
	char url[CL_BGASSET_MAX_URL];
	char localPath[MAX_OSPATH];
	char etag[CL_BGASSET_MAX_ETAG];
	char lastModified[CL_BGASSET_MAX_LASTMOD];
	unsigned checksum;
	char timestamp[CL_BGASSET_MAX_TIMESTAMP];
} clBgAssetCacheEntry_t;

typedef struct {
	qboolean registered;
	qboolean attempted;
	qboolean running;
	clBgAssetJobType_t jobType;
	char lastURL[CL_BGASSET_MAX_URL];
	char localPath[MAX_OSPATH];
	char tempPath[MAX_OSPATH];
	char etag[CL_BGASSET_MAX_ETAG];
	char lastModified[CL_BGASSET_MAX_LASTMOD];
	char newEtag[CL_BGASSET_MAX_ETAG];
	char newLastModified[CL_BGASSET_MAX_LASTMOD];
	int maxSize;
	int bytesWritten;
	qboolean sizeExceeded;
	fileHandle_t file;
#ifdef USE_CURL
	char errorText[CURL_ERROR_SIZE];
	CURL *easyHandle;
	CURLM *multiHandle;
	struct curl_slist *headers;
#endif
} clBgAssetContext_t;

static clBgAssetContext_t cl_bgasset;
static cvar_t *cl_uiBackgroundURL;
static cvar_t *cl_uiBackgroundEnable;
static cvar_t *cl_uiBackgroundMaxSize;
static cvar_t *cl_uiBackgroundState;
static cvar_t *cl_uiBackgroundError;

static void CL_BGAsset_SetState(const char *state) {
	if (cl_uiBackgroundState) {
		Cvar_Set(cl_uiBackgroundState->name, state);
	}
}

static void CL_BGAsset_SetError(const char *message) {
	if (cl_uiBackgroundError) {
		Cvar_Set(cl_uiBackgroundError->name, message ? message : "");
	}
}

static void CL_BGAsset_ResetState(void) {
	cl_bgasset.attempted = qfalse;
	cl_bgasset.running = qfalse;
	cl_bgasset.jobType = CL_BGASSET_JOB_NONE;
	cl_bgasset.lastURL[0] = '\0';
	cl_bgasset.localPath[0] = '\0';
	cl_bgasset.tempPath[0] = '\0';
	cl_bgasset.etag[0] = '\0';
	cl_bgasset.lastModified[0] = '\0';
	cl_bgasset.newEtag[0] = '\0';
	cl_bgasset.newLastModified[0] = '\0';
	cl_bgasset.maxSize = CL_BGASSET_DEFAULT_MAX_SIZE;
	cl_bgasset.bytesWritten = 0;
	cl_bgasset.sizeExceeded = qfalse;
	cl_bgasset.file = 0;
#ifdef USE_CURL
	cl_bgasset.errorText[0] = '\0';
	cl_bgasset.easyHandle = NULL;
	cl_bgasset.multiHandle = NULL;
	cl_bgasset.headers = NULL;
#endif
	CL_BGAsset_SetState("idle");
	CL_BGAsset_SetError("");
}

static qboolean CL_BGAsset_IsSafeRelativePath(const char *path) {
	if (!path || !*path) {
		return qfalse;
	}
	if (path[0] == '/' || path[0] == '\\') {
		return qfalse;
	}
	if (strstr(path, "..")) {
		return qfalse;
	}
	if (strchr(path, ':')) {
		return qfalse;
	}
	return qtrue;
}

static void CL_BGAsset_BuildTimestamp(char *buffer, size_t bufferSize) {
	qtime_t now;
	Com_RealTime(&now);
	Com_sprintf(buffer, bufferSize, "%04d-%02d-%02dT%02d:%02d:%02d",
		1900 + now.tm_year, 1 + now.tm_mon, now.tm_mday,
		now.tm_hour, now.tm_min, now.tm_sec);
}

static qboolean CL_BGAsset_CopyField(const char **cursor, char *dest, size_t destSize) {
	const char *tab;
	size_t len;

	if (!cursor || !*cursor || !dest || destSize == 0) {
		return qfalse;
	}

	tab = strchr(*cursor, '\t');
	len = tab ? (size_t)(tab - *cursor) : strlen(*cursor);
	if (len >= destSize) {
		len = destSize - 1;
	}
	Com_Memcpy(dest, *cursor, len);
	dest[len] = '\0';
	if (tab) {
		*cursor = tab + 1;
	} else {
		*cursor += len;
	}
	return qtrue;
}

static qboolean CL_BGAsset_ParseCacheLine(const char *line, clBgAssetCacheEntry_t *entry) {
	const char *cursor;
	char checksumField[32];

	if (!line || !*line) {
		return qfalse;
	}

	cursor = line;
	if (!CL_BGAsset_CopyField(&cursor, entry->url, sizeof(entry->url))) {
		return qfalse;
	}
	if (!CL_BGAsset_CopyField(&cursor, entry->localPath, sizeof(entry->localPath))) {
		return qfalse;
	}
	if (!CL_BGAsset_CopyField(&cursor, entry->etag, sizeof(entry->etag))) {
		return qfalse;
	}
	if (!CL_BGAsset_CopyField(&cursor, entry->lastModified, sizeof(entry->lastModified))) {
		return qfalse;
	}
	if (!CL_BGAsset_CopyField(&cursor, checksumField, sizeof(checksumField))) {
		return qfalse;
	}
	if (!CL_BGAsset_CopyField(&cursor, entry->timestamp, sizeof(entry->timestamp))) {
		return qfalse;
	}

	entry->checksum = (unsigned)strtoul(checksumField, NULL, 16);

	return qtrue;
}

static void CL_BGAsset_LoadCacheIndex(clBgAssetCacheEntry_t *entries, int *entryCount) {
	fileHandle_t file;
	int length;
	char *buffer;
	char *line;
	char *next;
	int count = 0;

	*entryCount = 0;

	length = FS_SV_FOpenFileRead(CL_BGASSET_CACHE_INDEX, &file);
	if (length <= 0) {
		return;
	}

	if (length > CL_BGASSET_MAX_CACHE_FILE_SIZE) {
		length = CL_BGASSET_MAX_CACHE_FILE_SIZE;
	}

	buffer = Z_Malloc(length + 1);
	FS_Read(buffer, length, file);
	FS_FCloseFile(file);
	buffer[length] = '\0';

	line = buffer;
	while (line && *line && count < CL_BGASSET_MAX_ENTRIES) {
		clBgAssetCacheEntry_t entry;

		next = strchr(line, '\n');
		if (next) {
			*next = '\0';
			next++;
		}
		if (*line == '#') {
			line = next;
			continue;
		}
		if (line[0] == '\0') {
			line = next;
			continue;
		}
		if (line[strlen(line) - 1] == '\r') {
			line[strlen(line) - 1] = '\0';
		}
		if (CL_BGAsset_ParseCacheLine(line, &entry)) {
			if (CL_BGAsset_IsSafeRelativePath(entry.localPath) &&
				!Q_stricmpn(entry.localPath, CL_BGASSET_CACHE_PREFIX, strlen(CL_BGASSET_CACHE_PREFIX))) {
				entries[count++] = entry;
			}
		}
		line = next;
	}

	Z_Free(buffer);
	*entryCount = count;
}

static void CL_BGAsset_SaveCacheIndex(const clBgAssetCacheEntry_t *entries, int entryCount) {
	fileHandle_t file;
	int i;
	const char *header = "# url\tlocal_path\tetag\tlast_modified\tcrc\ttimestamp\n";

	file = FS_SV_FOpenFileWrite(CL_BGASSET_CACHE_INDEX);
	if (!file) {
		return;
	}

	FS_Write(header, (int)strlen(header), file);
	for (i = 0; i < entryCount; i++) {
		char line[1024];
		Com_sprintf(line, sizeof(line), "%s\t%s\t%s\t%s\t%08x\t%s\n",
			entries[i].url,
			entries[i].localPath,
			entries[i].etag,
			entries[i].lastModified,
			entries[i].checksum,
			entries[i].timestamp);
		FS_Write(line, (int)strlen(line), file);
	}
	FS_FCloseFile(file);
}

static qboolean CL_BGAsset_GetCachedEntry(const char *url, clBgAssetCacheEntry_t *outEntry) {
	clBgAssetCacheEntry_t entries[CL_BGASSET_MAX_ENTRIES];
	int count;
	int i;

	CL_BGAsset_LoadCacheIndex(entries, &count);
	for (i = 0; i < count; i++) {
		if (!Q_stricmp(entries[i].url, url)) {
			if (outEntry) {
				*outEntry = entries[i];
			}
			return qtrue;
		}
	}
	return qfalse;
}

static void CL_BGAsset_UpdateCacheEntry(const char *url, const char *localPath, const char *etag,
	const char *lastModified, unsigned checksum) {
	clBgAssetCacheEntry_t entries[CL_BGASSET_MAX_ENTRIES];
	int count;
	int i;
	qboolean updated = qfalse;

	CL_BGAsset_LoadCacheIndex(entries, &count);
	for (i = 0; i < count; i++) {
		if (!Q_stricmp(entries[i].url, url)) {
			Q_strncpyz(entries[i].localPath, localPath, sizeof(entries[i].localPath));
			Q_strncpyz(entries[i].etag, etag, sizeof(entries[i].etag));
			Q_strncpyz(entries[i].lastModified, lastModified, sizeof(entries[i].lastModified));
			entries[i].checksum = checksum;
			CL_BGAsset_BuildTimestamp(entries[i].timestamp, sizeof(entries[i].timestamp));
			updated = qtrue;
			break;
		}
	}

	if (!updated && count < CL_BGASSET_MAX_ENTRIES) {
		Q_strncpyz(entries[count].url, url, sizeof(entries[count].url));
		Q_strncpyz(entries[count].localPath, localPath, sizeof(entries[count].localPath));
		Q_strncpyz(entries[count].etag, etag, sizeof(entries[count].etag));
		Q_strncpyz(entries[count].lastModified, lastModified, sizeof(entries[count].lastModified));
		entries[count].checksum = checksum;
		CL_BGAsset_BuildTimestamp(entries[count].timestamp, sizeof(entries[count].timestamp));
		count++;
	}

	CL_BGAsset_SaveCacheIndex(entries, count);
}

static qboolean CL_BGAsset_ExtractExtension(const char *url, char *ext, size_t extSize) {
	const char *query;
	const char *end;
	const char *dot;
	size_t len;
	size_t i;

	ext[0] = '\0';
	query = strpbrk(url, "?#");
	end = query ? query : url + strlen(url);
	dot = strrchr(url, '.');
	if (!dot || dot >= end || dot == end - 1) {
		return qfalse;
	}

	len = (size_t)(end - dot - 1);
	if (len == 0 || len >= extSize || len > CL_BGASSET_MAX_EXT) {
		return qfalse;
	}
	for (i = 0; i < len; i++) {
		if (!isalnum((unsigned char)dot[1 + i])) {
			return qfalse;
		}
	}
	Q_strncpyz(ext, dot + 1, extSize);
	return qtrue;
}

static void CL_BGAsset_GenerateLocalPath(const char *url, char *path, size_t pathSize) {
	char ext[CL_BGASSET_MAX_EXT + 1];
	unsigned hash = Com_BlockChecksum(url, (int)strlen(url));

	if (!CL_BGAsset_ExtractExtension(url, ext, sizeof(ext))) {
		Q_strncpyz(ext, "dat", sizeof(ext));
	}

	Com_sprintf(path, pathSize, "%s/bg_%08x.%s", CL_BGASSET_CACHE_DIR, hash, ext);
}

static unsigned CL_BGAsset_ComputeChecksum(const char *path, int maxSize) {
	fileHandle_t file;
	int length;
	unsigned checksum = 0;
	byte *buffer;

	length = FS_SV_FOpenFileRead(path, &file);
	if (length <= 0) {
		return 0;
	}
	if (length > maxSize) {
		FS_FCloseFile(file);
		return 0;
	}

	buffer = Z_Malloc(length);
	FS_Read(buffer, length, file);
	FS_FCloseFile(file);
	checksum = Com_BlockChecksum(buffer, length);
	Z_Free(buffer);
	return checksum;
}

static qboolean CL_BGAsset_IsUrlAllowed(const char *url) {
	if (!url || !*url) {
		return qfalse;
	}
	if (!Q_stricmpn(url, "http://", 7) || !Q_stricmpn(url, "https://", 8)) {
		return qtrue;
	}
	return qfalse;
}

#ifdef USE_CURL
static void CL_BGAsset_ClearHeaders(void) {
	if (cl_bgasset.headers) {
		qcurl_slist_free_all(cl_bgasset.headers);
		cl_bgasset.headers = NULL;
	}
}

static void CL_BGAsset_CleanupHandles(void) {
	if (cl_bgasset.multiHandle && cl_bgasset.easyHandle) {
		qcurl_multi_remove_handle(cl_bgasset.multiHandle, cl_bgasset.easyHandle);
	}
	if (cl_bgasset.easyHandle) {
		qcurl_easy_cleanup(cl_bgasset.easyHandle);
		cl_bgasset.easyHandle = NULL;
	}
	if (cl_bgasset.multiHandle) {
		qcurl_multi_cleanup(cl_bgasset.multiHandle);
		cl_bgasset.multiHandle = NULL;
	}
	CL_BGAsset_ClearHeaders();
	cl_bgasset.running = qfalse;
	cl_bgasset.jobType = CL_BGASSET_JOB_NONE;
}

static void CL_BGAsset_Fail(const char *state, const char *message) {
	CL_BGAsset_SetState(state && *state ? state : "failed");
	CL_BGAsset_SetError(message && *message ? message : "Download failed");
	if (cl_bgasset.file) {
		FS_FCloseFile(cl_bgasset.file);
		cl_bgasset.file = 0;
	}
	CL_BGAsset_CleanupHandles();
}

static size_t CL_BGAsset_WriteCallback(void *buffer, size_t size, size_t nmemb, void *userdata) {
	clBgAssetContext_t *ctx = (clBgAssetContext_t *)userdata;
	size_t bytes = size * nmemb;

	if (!ctx || ctx->sizeExceeded) {
		return 0;
	}

	if (ctx->bytesWritten + (int)bytes > ctx->maxSize) {
		ctx->sizeExceeded = qtrue;
		return 0;
	}

	if (!ctx->file) {
		ctx->file = FS_SV_FOpenFileWrite(ctx->tempPath);
		if (!ctx->file) {
			ctx->sizeExceeded = qtrue;
			return 0;
		}
	}

	FS_Write(buffer, (int)bytes, ctx->file);
	ctx->bytesWritten += (int)bytes;
	return bytes;
}

static size_t CL_BGAsset_HeaderCallback(void *buffer, size_t size, size_t nmemb, void *userdata) {
	clBgAssetContext_t *ctx = (clBgAssetContext_t *)userdata;
	size_t bytes = size * nmemb;
	char headerLine[256];
	char *line;
	char *value;
	size_t len;

	if (!ctx || bytes == 0) {
		return bytes;
	}

	len = bytes;
	if (len >= sizeof(headerLine)) {
		len = sizeof(headerLine) - 1;
	}
	Com_Memcpy(headerLine, buffer, len);
	headerLine[len] = '\0';

	line = headerLine;
	while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == '\n')) {
		line[len - 1] = '\0';
		len--;
	}
	if (len == 0) {
		return bytes;
	}

	if (!Q_stricmpn(line, "ETag:", 5)) {
		value = line + 5;
		while (*value && isspace((unsigned char)*value)) {
			value++;
		}
		Q_strncpyz(ctx->newEtag, value, sizeof(ctx->newEtag));
	} else if (!Q_stricmpn(line, "Last-Modified:", 14)) {
		value = line + 14;
		while (*value && isspace((unsigned char)*value)) {
			value++;
		}
		Q_strncpyz(ctx->newLastModified, value, sizeof(ctx->newLastModified));
	} else if (!Q_stricmpn(line, "Content-Length:", 15)) {
		value = line + 15;
		while (*value && isspace((unsigned char)*value)) {
			value++;
		}
		if (*value) {
			long length = strtol(value, NULL, 10);
			if (length > 0 && length > ctx->maxSize) {
				ctx->sizeExceeded = qtrue;
				return 0;
			}
		}
	}

	return bytes;
}

static void CL_BGAsset_StartDownload(const char *url) {
	clBgAssetCacheEntry_t cachedEntry;
	qboolean hasCache = qfalse;
	const char *localPath = NULL;
	char generatedPath[MAX_OSPATH];
	int maxSize;

	if (!CL_cURL_Init()) {
		CL_BGAsset_Fail("failed", "cURL initialization failed");
		return;
	}

	CL_BGAsset_SetState("checking");
	CL_BGAsset_SetError("");
	cl_bgasset.bytesWritten = 0;
	cl_bgasset.sizeExceeded = qfalse;
	cl_bgasset.errorText[0] = '\0';
	cl_bgasset.file = 0;
	cl_bgasset.newEtag[0] = '\0';
	cl_bgasset.newLastModified[0] = '\0';

	maxSize = cl_uiBackgroundMaxSize ? cl_uiBackgroundMaxSize->integer : CL_BGASSET_DEFAULT_MAX_SIZE;
	if (maxSize <= 0) {
		maxSize = CL_BGASSET_DEFAULT_MAX_SIZE;
	}
	cl_bgasset.maxSize = maxSize;

	if (CL_BGAsset_GetCachedEntry(url, &cachedEntry)) {
		hasCache = qtrue;
		if (!CL_BGAsset_IsSafeRelativePath(cachedEntry.localPath) ||
			Q_stricmpn(cachedEntry.localPath, CL_BGASSET_CACHE_PREFIX, strlen(CL_BGASSET_CACHE_PREFIX))) {
			hasCache = qfalse;
		} else if (!FS_SV_FileExists(cachedEntry.localPath)) {
			hasCache = qfalse;
		}
	}

	if (hasCache) {
		localPath = cachedEntry.localPath;
		Q_strncpyz(cl_bgasset.etag, cachedEntry.etag, sizeof(cl_bgasset.etag));
		Q_strncpyz(cl_bgasset.lastModified, cachedEntry.lastModified, sizeof(cl_bgasset.lastModified));
	} else {
		cl_bgasset.etag[0] = '\0';
		cl_bgasset.lastModified[0] = '\0';
	}

	if (!localPath) {
		CL_BGAsset_GenerateLocalPath(url, generatedPath, sizeof(generatedPath));
		localPath = generatedPath;
	}

	Q_strncpyz(cl_bgasset.localPath, localPath, sizeof(cl_bgasset.localPath));
	Com_sprintf(cl_bgasset.tempPath, sizeof(cl_bgasset.tempPath), "%s.tmp", cl_bgasset.localPath);

	cl_bgasset.easyHandle = qcurl_easy_init();
	if (!cl_bgasset.easyHandle) {
		CL_BGAsset_Fail("failed", "Failed to create cURL easy handle");
		return;
	}

	cl_bgasset.multiHandle = qcurl_multi_init();
	if (!cl_bgasset.multiHandle) {
		qcurl_easy_cleanup(cl_bgasset.easyHandle);
		cl_bgasset.easyHandle = NULL;
		CL_BGAsset_Fail("failed", "Failed to create cURL multi handle");
		return;
	}

	if (cl_bgasset.etag[0]) {
		cl_bgasset.headers = qcurl_slist_append(cl_bgasset.headers,
			va("If-None-Match: %s", cl_bgasset.etag));
	}
	if (cl_bgasset.lastModified[0]) {
		cl_bgasset.headers = qcurl_slist_append(cl_bgasset.headers,
			va("If-Modified-Since: %s", cl_bgasset.lastModified));
	}

	qcurl_easy_setopt(cl_bgasset.easyHandle, CURLOPT_URL, url);
	qcurl_easy_setopt(cl_bgasset.easyHandle, CURLOPT_WRITEFUNCTION, CL_BGAsset_WriteCallback);
	qcurl_easy_setopt(cl_bgasset.easyHandle, CURLOPT_WRITEDATA, &cl_bgasset);
	qcurl_easy_setopt(cl_bgasset.easyHandle, CURLOPT_HEADERFUNCTION, CL_BGAsset_HeaderCallback);
	qcurl_easy_setopt(cl_bgasset.easyHandle, CURLOPT_HEADERDATA, &cl_bgasset);
	qcurl_easy_setopt(cl_bgasset.easyHandle, CURLOPT_ERRORBUFFER, cl_bgasset.errorText);
	qcurl_easy_setopt(cl_bgasset.easyHandle, CURLOPT_NOPROGRESS, 1L);
	qcurl_easy_setopt(cl_bgasset.easyHandle, CURLOPT_FOLLOWLOCATION, 1L);
	qcurl_easy_setopt(cl_bgasset.easyHandle, CURLOPT_MAXREDIRS, 4L);
	qcurl_easy_setopt(cl_bgasset.easyHandle, CURLOPT_TIMEOUT, 15L);
#ifdef CURLOPT_USERAGENT
	qcurl_easy_setopt(cl_bgasset.easyHandle, CURLOPT_USERAGENT, va("%s %s", PRODUCT_NAME, PRODUCT_VERSION));
#endif
	if (cl_bgasset.headers) {
		qcurl_easy_setopt(cl_bgasset.easyHandle, CURLOPT_HTTPHEADER, cl_bgasset.headers);
	}

	if (qcurl_multi_add_handle(cl_bgasset.multiHandle, cl_bgasset.easyHandle) != CURLM_OK) {
		CL_BGAsset_Fail("failed", "Failed to queue background asset download");
		return;
	}

	cl_bgasset.running = qtrue;
	cl_bgasset.jobType = CL_BGASSET_JOB_DOWNLOAD;
	CL_BGAsset_SetState("downloading");
}

static void CL_BGAsset_HandleComplete(CURLcode result) {
	long responseCode = 0;
	unsigned checksum = 0;
	const char *etag = cl_bgasset.newEtag[0] ? cl_bgasset.newEtag : cl_bgasset.etag;
	const char *lastModified = cl_bgasset.newLastModified[0] ? cl_bgasset.newLastModified : cl_bgasset.lastModified;

	qcurl_easy_getinfo(cl_bgasset.easyHandle, CURLINFO_RESPONSE_CODE, &responseCode);

	if (cl_bgasset.file) {
		FS_FCloseFile(cl_bgasset.file);
		cl_bgasset.file = 0;
	}

	if (cl_bgasset.sizeExceeded) {
		CL_BGAsset_Fail("failed", "Background asset exceeds size limit");
		return;
	}

	if (result != CURLE_OK) {
		const char *err = cl_bgasset.errorText[0] ? cl_bgasset.errorText : qcurl_easy_strerror(result);
		CL_BGAsset_Fail("failed", err);
		return;
	}

	if (responseCode == 304) {
		CL_BGAsset_UpdateCacheEntry(cl_bgasset.lastURL, cl_bgasset.localPath, etag, lastModified,
			CL_BGAsset_ComputeChecksum(cl_bgasset.localPath, cl_bgasset.maxSize));
		CL_BGAsset_SetState("cached");
		CL_BGAsset_SetError("");
		CL_BGAsset_CleanupHandles();
		return;
	}

	if (responseCode < 200 || responseCode >= 300) {
		CL_BGAsset_Fail("failed", va("HTTP error %ld", responseCode));
		return;
	}

	FS_SV_Rename(cl_bgasset.tempPath, cl_bgasset.localPath, qfalse);
	checksum = CL_BGAsset_ComputeChecksum(cl_bgasset.localPath, cl_bgasset.maxSize);
	CL_BGAsset_UpdateCacheEntry(cl_bgasset.lastURL, cl_bgasset.localPath, etag, lastModified, checksum);
	CL_BGAsset_SetState("ready");
	CL_BGAsset_SetError("");
	CL_BGAsset_CleanupHandles();
}
#endif

void CL_BGAsset_Register(void) {
	cl_uiBackgroundURL = Cvar_Get("cl_uiBackgroundURL", "", CVAR_ARCHIVE);
	cl_uiBackgroundEnable = Cvar_Get("cl_uiBackgroundEnable", "1", CVAR_ARCHIVE);
	cl_uiBackgroundMaxSize = Cvar_Get("cl_uiBackgroundMaxSize", va("%d", CL_BGASSET_DEFAULT_MAX_SIZE), CVAR_ARCHIVE);
	cl_uiBackgroundState = Cvar_Get("cl_uiBackgroundState", "idle", CVAR_TEMP);
	cl_uiBackgroundError = Cvar_Get("cl_uiBackgroundError", "", CVAR_TEMP);

	cl_bgasset.registered = qtrue;
	CL_BGAsset_ResetState();
}

void CL_BGAsset_Frame(void) {
#ifndef USE_CURL
	if (cl_uiBackgroundEnable && cl_uiBackgroundEnable->integer) {
		CL_BGAsset_SetState("unavailable");
		CL_BGAsset_SetError("cURL support not compiled");
	}
	return;
#else
	int runningHandles = 0;
	CURLMcode multiResult;

	if (!cl_bgasset.registered) {
		CL_BGAsset_Register();
	}

	if (!cl_uiBackgroundEnable || !cl_uiBackgroundEnable->integer) {
		return;
	}

	if (!cl_uiBackgroundURL || !*cl_uiBackgroundURL->string) {
		return;
	}

	if (!CL_BGAsset_IsUrlAllowed(cl_uiBackgroundURL->string)) {
		CL_BGAsset_SetState("failed");
		CL_BGAsset_SetError("Invalid URL scheme");
		return;
	}

	if (Q_stricmp(cl_bgasset.lastURL, cl_uiBackgroundURL->string)) {
		Q_strncpyz(cl_bgasset.lastURL, cl_uiBackgroundURL->string, sizeof(cl_bgasset.lastURL));
		cl_bgasset.attempted = qfalse;
	}

	if (!cl_bgasset.running && !cl_bgasset.attempted) {
		cl_bgasset.attempted = qtrue;
		CL_BGAsset_StartDownload(cl_bgasset.lastURL);
		return;
	}

	if (!cl_bgasset.running || !cl_bgasset.multiHandle) {
		return;
	}

	do {
		multiResult = qcurl_multi_perform(cl_bgasset.multiHandle, &runningHandles);
	} while (multiResult == CURLM_CALL_MULTI_PERFORM);

	if (multiResult != CURLM_OK) {
		CL_BGAsset_Fail("failed", qcurl_multi_strerror(multiResult));
		return;
	}

	if (runningHandles == 0) {
		int msgsInQueue;
		CURLMsg *msg;

		while ((msg = qcurl_multi_info_read(cl_bgasset.multiHandle, &msgsInQueue)) != NULL) {
			if (msg->easy_handle != cl_bgasset.easyHandle) {
				continue;
			}

			if (msg->msg == CURLMSG_DONE) {
				CL_BGAsset_HandleComplete(msg->data.result);
			}
		}
	}
#endif
}

void CL_BGAsset_Shutdown(void) {
#ifdef USE_CURL
	if (cl_bgasset.running || cl_bgasset.multiHandle || cl_bgasset.easyHandle) {
		if (cl_bgasset.file) {
			FS_FCloseFile(cl_bgasset.file);
			cl_bgasset.file = 0;
		}
		CL_BGAsset_CleanupHandles();
	}
#endif
	CL_BGAsset_ResetState();
}
