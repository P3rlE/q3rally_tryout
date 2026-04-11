#include "client.h"
#include "cl_bgasset.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define CL_BGASSET_CACHE_QDIR "ui_cache"
#define CL_BGASSET_CACHE_DIR BASEGAME "/" CL_BGASSET_CACHE_QDIR
#define CL_BGASSET_CACHE_INDEX CL_BGASSET_CACHE_DIR "/cache_index.txt"
#define CL_BGASSET_CACHE_PREFIX CL_BGASSET_CACHE_DIR "/"
#define CL_BGASSET_CACHE_QPREFIX CL_BGASSET_CACHE_QDIR "/"
#define CL_BGASSET_MAX_URL 512
#define CL_BGASSET_MAX_ETAG 128
#define CL_BGASSET_MAX_LASTMOD 128
#define CL_BGASSET_MAX_TIMESTAMP 32
#define CL_BGASSET_MAX_ENTRIES 64
#define CL_BGASSET_MAX_EXT 8
#define CL_BGASSET_MAX_CACHE_FILE_SIZE 32768
#define CL_BGASSET_DEFAULT_MAX_SIZE (5 * 1024 * 1024)
#define CL_BGASSET_MAX_DIMENSION 4096


/* ---------- minimal stored-ZIP / pk3 writer --------------------------------
 * Writes a single-file, uncompressed ZIP so that the renderer can find the
 * background image via the normal pk3 search path without a FS_Restart.
 * The format is: local-file-header | data | central-dir-header | EOCD.
 * All multi-byte fields are little-endian.
 * --------------------------------------------------------------------------*/

/* Standard CRC-32 for ZIP files (poly 0xEDB88320, reflected). */
static unsigned CL_BGAsset_CRC32( const byte *data, int len ) {
	unsigned crc = 0xFFFFFFFFu;
	int i, b;
	for ( i = 0; i < len; i++ ) {
		crc ^= data[i];
		for ( b = 0; b < 8; b++ ) {
			if ( crc & 1 ) crc = (crc >> 1) ^ 0xEDB88320u;
			else           crc >>= 1;
		}
	}
	return crc ^ 0xFFFFFFFFu;
}

/* Write a little-endian 16-bit value into buf at offset. */
static void CL_BGAsset_ZipPut16( byte *buf, int off, unsigned v ) {
	buf[off]   = (byte)( v        & 0xff );
	buf[off+1] = (byte)( (v >> 8) & 0xff );
}

/* Write a little-endian 32-bit value into buf at offset. */
static void CL_BGAsset_ZipPut32( byte *buf, int off, unsigned v ) {
	buf[off]   = (byte)( v         & 0xff );
	buf[off+1] = (byte)( (v >>  8) & 0xff );
	buf[off+2] = (byte)( (v >> 16) & 0xff );
	buf[off+3] = (byte)( (v >> 24) & 0xff );
}

/*
 * CL_BGAsset_WriteStoredPk3
 *
 * Reads the image file at <imagePath> (homepath-relative, e.g.
 * "baseq3r/ui_cache/bg_XXXX.png"), wraps it in a minimal stored-ZIP and
 * writes the result to <pk3Path> (also homepath-relative).  The entry name
 * inside the ZIP is <entryName> (the qpath the renderer will look up, e.g.
 * "ui_cache/bg_XXXX.png").
 *
 * Returns qtrue on success.
 */
static qboolean CL_BGAsset_WriteStoredPk3( const char *imagePath,
                                            const char *pk3SvPath,
                                            const char *entryName ) {
	fileHandle_t	srcFile;
	long		imgLen;
	byte		*imgBuf;
	byte		*zipBuf;
	int			zipSize;
	byte		*p;
	unsigned	crc;
	int			nameLen;
	unsigned	dataOffset;
	unsigned	cdrOffset;
	qboolean	ok;

	nameLen = (int)strlen( entryName );

	/* --- read the image into memory --- */
	imgLen = FS_SV_FOpenFileRead( imagePath, &srcFile );
	if ( imgLen <= 0 ) {
		Com_DPrintf( "CL_BGAsset_WriteStoredPk3: cannot read '%s'\n", imagePath );
		return qfalse;
	}
	imgBuf = Z_Malloc( imgLen );
	FS_Read( imgBuf, imgLen, srcFile );
	FS_FCloseFile( srcFile );

	/* --- compute CRC-32 (standard ZIP polynomial) --- */
	crc = CL_BGAsset_CRC32( imgBuf, (int)imgLen );

	/* --- allocate contiguous ZIP buffer --- */
	dataOffset = 30 + nameLen;
	cdrOffset  = dataOffset + (unsigned)imgLen;
	zipSize    = cdrOffset + 46 + nameLen + 22;
	zipBuf     = Z_Malloc( zipSize );
	Com_Memset( zipBuf, 0, zipSize );
	p = zipBuf;

	/* local file header */
	CL_BGAsset_ZipPut32( p,  0, 0x04034b50 );
	CL_BGAsset_ZipPut16( p,  4, 20 );
	CL_BGAsset_ZipPut32( p, 14, crc );
	CL_BGAsset_ZipPut32( p, 18, (unsigned)imgLen );
	CL_BGAsset_ZipPut32( p, 22, (unsigned)imgLen );
	CL_BGAsset_ZipPut16( p, 26, (unsigned short)nameLen );
	p += 30;
	Com_Memcpy( p, entryName, nameLen );
	p += nameLen;

	/* file data */
	Com_Memcpy( p, imgBuf, imgLen );
	p += imgLen;
	Z_Free( imgBuf );

	/* central directory header */
	CL_BGAsset_ZipPut32( p,  0, 0x02014b50 );
	CL_BGAsset_ZipPut16( p,  4, 20 );
	CL_BGAsset_ZipPut16( p,  6, 20 );
	CL_BGAsset_ZipPut32( p, 16, crc );
	CL_BGAsset_ZipPut32( p, 20, (unsigned)imgLen );
	CL_BGAsset_ZipPut32( p, 24, (unsigned)imgLen );
	CL_BGAsset_ZipPut16( p, 28, (unsigned short)nameLen );
	CL_BGAsset_ZipPut32( p, 42, 0 );
	p += 46;
	Com_Memcpy( p, entryName, nameLen );
	p += nameLen;

	/* end of central directory */
	CL_BGAsset_ZipPut32( p,  0, 0x06054b50 );
	CL_BGAsset_ZipPut16( p,  8, 1 );
	CL_BGAsset_ZipPut16( p, 10, 1 );
	CL_BGAsset_ZipPut32( p, 12, (unsigned)(46 + nameLen) );
	CL_BGAsset_ZipPut32( p, 16, cdrOffset );

	/* write via dedicated function that bypasses the .pk3 write restriction */
	ok = FS_SV_WritePk3File( pk3SvPath, zipBuf, zipSize );
	Z_Free( zipBuf );
	if ( ok ) {
		Com_Printf( "BGASSET DEBUG: wrote pk3 '%s' (%d bytes, entry='%s', imgLen=%ld)\n",
			pk3SvPath, zipSize, entryName, imgLen );
	} else {
		Com_Printf( "BGASSET DEBUG: FAILED to write pk3 '%s'\n", pk3SvPath );
	}
	return ok;
}

/* Build the OS path/* Build the OS path for the generated pk3 given the image's local path. */
static void CL_BGAsset_GetPk3Path( const char *localPath,
                                    char *pk3SvPath, size_t pk3SvSize,
                                    char *pk3OsPath, size_t pk3OsSize ) {
	const char	*homepath;
	char		stripped[MAX_OSPATH];
	char		*dot;

	/* Strip extension from localPath to form pk3 filename, e.g.
	 * "baseq3r/ui_cache/bg_XXXX.png" -> "baseq3r/ui_cache/bg_XXXX.pk3" */
	Q_strncpyz( stripped, localPath, sizeof(stripped) );
	dot = strrchr( stripped, '.' );
	if ( dot && dot > strrchr( stripped, '/' ) ) {
		*dot = '\0';
	}
	Com_sprintf( pk3SvPath, pk3SvSize, "%s.pk3", stripped );

	homepath = Cvar_VariableString( "fs_homepath" );
	Com_sprintf( pk3OsPath, pk3OsSize, "%s/%s", homepath, pk3SvPath );
	/* Normalise path separators for the current platform */
	{
		char *p;
		for ( p = pk3OsPath; *p; p++ ) {
			if ( *p == '/' || *p == '\\' ) {
				*p = PATH_SEP;
			}
		}
	}
}

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
	int nextRefreshTime;
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
static cvar_t *ui_menuBackUrl;
static cvar_t *ui_menuBackEnable;
static cvar_t *ui_menuBackRefreshSec;
static cvar_t *ui_menuBackPath;
static cvar_t *ui_menuBackState;
static cvar_t *ui_menuBackError;

static void CL_BGAsset_SetState(const char *state) {
	if (ui_menuBackState) {
		Cvar_Set(ui_menuBackState->name, state);
	}
	if (cl_uiBackgroundState) {
		Cvar_Set(cl_uiBackgroundState->name, state);
	}
}

static void CL_BGAsset_SetError(const char *message) {
	if (ui_menuBackError) {
		Cvar_Set(ui_menuBackError->name, message ? message : "");
	}
	if (cl_uiBackgroundError) {
		Cvar_Set(cl_uiBackgroundError->name, message ? message : "");
	}
}

static void CL_BGAsset_SetPath(const char *path) {
	if (ui_menuBackPath) {
		Cvar_Set(ui_menuBackPath->name, path ? path : "");
	}
}

static const char *CL_BGAsset_GetURL(void) {
	if (ui_menuBackUrl && ui_menuBackUrl->string[0]) {
		return ui_menuBackUrl->string;
	}
	if (cl_uiBackgroundURL && cl_uiBackgroundURL->string[0]) {
		return cl_uiBackgroundURL->string;
	}
	return "";
}

static qboolean CL_BGAsset_IsEnabled(void) {
	if (ui_menuBackEnable) {
		return ui_menuBackEnable->integer != 0;
	}
	if (cl_uiBackgroundEnable) {
		return cl_uiBackgroundEnable->integer != 0;
	}
	return qfalse;
}

static int CL_BGAsset_GetRefreshSec(void) {
	if (ui_menuBackRefreshSec) {
		return ui_menuBackRefreshSec->integer;
	}
	return 0;
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
	cl_bgasset.nextRefreshTime = 0;
#ifdef USE_CURL
	cl_bgasset.errorText[0] = '\0';
	cl_bgasset.easyHandle = NULL;
	cl_bgasset.multiHandle = NULL;
	cl_bgasset.headers = NULL;
#endif
	CL_BGAsset_SetState("idle");
	CL_BGAsset_SetError("");
	CL_BGAsset_SetPath("");
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

static qboolean CL_BGAsset_ToQPath(const char *localPath, char *qpath, size_t qpathSize) {

	if (!localPath || !localPath[0] || !qpath || qpathSize == 0) {
		return qfalse;
	}

	if (!Q_stricmpn(localPath, CL_BGASSET_CACHE_PREFIX, strlen(CL_BGASSET_CACHE_PREFIX))) {
		Com_sprintf(qpath, qpathSize, "%s", localPath + strlen(BASEGAME "/"));

	} else if (!Q_stricmpn(localPath, CL_BGASSET_CACHE_QPREFIX, strlen(CL_BGASSET_CACHE_QPREFIX))) {
		Q_strncpyz(qpath, localPath, qpathSize);
	} else {
		return qfalse;
	}

	// Keep the file extension in the qpath so that trap_R_RegisterShaderNoMip
	// can resolve the image directly. Previously the extension was stripped here,
	// which caused the renderer to search for a shader script named e.g.
	// "ui_cache/bg_82405e13" and fail with "Couldn't find image file for shader".

	return qtrue;
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

static void CL_BGAsset_ScheduleRefresh(void) {
	int refreshSec = CL_BGAsset_GetRefreshSec();

	if (refreshSec > 0) {
		cl_bgasset.nextRefreshTime = cls.realtime + (refreshSec * 1000);
	} else {
		cl_bgasset.nextRefreshTime = 0;
	}
}

static void CL_BGAsset_LogFailure(const char *message) {
	if (message && *message) {
		Com_Printf("Menu background: %s\n", message);
	} else {
		Com_Printf("Menu background: failed\n");
	}
}

static void CL_BGAsset_Fail(const char *state, const char *message) {
	CL_BGAsset_LogFailure(message && *message ? message : "Download failed");
	CL_BGAsset_SetState(state && *state ? state : "failed");
	CL_BGAsset_SetError(message && *message ? message : "Download failed");
	CL_BGAsset_SetPath("");
	if (cl_bgasset.tempPath[0]) {
		FS_HomeRemove(cl_bgasset.tempPath);
	}
	if (cl_bgasset.file) {
		FS_FCloseFile(cl_bgasset.file);
		cl_bgasset.file = 0;
	}
	CL_BGAsset_CleanupHandles();
	CL_BGAsset_ScheduleRefresh();
}

static int CL_BGAsset_ReadBE32(const byte *buffer) {
	return ((int)buffer[0] << 24) | ((int)buffer[1] << 16) | ((int)buffer[2] << 8) | (int)buffer[3];
}

static qboolean CL_BGAsset_ParsePNG(const byte *buffer, int length, int *width, int *height) {
	static const byte signature[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };

	if (length < 24) {
		return qfalse;
	}
	if (memcmp(buffer, signature, sizeof(signature)) != 0) {
		return qfalse;
	}
	if (memcmp(buffer + 12, "IHDR", 4) != 0) {
		return qfalse;
	}

	*width = CL_BGAsset_ReadBE32(buffer + 16);
	*height = CL_BGAsset_ReadBE32(buffer + 20);
	return qtrue;
}

static qboolean CL_BGAsset_IsSOFMarker(byte marker) {
	switch (marker) {
	case 0xC0:
	case 0xC1:
	case 0xC2:
	case 0xC3:
	case 0xC5:
	case 0xC6:
	case 0xC7:
	case 0xC9:
	case 0xCA:
	case 0xCB:
	case 0xCD:
	case 0xCE:
	case 0xCF:
		return qtrue;
	default:
		return qfalse;
	}
}

static qboolean CL_BGAsset_ParseJPG(const byte *buffer, int length, int *width, int *height) {
	int offset = 2;

	if (length < 4 || buffer[0] != 0xFF || buffer[1] != 0xD8) {
		return qfalse;
	}

	while (offset + 1 < length) {
		byte marker;
		int segmentLength;

		if (buffer[offset] != 0xFF) {
			offset++;
			continue;
		}
		while (offset < length && buffer[offset] == 0xFF) {
			offset++;
		}
		if (offset >= length) {
			break;
		}

		marker = buffer[offset++];
		if (marker == 0xD9 || marker == 0xDA) {
			break;
		}
		if (offset + 1 >= length) {
			break;
		}
		segmentLength = (buffer[offset] << 8) | buffer[offset + 1];
		if (segmentLength < 2 || offset + segmentLength > length) {
			break;
		}
		if (CL_BGAsset_IsSOFMarker(marker)) {
			if (segmentLength < 7) {
				return qfalse;
			}
			*height = (buffer[offset + 3] << 8) | buffer[offset + 4];
			*width = (buffer[offset + 5] << 8) | buffer[offset + 6];
			return qtrue;
		}

		offset += segmentLength;
	}

	return qfalse;
}

static qboolean CL_BGAsset_ParseTGA(const byte *buffer, int length, int *width, int *height) {
	byte imageType;
	byte pixelDepth;

	if (length < 18) {
		return qfalse;
	}

	imageType = buffer[2];
	pixelDepth = buffer[16];

	if (imageType != 2 && imageType != 10) {
		return qfalse;
	}
	if (pixelDepth != 24 && pixelDepth != 32) {
		return qfalse;
	}

	*width = buffer[12] | (buffer[13] << 8);
	*height = buffer[14] | (buffer[15] << 8);
	return qtrue;
}

static qboolean CL_BGAsset_ValidateImageFile(const char *path, int maxSize, char *error, size_t errorSize) {
	fileHandle_t file;
	int length;
	byte *buffer = NULL;
	int width = 0;
	int height = 0;
	qboolean parsed = qfalse;
	const char *ext;

	if (!path || !path[0]) {
		Q_strncpyz(error, "Download failed", errorSize);
		return qfalse;
	}

	length = FS_SV_FOpenFileRead(path, &file);
	if (length <= 0) {
		Q_strncpyz(error, "Download failed", errorSize);
		return qfalse;
	}
	if (length > maxSize) {
		FS_FCloseFile(file);
		Q_strncpyz(error, "Background asset exceeds size limit", errorSize);
		return qfalse;
	}

	buffer = Z_Malloc(length);
	FS_Read(buffer, length, file);
	FS_FCloseFile(file);

	if (CL_BGAsset_ParsePNG(buffer, length, &width, &height)) {
		parsed = qtrue;
	} else if (CL_BGAsset_ParseJPG(buffer, length, &width, &height)) {
		parsed = qtrue;
	} else {
		ext = COM_GetExtension(path);
		if (ext && !Q_stricmp(ext, "tga")) {
			parsed = CL_BGAsset_ParseTGA(buffer, length, &width, &height);
		}
	}

	Z_Free(buffer);

	if (!parsed) {
		Q_strncpyz(error, "Invalid image format", errorSize);
		return qfalse;
	}

	if (width <= 0 || height <= 0) {
		Q_strncpyz(error, "Invalid image dimensions", errorSize);
		return qfalse;
	}

	if (width > CL_BGASSET_MAX_DIMENSION || height > CL_BGASSET_MAX_DIMENSION) {
		Com_sprintf(error, errorSize, "Image dimensions exceed %dx%d", CL_BGASSET_MAX_DIMENSION,
			CL_BGASSET_MAX_DIMENSION);
		return qfalse;
	}

	return qtrue;
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
	char errorMessage[128];
	char qpath[MAX_QPATH];

	qcurl_easy_getinfo(cl_bgasset.easyHandle, CURLINFO_RESPONSE_CODE, &responseCode);

	if (cl_bgasset.file) {
		FS_FCloseFile(cl_bgasset.file);
		cl_bgasset.file = 0;
	}

	if (cl_bgasset.sizeExceeded) {
		CL_BGAsset_Fail("failed", "Download failed: background asset exceeds size limit");
		return;
	}

	if (result != CURLE_OK) {
		const char *err = cl_bgasset.errorText[0] ? cl_bgasset.errorText : qcurl_easy_strerror(result);
		CL_BGAsset_Fail("failed", va("Download failed: %s", err));
		return;
	}

	if (responseCode == 304) {
		if (!CL_BGAsset_ValidateImageFile(cl_bgasset.localPath, cl_bgasset.maxSize, errorMessage,
			sizeof(errorMessage))) {
			FS_HomeRemove(cl_bgasset.localPath);
			CL_BGAsset_Fail("failed", errorMessage);
			return;
		}
		CL_BGAsset_UpdateCacheEntry(cl_bgasset.lastURL, cl_bgasset.localPath, etag, lastModified,
			CL_BGAsset_ComputeChecksum(cl_bgasset.localPath, cl_bgasset.maxSize));
		CL_BGAsset_SetState("cached");
		CL_BGAsset_SetError("");
		if (CL_BGAsset_ToQPath(cl_bgasset.localPath, qpath, sizeof(qpath))) {
			CL_BGAsset_SetPath(qpath);
		} else {
			CL_BGAsset_SetPath("");
		}
	{
		char pk3SvPath[MAX_OSPATH], pk3OsPath[MAX_OSPATH];
		CL_BGAsset_GetPk3Path( cl_bgasset.localPath, pk3SvPath, sizeof(pk3SvPath),
		                        pk3OsPath, sizeof(pk3OsPath) );
		if ( qpath[0] && CL_BGAsset_WriteStoredPk3( cl_bgasset.localPath, pk3SvPath, qpath ) ) {
			FS_AddPakToSearchpaths( pk3OsPath, pk3SvPath );
			Com_DPrintf( "Menu background: loaded pk3 '%s'\n", pk3OsPath );
		}
	}
		CL_BGAsset_CleanupHandles();
		CL_BGAsset_ScheduleRefresh();
		return;
	}

	if (responseCode < 200 || responseCode >= 300) {
		CL_BGAsset_Fail("failed", va("Download failed: HTTP error %ld", responseCode));
		return;
	}

	if (!CL_BGAsset_ValidateImageFile(cl_bgasset.tempPath, cl_bgasset.maxSize, errorMessage,
		sizeof(errorMessage))) {
		FS_HomeRemove(cl_bgasset.tempPath);
		CL_BGAsset_Fail("failed", errorMessage);
		return;
	}

	FS_SV_Rename(cl_bgasset.tempPath, cl_bgasset.localPath, qfalse);
	checksum = CL_BGAsset_ComputeChecksum(cl_bgasset.localPath, cl_bgasset.maxSize);
	CL_BGAsset_UpdateCacheEntry(cl_bgasset.lastURL, cl_bgasset.localPath, etag, lastModified, checksum);
	CL_BGAsset_SetState("ready");
	CL_BGAsset_SetError("");
	if (CL_BGAsset_ToQPath(cl_bgasset.localPath, qpath, sizeof(qpath))) {
		CL_BGAsset_SetPath(qpath);
	} else {
		CL_BGAsset_SetPath("");
	}
	{
		char pk3SvPath[MAX_OSPATH], pk3OsPath[MAX_OSPATH];
		CL_BGAsset_GetPk3Path( cl_bgasset.localPath, pk3SvPath, sizeof(pk3SvPath),
		                        pk3OsPath, sizeof(pk3OsPath) );
		if ( qpath[0] && CL_BGAsset_WriteStoredPk3( cl_bgasset.localPath, pk3SvPath, qpath ) ) {
			FS_AddPakToSearchpaths( pk3OsPath, pk3SvPath );
			Com_DPrintf( "Menu background: loaded pk3 '%s'\n", pk3OsPath );
		}
	}
	CL_BGAsset_CleanupHandles();
	CL_BGAsset_ScheduleRefresh();
}
#endif

void CL_BGAsset_Register(void) {
	ui_menuBackUrl = Cvar_Get("ui_menuBackUrl", "https://ladder.q3rally.com/background/bg.png", CVAR_ARCHIVE);
	ui_menuBackEnable = Cvar_Get("ui_menuBackEnable", "1", CVAR_ARCHIVE);
	ui_menuBackRefreshSec = Cvar_Get("ui_menuBackRefreshSec", "0", CVAR_ARCHIVE);
	ui_menuBackPath = Cvar_Get("ui_menuBackPath", "", CVAR_TEMP);
	ui_menuBackState = Cvar_Get("ui_menuBackState", "idle", CVAR_TEMP);
	ui_menuBackError = Cvar_Get("ui_menuBackError", "", CVAR_TEMP);

	cl_uiBackgroundURL = Cvar_Get("cl_uiBackgroundURL", "https://ladder.q3rally.com/background/bg.png", CVAR_ARCHIVE);
	cl_uiBackgroundEnable = Cvar_Get("cl_uiBackgroundEnable", "1", CVAR_ARCHIVE);
	cl_uiBackgroundMaxSize = Cvar_Get("cl_uiBackgroundMaxSize", va("%d", CL_BGASSET_DEFAULT_MAX_SIZE), CVAR_ARCHIVE);
	cl_uiBackgroundState = Cvar_Get("cl_uiBackgroundState", "idle", CVAR_TEMP);
	cl_uiBackgroundError = Cvar_Get("cl_uiBackgroundError", "", CVAR_TEMP);

	cl_bgasset.registered = qtrue;
	CL_BGAsset_ResetState();
}

void CL_BGAsset_Frame(void) {
#ifndef USE_CURL
	if (CL_BGAsset_IsEnabled()) {
		CL_BGAsset_SetState("unavailable");
		CL_BGAsset_SetError("cURL support not compiled");
		CL_BGAsset_SetPath("");
	}
	return;
#else
	int runningHandles = 0;
	CURLMcode multiResult;
	const char *url;
	int refreshSec;

	if (!cl_bgasset.registered) {
		CL_BGAsset_Register();
	}

	if (!CL_BGAsset_IsEnabled()) {
		CL_BGAsset_SetState("disabled");
		CL_BGAsset_SetError("");
		CL_BGAsset_SetPath("");
		cl_bgasset.attempted = qfalse;
		cl_bgasset.running = qfalse;
		CL_BGAsset_CleanupHandles();
		return;
	}

	url = CL_BGAsset_GetURL();
	if (!url[0]) {
		CL_BGAsset_SetState("idle");
		CL_BGAsset_SetError("");
		CL_BGAsset_SetPath("");
		return;
	}

	if (!CL_BGAsset_IsUrlAllowed(url)) {
		CL_BGAsset_SetState("failed");
		CL_BGAsset_SetError("Invalid URL scheme");
		CL_BGAsset_SetPath("");
		CL_BGAsset_ScheduleRefresh();
		return;
	}

	refreshSec = CL_BGAsset_GetRefreshSec();
	if (refreshSec <= 0 && cl_bgasset.nextRefreshTime) {
		cl_bgasset.nextRefreshTime = 0;
	}

	if (Q_stricmp(cl_bgasset.lastURL, url)) {
		Q_strncpyz(cl_bgasset.lastURL, url, sizeof(cl_bgasset.lastURL));
		cl_bgasset.attempted = qfalse;
		cl_bgasset.nextRefreshTime = 0;
	}

	if (cl_bgasset.nextRefreshTime && cls.realtime >= cl_bgasset.nextRefreshTime && !cl_bgasset.running) {
		cl_bgasset.attempted = qfalse;
		cl_bgasset.nextRefreshTime = 0;
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

void CL_BGAsset_ForceRefresh(void) {
	if (!cl_bgasset.registered) {
		CL_BGAsset_Register();
	}
#ifdef USE_CURL
	if (cl_bgasset.running || cl_bgasset.multiHandle || cl_bgasset.easyHandle) {
		if (cl_bgasset.file) {
			FS_FCloseFile(cl_bgasset.file);
			cl_bgasset.file = 0;
		}
		CL_BGAsset_CleanupHandles();
	}
#endif
	cl_bgasset.running = qfalse;
	cl_bgasset.attempted = qfalse;
	cl_bgasset.jobType = CL_BGASSET_JOB_NONE;
	cl_bgasset.bytesWritten = 0;
	cl_bgasset.sizeExceeded = qfalse;
	cl_bgasset.nextRefreshTime = 0;
	CL_BGAsset_SetState("idle");
	CL_BGAsset_SetError("");
}

void CL_BGAsset_ForceRefresh_f(void) {
	CL_BGAsset_ForceRefresh();
}
