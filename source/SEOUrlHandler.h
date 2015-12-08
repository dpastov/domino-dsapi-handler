// SEOUrlHandler.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once

#define DLLEXPORT  __declspec( dllexport ) // Macro to make our code more readable

// Windows Header Files:
#include <windows.h>

// TODO: reference additional headers your program requires here
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <global.h>
#include <addin.h>
#include <misc.h>
#include <dsapi.h>
#include <nsfdb.h>
#include <nsfnote.h>
#include <nsfsearc.h>
#include <osmem.h>
#include <nsferr.h>
#include <osmisc.h>

typedef struct {
	DWORD		responseCode;
	//DWORD			status;
	//int				k;
} LOGDATA;

#ifndef DEFAULT_BUFFER_SIZE
#define DEFAULT_BUFFER_SIZE 1024
#endif

#ifndef URL_BUFFER_SIZE
#define URL_BUFFER_SIZE 4096
#endif

#ifndef ERRRO_PAGE_SIZE
#define ERRRO_PAGE_SIZE 32768
#endif

#ifndef MAX_EXCEPTIONS
#define MAX_EXCEPTIONS 512
#endif

#ifndef MAX_REWRITE
#define MAX_REWRITE 512
#endif

#ifndef MAX_ERROR_PAGES
#define MAX_ERROR_PAGES 512
#endif

#ifndef MATCH_ENDS
#define MATCH_ENDS 1
#endif

#ifndef MATCH_DOES_NOT_END
#define MATCH_DOES_NOT_END 2
#endif

#ifndef MATCH_NO_ACTION
#define MATCH_NO_ACTION 3
#endif

#ifndef REDIRECT_NONE
#define REDIRECT_NONE 0
#endif

#ifndef REDIRECT_HTTPS_TO_HTTP
#define REDIRECT_HTTPS_TO_HTTP 1
#endif

#ifndef REDIRECT_HTTP_TO_HTTPS
#define REDIRECT_HTTP_TO_HTTPS 2
#endif

#ifndef REWRITE_RULES_NONE
#define REWRITE_RULES_NONE 0
#endif

#ifndef REWRITE_RULES_ACTIVE
#define REWRITE_RULES_ACTIVE 1
#endif

#ifndef DEFAULT_ERROR
#define DEFAULT_ERROR 0
#endif

#ifndef CUSTOM_ERROR
#define CUSTOM_ERROR 1
#endif

// Constants for log level
#ifndef CRITICAL_MSG
#define CRITICAL_MSG 256
#define IMPORTANT_MSG 256
#define ERROR_MSG 16
#define NORMAL_MSG 8
#define VERBOSE_MSG 4
#define DEBUG_MSG 2
#endif