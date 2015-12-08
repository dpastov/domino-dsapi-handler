/* SEOUrlHandler.c
 * Developed by Tron Systems Ltd.
 * Released under GPL-3.0 2011-09-12
 */

//
#include "SEOUrlHandler.h"

char*	db_filename = "dfc.nsf";
char*	db_filenamedev = "dfc-dev.nsf";
char	filter_name[kMaxFilterDesc]={0};
char*	endToFind = "/";
char*	endReplacement = "";
int		matchType = MATCH_ENDS;
int		redirectType = REDIRECT_NONE;
int		custom_error_pages = DEFAULT_ERROR;

char	exceptions[MAX_EXCEPTIONS][URL_BUFFER_SIZE];
int		exceptionsCount = 0;
char	redirectExceptions[MAX_EXCEPTIONS][URL_BUFFER_SIZE];
int		redirectExceptionsCount = 0;
char	redirectAdditionalsFrom[MAX_EXCEPTIONS][URL_BUFFER_SIZE];
char	redirectAdditionalsTo[MAX_EXCEPTIONS][URL_BUFFER_SIZE];
int		redirectAdditionalsCount = 0;
char	errorKey[MAX_ERROR_PAGES][URL_BUFFER_SIZE];
char	errorHTML[MAX_ERROR_PAGES][ERRRO_PAGE_SIZE];
int		errorPagesCount = 0;
int		rewriteRules = REWRITE_RULES_NONE;
char	rewriteFrom[MAX_REWRITE][URL_BUFFER_SIZE];
char	rewriteTo[MAX_REWRITE][URL_BUFFER_SIZE];
int		rewriteCount = 0;

int		iReportSeverity=DEBUG_MSG;		// set default logging to debug - the
										// config setting will override if found

int		bLog=TRUE;						// for debugging it can help to issue printf
										// statements instead of logging to the Domino
										// log as time delays can occurr making it 
										// hard to follow the order of events. To use printf
										// set bLog=FALSE

/* Notes SDK shared library entrypoint */
DLLEXPORT int MainEntryPoint(void);

int		RewriteURL(FilterContext* context, FilterMapURL* pEventData);
int		ResponseHeaders(FilterContext* context, FilterResponseHeaders* eventData);
int		OutputErrorPage(FilterContext* context, FilterResponseHeaders* eventData);
int		SendRedirect(FilterContext* context, const char* pszUrl);
int		QueryRewrite(FilterContext* context, FilterMapURL* pEventData);

int		writeToLog(int severity, const char *message);
int		ConstructUrls(FilterContext* context, char* fullurl, char* shorturl, char* query, const char* url);
int		StringEnds(const char* needle, const char* haystack);
int		BSearch(const void *buf, int size, const char* url);
int		LSearch(const char *buf[], int size, const char* url);

int		FormatUrl(FilterContext* context, char* newUrl, const char* url, const char* query);
int		FormatRedirectionUrl(FilterContext* context, char* newUrl, const char* url, const int securePort);

/* Prototypes for config handling functions */
int		ReadGeneralConfig();
int		PreloadExceptions();
int		PreloadRedirectExceptions();
int		PreloadRedirectAdditionals();
int		loadRewrites();
int		loadErrorPages();

STATUS LNPUBLIC store_config(void far *db_handle, SEARCH_MATCH far *pSearchMatch, ITEM_TABLE far *summary_info);
STATUS LNPUBLIC store_exception(void far *db_handle, SEARCH_MATCH far *pSearchMatch, ITEM_TABLE far *summary_info);
STATUS LNPUBLIC store_redirect_exception(void far *db_handle, SEARCH_MATCH far *pSearchMatch, ITEM_TABLE far *summary_info);
STATUS LNPUBLIC store_redirect_additional(void far *db_handle, SEARCH_MATCH far *pSearchMatch, ITEM_TABLE far *summary_info);
STATUS LNPUBLIC store_error_pages(void far *db_handle, SEARCH_MATCH far *pSearchMatch, ITEM_TABLE far *summary_info);
STATUS LNPUBLIC store_rewrites(void far *db_handle, SEARCH_MATCH far *pSearchMatch, ITEM_TABLE far *summary_info);

void	PrintAPIError (STATUS api_error);

DLLEXPORT int MainEntryPoint(void)
{
	return NOERROR;
}

DLLEXPORT unsigned int FilterInit(FilterInitData* filterInitData)
{
	// Buffer for building messages for the log
	char	szBuffer[DEFAULT_BUFFER_SIZE]={0};
	int		bConfigOk = FALSE;

	filterInitData->appFilterVersion = kInterfaceVersion;

	// Use a default name for the filter for log messages that may
	// come out due to the config db not being found...
	strncpy(filter_name, "DSAPI filter", kMaxFilterDesc);

	// Read configuration settings from Database
	if (0 == ReadGeneralConfig()) {
		// Only register for the events that we want to filter if config ok
		filterInitData->eventFlags = kFilterRewriteURL | kFilterResponse | kFilterTranslateRequest;
		bConfigOk = TRUE;
	}

	strcpy(filterInitData->filterDesc, "filter");

	// Output some information to the console and log
	writeToLog(CRITICAL_MSG, "****************************************");
	writeToLog(CRITICAL_MSG, "         DSAPI filter ver. 1.6          ");
	writeToLog(CRITICAL_MSG, "****************************************");
	writeToLog(CRITICAL_MSG, "Loading...");
	writeToLog(CRITICAL_MSG, "Released under GPL-3.0");
	writeToLog(CRITICAL_MSG, "Designed for use on Domino 8.5.x servers.");
	sprintf(szBuffer, "Version from %s %s.", __TIME__, __DATE__);
	writeToLog(CRITICAL_MSG, szBuffer);
	sprintf(szBuffer, "URL Handler status: %d.", matchType);
	writeToLog(CRITICAL_MSG, szBuffer);
	sprintf(szBuffer, "URL Parameter status: %d.", rewriteRules);
	writeToLog(CRITICAL_MSG, szBuffer);
	sprintf(szBuffer, "Redirect option status: %d.", redirectType);
	writeToLog(CRITICAL_MSG, szBuffer);

	if (bConfigOk == FALSE) {
		writeToLog(CRITICAL_MSG, "Error: Unable to find dfc.nsf or dfc-dev.nsf configuration databases.");
		writeToLog(CRITICAL_MSG, "The filter will not process any requests.");
		//Return an error so that the filter will not be called for any events
		return kFilterError;
	}

	// *** Read exceptions from database ***
	if (0 != PreloadExceptions()) {
		writeToLog(CRITICAL_MSG, "Unable to process Exceptions settings.");
	}
	if (exceptionsCount>0) {
		// make sure the strings are sorted - we will later use bsearch
		qsort(exceptions, exceptionsCount, URL_BUFFER_SIZE, (int(*)(const void*, const void*))strcmp);
	}
	sprintf(szBuffer, "Loaded %d exception(s).", exceptionsCount);
	writeToLog(CRITICAL_MSG, szBuffer);


	// *** Read redirection exceptions from database ***
	if (0 != PreloadRedirectExceptions()) {
		writeToLog(CRITICAL_MSG, "Unable to process Redirect Exceptions settings.");
	}
	if (redirectExceptionsCount>0) {
		// make sure the strings are sorted - we will later use bsearch
		qsort(redirectExceptions, redirectExceptionsCount, URL_BUFFER_SIZE, (int(*)(const void*, const void*))strcmp);
	}
	sprintf(szBuffer, "Loaded %d redirect exception(s).", redirectExceptionsCount);
	writeToLog(CRITICAL_MSG, szBuffer);


	// *** Read redirection additionals from database ***
	if (0 != PreloadRedirectAdditionals()) {
		writeToLog(CRITICAL_MSG, "Unable to process redirect additionals settings.");
	}
	sprintf(szBuffer, "Loaded %d redirect additional(s).", redirectAdditionalsCount);
	writeToLog(CRITICAL_MSG, szBuffer);


	// *** Read error pages from database ***
	if (0 != loadErrorPages()) {
		writeToLog(CRITICAL_MSG, "Unable to process Error pages.");
	}
	sprintf(szBuffer, "Loaded %d error page(s).", errorPagesCount);
	writeToLog(CRITICAL_MSG, szBuffer);


	// *** Read rewrite rules from database ***
	if (0 != loadRewrites()) {
		writeToLog(CRITICAL_MSG, "Unable to process Rewrite rules.");
	}
	sprintf(szBuffer, "Loaded %d rewrite rule(s).", rewriteCount);
	writeToLog(CRITICAL_MSG, szBuffer);


	return kFilterHandledRequest;
}	// end FilterInit

DLLEXPORT unsigned int TerminateFilter(unsigned int reserved)
{
	writeToLog(CRITICAL_MSG, "Filter unloaded.");
	return kFilterHandledEvent;
}	// end TerminateFilter

DLLEXPORT unsigned int HttpFilterProc(FilterContext* context, unsigned int eventType, void* eventData) {
	switch (eventType) {
		case kFilterTranslateRequest:
			writeToLog(DEBUG_MSG, "kFilterTranslateRequest Event");
			return QueryRewrite(context, (FilterMapURL *) eventData);
		case kFilterRewriteURL:
			writeToLog(DEBUG_MSG, "kFilterRewriteURL Event");
			return RewriteURL(context, (FilterMapURL *) eventData);
		case kFilterResponse:
			writeToLog(DEBUG_MSG, "kFilterResponse Event");
			return ResponseHeaders(context, (FilterResponseHeaders *) eventData);
		default:
			return kFilterNotHandled;
	}
}	// end HttpFilterProc

int ResponseHeaders(FilterContext* context, FilterResponseHeaders* eventData) {
	char			szBuffer[ERRRO_PAGE_SIZE]={0};
	int				responseCode;

	if (custom_error_pages==DEFAULT_ERROR) {
		return kFilterNotHandled;
	}

	responseCode = eventData->responseCode;
	if (responseCode==404 || responseCode==400 || responseCode==500) {
		if (OutputErrorPage(context, eventData)!=TRUE) {
			return kFilterNotHandled;
		};
	}

	return kFilterHandledEvent;
}

int OutputErrorPage(FilterContext* context, FilterResponseHeaders* response) {
	unsigned int			errID = 0;
	char					szBuffer[DEFAULT_BUFFER_SIZE]={0};
	char					szKey[DEFAULT_BUFFER_SIZE]={0};
	FilterParsedRequestLine pRequestLine;
	int						i;
	int						k=-1;

	// get domain name (its our key), could be done faster? right now its simple walk via 10-20 documents which is fine for now
	context->ServerSupport(context, kGetParsedRequest, &pRequestLine, NULL, NULL, &errID);
	for(i=0; i<errorPagesCount;i++) {
		sprintf(szKey, "%s%d", pRequestLine.pHostName, response->responseCode);
		if (strcmp(errorKey[i], szKey)==0) {
			k=i;
			i=errorPagesCount;
		}
	}

	if (k==-1) {
		sprintf(szBuffer, "There is no error page for %s%d", pRequestLine.pHostName, response->responseCode);
		writeToLog(CRITICAL_MSG, szBuffer);
		return FALSE;
	}

	sprintf(szBuffer, "Content-Type: text/html; charset=UTF-8\r\nContent-length: %d\r\n\r\n", strlen(errorHTML[k]));
	response->headerText = szBuffer;

	if (context->ServerSupport(context, kWriteResponseHeaders, response, 0, 0, &errID) != TRUE) {
		sprintf(szBuffer, "Error sending redirect, code: %d", errID);
		writeToLog(CRITICAL_MSG, szBuffer);
		return FALSE;
	}

    if (context->WriteClient(context, errorHTML[k], (unsigned int) strlen(errorHTML[k]), 0, &errID) != TRUE) {
		sprintf(szBuffer, "Error sending redirect, code: %d", errID);
		writeToLog(CRITICAL_MSG, szBuffer);
		return FALSE;
	}

	return TRUE;
}

int RewriteURL(FilterContext* context, FilterMapURL* pEventData) {
	char szFullUrl[URL_BUFFER_SIZE] = {0};
	char szShortUrl[URL_BUFFER_SIZE] = {0};
	char szQuery[URL_BUFFER_SIZE] = {0};
	char szNewUrl[URL_BUFFER_SIZE] = {0};
	char szBuffer[DEFAULT_BUFFER_SIZE]={0};
	int endFound = 0;
	int redirect = 0;
	int redirectToPort = 0;
	int n=-1;
	int i;

	//Split the requested url into path and querystring
	if (ConstructUrls(context, szFullUrl, szShortUrl, szQuery, pEventData->url) == FALSE) {
		return kFilterNotHandled;
	}

	//STEP1. URL Handler (i.e. remove last '/')
	endFound = StringEnds(endToFind, szShortUrl);

	//Check what match type we are using (set in config)
	if (((matchType == MATCH_ENDS) && (endFound == TRUE)) || ((matchType == MATCH_DOES_NOT_END) && (endFound == FALSE))) {
		writeToLog(DEBUG_MSG, "End of url matches criteria");

		if (BSearch(exceptions, exceptionsCount, szShortUrl)==FALSE) {
			if (FormatUrl(context, szNewUrl, szShortUrl, szQuery) != TRUE) {
				writeToLog(ERROR_MSG, "Error: Unable to build url for redirection (1), request not handled");
				return kFilterNotHandled;
			}
			if (SendRedirect(context, szNewUrl) == TRUE) {	//Send "301" requests to new url
				return kFilterHandledRequest;
			} else {
				return kFilterNotHandled;
			}
		}
	}

	// Redirect (with protocol)
	for(i=0; i<redirectAdditionalsCount && n==-1; i++) {
		if (strcmp(redirectAdditionalsFrom[i], szFullUrl)==0) {
			writeToLog(VERBOSE_MSG, "Url FOUND in additional exceptions list.");
			n = i;
			i = redirectAdditionalsCount;
		}
	}	
	if (n >= 0) {
		sprintf(szBuffer, "We found %d %s.", n, redirectAdditionalsTo[n]);
		writeToLog(CRITICAL_MSG, szBuffer);

		strcpy(szNewUrl, redirectAdditionalsTo[n]);

		if (SendRedirect(context, szNewUrl) == TRUE) {	//Send "bad" requests to a 301 redirect
			return kFilterHandledRequest;
		} else {
			return kFilterNotHandled;
		}
	}

	// exit in case if redirect flag set to None
	// we agreed we will no use it this year at least (2012)
	if (redirectType != REDIRECT_NONE) {
		// Do we need http=>https or https=>http?
		if (redirectType == REDIRECT_HTTPS_TO_HTTP && context->securePort == 1) {
			redirect = 1;
			redirectToPort = 0;
		}
		else if(redirectType == REDIRECT_HTTP_TO_HTTPS && context->securePort == 0) {
			redirect = 1;
			redirectToPort = 1;
		}

		// is it exception?
		if (BSearch(redirectExceptions, redirectExceptionsCount, szFullUrl)==TRUE) {
			writeToLog(CRITICAL_MSG, "Skip this URL, it is exception");
			redirect = 0;
		}

		// We need to redirect page to http/https
		if (redirect==1) {
			if (FormatRedirectionUrl(context, szNewUrl, pEventData->url, redirectToPort) != TRUE) {
				writeToLog(ERROR_MSG, "Error: Unable to build url for redirection (2), request not handled");
				return kFilterNotHandled;
			}
			writeToLog(ERROR_MSG, szNewUrl);
			if (SendRedirect(context, szNewUrl) == TRUE) {	//Send "bad" requests to a 301 redirect
				return kFilterHandledRequest;
			} else {
				return kFilterNotHandled;
			}
		}
	}

	return kFilterNotHandled;
}

/*********************************************************************
Function:		QueryRewrite
Description:	It adds ?open to url with query that start without ?open.
				i.e. http://path?param1=123 will be transformed to http://path?open&param1=123
*********************************************************************/
int	QueryRewrite(FilterContext* context, FilterMapURL* pEventData) {
	FilterParsedRequestLine pReqData;
	unsigned int errid=0;
	char szNewURL[URL_BUFFER_SIZE] = {0};
	char szQuery[URL_BUFFER_SIZE] = {0};
	int i;

	if (rewriteCount==0 || rewriteRules!=REWRITE_RULES_ACTIVE) return FALSE;

	context->ServerSupport(context, kGetParsedRequest, &pReqData, NULL, NULL, &errid);

	if (pReqData.pQueryUri == NULL) return kFilterNotHandled;

	sprintf(szQuery, "%s", pReqData.pQueryUri);
	for(i=0; i<rewriteCount; i++) {
		if (strstr(szQuery, rewriteFrom[i])!=NULL && strncmp(szQuery, rewriteTo[i], strlen(rewriteTo[i]))!=0) {
			writeToLog(DEBUG_MSG, "URL query has been rewritten");
			sprintf(pEventData->pathBuffer, "%s?%s&%s", pReqData.pPathUri, rewriteTo[i], pReqData.pQueryUri);
			writeToLog(DEBUG_MSG, pEventData->pathBuffer);
			pEventData->mapType = kURLMapRedirect;

			return kFilterHandledEvent;
		}
	}

	return kFilterNotHandled;
}

/*********************************************************************
Function:		FormatRedirectionUrl
Description:	Construct a new url from the supplied path (http or https replacement).
				New URL may come from exception, so it will be absolutely different from original.
				The returned value stored in newUrl is absolute.
*********************************************************************/
int	FormatRedirectionUrl(FilterContext* context, char* newUrl, const char* url, const int securePort)
{
	unsigned int			errID = 0;
	FilterParsedRequestLine requestLine;
	if (context->ServerSupport(context,kGetParsedRequest,&requestLine,0,0,&errID) != TRUE)
	{
		// Unable to parse the request - just return false
		return FALSE;
	}
	// Set the request protocol
	if (securePort == 1) {
		strcat(newUrl, "https://");
	} else {
		strcat(newUrl, "http://");
	}

	// Append the hostname
	strcat(newUrl, requestLine.pHostName);
	strcat(newUrl, url);

	return TRUE;
}


/*********************************************************************
Function:		FormatUrl
Description:	Construct a new url from the supplied path and query.
				The returned value stored in newUrl is absolute.
*********************************************************************/
int	FormatUrl(FilterContext* context, char* newUrl, const char* url, const char* query)
{
	unsigned int			errID = 0;
	FilterParsedRequestLine requestLine;
	if (context->ServerSupport(context,kGetParsedRequest,&requestLine,0,0,&errID) != TRUE)
	{
		// Unable to parse the request - just return false
		return FALSE;
	}
	// Set the request protocol
	if (context->securePort == 1) {
		strcat(newUrl, "https://");
	} else {
		strcat(newUrl, "http://");
	}
	// Append the hostname
	strcat(newUrl, requestLine.pHostName);

	if (matchType == MATCH_ENDS)
	{
		//Check we have room to build the new value
		if ((strlen(newUrl) + strlen(url) 
			- strlen(endToFind) + strlen(query) + 1) > URL_BUFFER_SIZE) {
			return FALSE;
		}
		strncat(newUrl, url, strlen( url ) - strlen(endToFind));
		strcat(newUrl, endReplacement);
	} else {
		//Check we have room to build the new value
		if ((strlen(newUrl) + strlen(url) 
			+ strlen(endToFind) + strlen(query) + 1) > URL_BUFFER_SIZE) {
			return FALSE;
		}
		strncat(newUrl, url, strlen( url ));
		strcat(newUrl, endToFind);
	}
	if (strlen(query)>0)
	{
		strcat(newUrl, "?");
		strcat(newUrl, query);
	}
	return TRUE;
}


/*********************************************************************
Function:		SendRedirect
Description:	Return a 301 redirection to the new url location
*********************************************************************/
int SendRedirect(FilterContext* context, const char* pszNewUrl)
{
	FilterResponseHeaders	response;
	unsigned int			errID = 0;
	unsigned int			reserved = 0;
	char					szHeaderSet[URL_BUFFER_SIZE] = {0};
	// Buffer for building messages for the log
	char					szBuffer[DEFAULT_BUFFER_SIZE]={0};

	if (strlen(pszNewUrl) > (URL_BUFFER_SIZE - strlen("Location:") - strlen("\r\n\r\n") - 1))
	{
		writeToLog(ERROR_MSG, "Incoming URL length is larger than the maximum buffer size for redirection - ignoring request.");
		return FALSE;
	}

	response.responseCode=301;
	response.reasonText="Moved Permanently";
	
	strcpy(szHeaderSet, "Location:");
	strcat(szHeaderSet, pszNewUrl);
	strcat(szHeaderSet, "\r\n\r\n");

	response.headerText=szHeaderSet;

	if (context->ServerSupport(context,kWriteResponseHeaders,&response,0,0,&errID) != TRUE)
	{
		sprintf(szBuffer, "Error sending redirect, code: %d", errID);
		writeToLog(CRITICAL_MSG, szBuffer);
		return FALSE;
	}
	return TRUE;
}

/*********************************************************************
Function:		BSearch
Description:	Return true if the specified string is an element of list
*********************************************************************/
int BSearch(const void *buf, int size, const char* key)
{
	char *pos = NULL;

	if (size == 0) {
		return FALSE;
	}
	// search for url
	pos = (char*) bsearch(key, buf, size, URL_BUFFER_SIZE, (int(*)(const void*, const void*))strcmp);
 
	if (pos) {
		writeToLog(VERBOSE_MSG, "Url FOUND in allowed exceptions list.");
		return TRUE;
	} else {
		writeToLog(DEBUG_MSG, "Url not found in allowed exceptions list.");
		return FALSE;
	}
}

/*********************************************************************
Function:		StringEnds
Description:	Return true only if haystack ends with needle
*********************************************************************/
int StringEnds(const char* needle, const char* haystack)
{
	size_t lenNeedle = strlen(needle);
	size_t lenHaystack = strlen(haystack);

	if (lenNeedle > lenHaystack) { 
		return FALSE;
	}

	//Compare the strings, starting at the adjusted postion
	if (strncmp(haystack + (lenHaystack - lenNeedle), needle, lenNeedle) == 0) {
		return TRUE;
	}

	return FALSE;
}

/*********************************************************************
Function:		StripQueryString
Description:	Split requested url into path and querystring
*********************************************************************/
int ConstructUrls(FilterContext* context, char* fullurl, char* shorturl, char* query, const char* url)
{
	char *ptr;
	unsigned int			errID = 0;
	
	FilterParsedRequestLine requestLine;
	if (context->ServerSupport(context,kGetParsedRequest,&requestLine,0,0,&errID) != TRUE)
	{
		// Unable to parse the request - just return false
		return FALSE;
	}

	writeToLog(DEBUG_MSG, "Incoming Url:");
	writeToLog(DEBUG_MSG, url);

	if (context->securePort == 1) {
		strcat(fullurl, "https://");
	} else {
		strcat(fullurl, "http://");
	}
	strcat(fullurl, requestLine.pHostName);
	strcat(fullurl, url);
	writeToLog(DEBUG_MSG, "[Full Url] after query strip:");
	writeToLog(DEBUG_MSG, fullurl);


	ptr = strchr(url, '?');
	if (ptr == NULL) {
		//No querystring found, so store the original url as result
		strncpy(shorturl, url, strlen(url));
		
	} else {
		//Store the path up to the query in result...
		strncpy(shorturl, url, (ptr - url));
		//Store what follows the query character in query...
		strncpy(query, ++ptr, strlen(url) - strlen(shorturl));
	}

	writeToLog(DEBUG_MSG, "[Short Url] after query strip:");
	writeToLog(DEBUG_MSG, shorturl);
	return TRUE;
}

/* 
 * Function: writeToLog
 * Description: writes output to the Domino log and console.
 * If bLog is set to FALSE, then output is only written to the console.
 * This can help with debugging, as there are no delays in the console output
 * if the log is bypassed.
 * Messages are only output if the severity of the message is higher than the 
 * threshold set in iReportSeverity.
 */
int writeToLog(int severity, const char *message) 
{
	if (severity < iReportSeverity) {
		return NOERROR;
	}

	if (filter_name && *message) {
		if (bLog==TRUE) {
			AddInLogMessageText("%s: %s", NOERROR, filter_name, message);
		} else {
			printf("%s: %s\n", filter_name, message);
		}
	}

	return NOERROR;
}

/* Configuration handling functions */

int ReadGeneralConfig()
{
    DBHANDLE    db_handle; 
	char        formula[] = "Form=\"Configuration\""; 
    FORMULAHANDLE    formula_handle;
    WORD     wdc;
    STATUS   error = NOERROR;
   
	//Open the database (prod version and if it does not exists - dev version)
    if (error = NSFDbOpen (db_filename, &db_handle)) {
		if(error = NSFDbOpen (db_filenamedev, &db_handle)) {
			PrintAPIError (error);
			return (1);
		}
	}
    
    //Create a selection formula to read just the configuration document
    if (error = NSFFormulaCompile (NULL, (WORD) 0, formula, 
									(WORD) strlen(formula), &formula_handle,
									&wdc, &wdc, &wdc, &wdc, &wdc, &wdc)) {
        NSFDbClose (db_handle);
        PrintAPIError (error);  
        return (1);
    }

	//Perform the search...
    if (error = NSFSearch (db_handle, formula_handle, NULL, 0,
                NOTE_CLASS_DOCUMENT, NULL, store_config, &db_handle,
                NULL)) {
        NSFDbClose (db_handle);
        PrintAPIError (error);  
        return (1);
    }

	//Free the memory
    OSMemFree (formula_handle);

	// Close the db
    if (error = NSFDbClose (db_handle)) {
        PrintAPIError (error);  
        return (1);
    } 
	return (0);
}

STATUS LNPUBLIC store_config(void far *db_handle, SEARCH_MATCH far *pSearchMatch, ITEM_TABLE far *summary_info)
{
    SEARCH_MATCH SearchMatch;
    NOTEHANDLE   note_handle;
    WORD         field_len;
    char         field_text[500];
    STATUS       error;
	int			iLogLevel = 0;

    memcpy( (char*)&SearchMatch, (char*)pSearchMatch, sizeof(SEARCH_MATCH) );

	/* Open the note. */
    if (error = NSFNoteOpen (
                *(DBHANDLE far *)db_handle,		/* database handle */
                SearchMatch.ID.NoteID,			/* note ID */
                0,								/* open flags */
                &note_handle)					/* note handle (return) */
				) {
		return (ERR(error));
	}

	field_len = NSFItemGetText(note_handle, "FilterName", field_text, (WORD) sizeof (field_text));
	if (field_len > 0) {
		strncpy(filter_name, field_text, kMaxFilterDesc);
	} else {
		strncpy(filter_name, "DSAPI filter", kMaxFilterDesc);
	}
    
	field_len = NSFItemGetText(note_handle, "LogLevel",	field_text, (WORD) sizeof (field_text));
	if (field_len>0) {
		iLogLevel = strtol(field_text, NULL, 0);
		iReportSeverity = iLogLevel;
	}
	
	// URL Handler
	field_len = NSFItemGetText(note_handle, "Operation", field_text, (WORD) sizeof (field_text));
	if (field_len > 0) {
		if (strcmp(field_text, "1") == 0) {
			matchType = MATCH_ENDS;
		} else if (strcmp(field_text, "2") == 0) {
			matchType = MATCH_DOES_NOT_END;
		} else if (strcmp(field_text, "3") == 0) {
			matchType = MATCH_NO_ACTION;
		}
	}

	// Redirect optinos
	field_len = NSFItemGetText(note_handle, "Redirection", field_text, (WORD) sizeof (field_text));
	if (field_len > 0) {
		if (strcmp(field_text, "0") == 0) {
			redirectType = REDIRECT_NONE;
		} else if (strcmp(field_text, "1") == 0) {
			redirectType = REDIRECT_HTTPS_TO_HTTP;
		} else if (strcmp(field_text, "2") == 0) {
			redirectType = REDIRECT_HTTP_TO_HTTPS;
		}
	}

	// URL Parameters
	field_len = NSFItemGetText(note_handle, "RewriteRules", field_text, (WORD) sizeof (field_text));
	if (field_len > 0) {
		if (strcmp(field_text, "1") == 0) {
			rewriteRules = REWRITE_RULES_ACTIVE;
		}
	}

	// Custom Error pages
	field_len = NSFItemGetText(note_handle, "ErrorPage", field_text, (WORD) sizeof (field_text));
	if (field_len > 0) {
		if (strcmp(field_text, "0") == 0) {
			custom_error_pages = DEFAULT_ERROR;
		} else if (strcmp(field_text, "1") == 0) {
			custom_error_pages = CUSTOM_ERROR;
		}
	}
		
	/* Close the note. */
	if (error = NSFNoteClose (note_handle)) {
        return (ERR(error));
	}

	/* End of subroutine. */
    return (NOERROR);
}

int PreloadExceptions()
{  
    DBHANDLE		db_handle;      
	char			formula[] = "Form=\"Exception\"";	
    FORMULAHANDLE   formula_handle;						
    WORD			wdc;
    STATUS			error = NOERROR;

	//Open the database (prod version and if it does not exists - dev version)
    if (error = NSFDbOpen (db_filename, &db_handle)) {
		if(error = NSFDbOpen (db_filenamedev, &db_handle)) {
			PrintAPIError (error);
			return (1);
		}
	}
    
    /* Compile the selection formula. */
    if (error = NSFFormulaCompile (NULL, (WORD) 0, formula,
                (WORD) strlen(formula), &formula_handle, &wdc, &wdc,
                &wdc, &wdc, &wdc, &wdc)) {
        NSFDbClose (db_handle);
        PrintAPIError (error);  
        return (1);
    }

	//Call "store_exception" for each document matching the search
    if (error = NSFSearch (db_handle, formula_handle, 
							NULL, 0, NOTE_CLASS_DOCUMENT,
							NULL, store_exception, &db_handle, 
							NULL)) {
        NSFDbClose (db_handle);
        PrintAPIError (error);  
        return (1);
    }

	/* Free the memory allocated to the compiled formula. */
    OSMemFree (formula_handle);

	/* Close the database. */
    if (error = NSFDbClose (db_handle)) {
        PrintAPIError (error);  
        return (1);
    } 
	
	return (0);
}
int PreloadRedirectExceptions()
{  
    DBHANDLE		db_handle;      
	char			formula[] = "Form=\"RedirectException\"";	
    FORMULAHANDLE   formula_handle;						
    WORD			wdc;
    STATUS			error = NOERROR;

	//Open the database (prod version and if it does not exists - dev version)
    if (error = NSFDbOpen (db_filename, &db_handle)) {
		if(error = NSFDbOpen (db_filenamedev, &db_handle)) {
			PrintAPIError (error);
			return (1);
		}
	}
    
    /* Compile the selection formula. */
    if (error = NSFFormulaCompile (NULL, (WORD) 0, formula,
                (WORD) strlen(formula), &formula_handle, &wdc, &wdc,
                &wdc, &wdc, &wdc, &wdc)) {
        NSFDbClose (db_handle);
        PrintAPIError (error);  
        return (1);
    }

	//Call "store_redirect_exception" for each document matching the search
    if (error = NSFSearch (db_handle, formula_handle, 
							NULL, 0, NOTE_CLASS_DOCUMENT,
							NULL, store_redirect_exception, &db_handle, 
							NULL)) {
        NSFDbClose (db_handle);
        PrintAPIError (error);  
        return (1);
    }

	/* Free the memory allocated to the compiled formula. */
    OSMemFree (formula_handle);

	/* Close the database. */
    if (error = NSFDbClose (db_handle)) {
        PrintAPIError (error);  
        return (1);
    } 
	
	return (0);
}
int PreloadRedirectAdditionals()
{  
    DBHANDLE		db_handle;      
	char			formula[] = "Form=\"RedirectAdditional\"";	
    FORMULAHANDLE   formula_handle;						
    WORD			wdc;
    STATUS			error = NOERROR;

	//Open the database (prod version and if it does not exists - dev version)
    if (error = NSFDbOpen (db_filename, &db_handle)) {
		if(error = NSFDbOpen (db_filenamedev, &db_handle)) {
			PrintAPIError (error);
			return (1);
		}
	}
    
    /* Compile the selection formula. */
    if (error = NSFFormulaCompile (NULL, (WORD) 0, formula,
                (WORD) strlen(formula), &formula_handle, &wdc, &wdc,
                &wdc, &wdc, &wdc, &wdc)) {
        NSFDbClose (db_handle);
        PrintAPIError (error);  
        return (1);
    }

	//Call "store_redirect_additional" for each document matching the search
    if (error = NSFSearch (db_handle, formula_handle, 
							NULL, 0, NOTE_CLASS_DOCUMENT,
							NULL, store_redirect_additional, &db_handle, 
							NULL)) {
        NSFDbClose (db_handle);
        PrintAPIError (error);  
        return (1);
    }

	/* Free the memory allocated to the compiled formula. */
    OSMemFree (formula_handle);

	/* Close the database. */
    if (error = NSFDbClose (db_handle)) {
        PrintAPIError (error);  
        return (1);
    } 
	
	return (0);
}

STATUS LNPUBLIC store_exception(void far *db_handle, SEARCH_MATCH far *pSearchMatch, ITEM_TABLE far *summary_info)
{
    SEARCH_MATCH SearchMatch;
    NOTEHANDLE   note_handle;
    WORD         field_len;
    char         field_text[URL_BUFFER_SIZE];
	char		 szBuffer[DEFAULT_BUFFER_SIZE]={0};
    STATUS       error;

	//Message to show if there are more configured exceptions than are allowed
	sprintf(szBuffer, "Unable to store exception (maximum of %d already reached).", MAX_EXCEPTIONS);

    memcpy( (char*)&SearchMatch, (char*)pSearchMatch, sizeof(SEARCH_MATCH) );

	/* Open the note. */
    if (error = NSFNoteOpen (
                *(DBHANDLE far *)db_handle,		/* database handle */
                SearchMatch.ID.NoteID,			/* note ID */
                0,								/* open flags */
                &note_handle)					/* note handle (return) */
				) {
		return (ERR(error));
	}
    
	field_len = NSFItemGetText(note_handle, "ExceptionPath", field_text, 
		(WORD) sizeof (field_text));
	
	if (field_len > 0) {
		if (exceptionsCount < MAX_EXCEPTIONS) {
			strcpy(exceptions[exceptionsCount],field_text);
			exceptionsCount++;
		} else {
			writeToLog(CRITICAL_MSG, szBuffer);
		}
	}

	field_len = NSFItemGetText(note_handle, "ExceptionAlias", field_text,
		(WORD) sizeof (field_text));
	
	if (field_len > 0) {
		if (exceptionsCount < MAX_EXCEPTIONS) {
			strcpy(exceptions[exceptionsCount],field_text);
			exceptionsCount++;
		} else {
			writeToLog(CRITICAL_MSG, szBuffer);
		}
	}

	/* Close the note. */
	if (error = NSFNoteClose (note_handle)) {
        return (ERR(error));
	}

	/* End of subroutine. */
    return (NOERROR);
}
STATUS LNPUBLIC store_redirect_exception(void far *db_handle, SEARCH_MATCH far *pSearchMatch, ITEM_TABLE far *summary_info)
{
    SEARCH_MATCH SearchMatch;
    NOTEHANDLE   note_handle;
    WORD         field_len;
    char         field_text[URL_BUFFER_SIZE];
	char		 szBuffer[DEFAULT_BUFFER_SIZE]={0};
    STATUS       error;

	//Message to show if there are more configured exceptions than are allowed
	sprintf(szBuffer, "Unable to store redirect exception (maximum of %d already reached).", MAX_EXCEPTIONS);

    memcpy( (char*)&SearchMatch, (char*)pSearchMatch, sizeof(SEARCH_MATCH) );

	/* Open the note. */
    if (error = NSFNoteOpen (
                *(DBHANDLE far *)db_handle,		/* database handle */
                SearchMatch.ID.NoteID,			/* note ID */
                0,								/* open flags */
                &note_handle)					/* note handle (return) */
				) {
		return (ERR(error));
	}
    
	field_len = NSFItemGetText(note_handle, "ExceptionPath", field_text, 
		(WORD) sizeof (field_text));
	
	if (field_len > 0) {
		if (redirectExceptionsCount < MAX_EXCEPTIONS) {
			strcpy(redirectExceptions[redirectExceptionsCount],field_text);
			redirectExceptionsCount++;
		} else {
			writeToLog(CRITICAL_MSG, szBuffer);
		}
	}

	/* Close the note. */
	if (error = NSFNoteClose (note_handle)) {
        return (ERR(error));
	}

	/* End of subroutine. */
    return (NOERROR);
}
STATUS LNPUBLIC store_redirect_additional(void far *db_handle, SEARCH_MATCH far *pSearchMatch, ITEM_TABLE far *summary_info)
{
    SEARCH_MATCH SearchMatch;
    NOTEHANDLE   note_handle;
    WORD         field_len;
    char         field_text[URL_BUFFER_SIZE];
	char		 szBuffer[DEFAULT_BUFFER_SIZE]={0};
    STATUS       error;

	//Message to show if there are more configured exceptions than are allowed
	sprintf(szBuffer, "Unable to store exception (maximum of %d already reached).", MAX_EXCEPTIONS);

    memcpy( (char*)&SearchMatch, (char*)pSearchMatch, sizeof(SEARCH_MATCH) );

	/* Open the note. */
    if (error = NSFNoteOpen (
                *(DBHANDLE far *)db_handle,		/* database handle */
                SearchMatch.ID.NoteID,			/* note ID */
                0,								/* open flags */
                &note_handle)					/* note handle (return) */
				) {
		return (ERR(error));
	}
    
	field_len = NSFItemGetText(note_handle, "From", field_text, 
		(WORD) sizeof (field_text));
	
	if (field_len > 0) {
		if (redirectAdditionalsCount < MAX_EXCEPTIONS) {
			strcpy(redirectAdditionalsFrom[redirectAdditionalsCount],field_text);
		} else {
			writeToLog(CRITICAL_MSG, szBuffer);
		}
	}

	field_len = NSFItemGetText(note_handle, "To", field_text,
		(WORD) sizeof (field_text));
	
	if (field_len > 0) {
		if (redirectAdditionalsCount < MAX_EXCEPTIONS) {
			strcpy(redirectAdditionalsTo[redirectAdditionalsCount],field_text);
		} else {
			writeToLog(CRITICAL_MSG, szBuffer);
		}
	}
	redirectAdditionalsCount++;

	/* Close the note. */
	if (error = NSFNoteClose (note_handle)) {
        return (ERR(error));
	}

	/* End of subroutine. */
    return (NOERROR);
}

int loadErrorPages()
{  
    DBHANDLE		db_handle;      
	char			formula[] = "Form=\"Html\"";	
    FORMULAHANDLE   formula_handle;						
    WORD			wdc;
    STATUS			error = NOERROR;

	//Open the database (prod version and if it does not exists - dev version)
    if (error = NSFDbOpen (db_filename, &db_handle)) {
		if(error = NSFDbOpen (db_filenamedev, &db_handle)) {
			PrintAPIError (error);
			return (1);
		}
	}
    
    /* Compile the selection formula. */
    if (error = NSFFormulaCompile (NULL, (WORD) 0, formula,
                (WORD) strlen(formula), &formula_handle, &wdc, &wdc,
                &wdc, &wdc, &wdc, &wdc)) {
        NSFDbClose (db_handle);
        PrintAPIError (error);  
        return (1);
    }

	//Call "store_exception" for each document matching the search
    if (error = NSFSearch (db_handle, formula_handle, 
							NULL, 0, NOTE_CLASS_DOCUMENT,
							NULL, store_error_pages, &db_handle, 
							NULL)) {
        NSFDbClose (db_handle);
        PrintAPIError (error);  
        return (1);
    }

	/* Free the memory allocated to the compiled formula. */
    OSMemFree (formula_handle);

	/* Close the database. */
    if (error = NSFDbClose (db_handle)) {
        PrintAPIError (error);  
        return (1);
    } 
	
	return (0);
}

STATUS LNPUBLIC store_error_pages(void far *db_handle, SEARCH_MATCH far *pSearchMatch, ITEM_TABLE far *summary_info)
{
    SEARCH_MATCH SearchMatch;
    NOTEHANDLE   note_handle;
    WORD         domain_len;
    char         domain[URL_BUFFER_SIZE];
    WORD         type_len;
    char         type[URL_BUFFER_SIZE];
	WORD         html_len;
    char         html[ERRRO_PAGE_SIZE];
	int			 i;

	// Buffer for building messages for the log
	char		 szBuffer[ERRRO_PAGE_SIZE]={0};
    STATUS       error;

    memcpy( (char*)&SearchMatch, (char*)pSearchMatch, sizeof(SEARCH_MATCH));

	/* Open the note. */
    if (error = NSFNoteOpen (
                *(DBHANDLE far *)db_handle,		/* database handle */
                SearchMatch.ID.NoteID,			/* note ID */
                0,								/* open flags */
                &note_handle)					/* note handle (return) */
				) {
		return (ERR(error));
	}

	type_len = NSFItemGetText(note_handle, "Type", type, (WORD) sizeof (type));
	domain_len = NSFItemGetText(note_handle, "Domain", domain, (WORD) sizeof (domain));
	html_len = NSFItemGetText(note_handle, "HTML", html, (WORD) sizeof (html));
	for(i=0;i<html_len ;i++) {
		if(html[i]=='\0') {
			html[i]='\n';
		}
	}

	if ((html_len > 0) && (domain_len > 0)) {
		if (errorPagesCount < MAX_ERROR_PAGES) {
			strcat(domain, type);
			strcpy(errorKey[errorPagesCount], domain);
			OSTranslate (OS_TRANSLATE_LMBCS_TO_UTF8, html, html_len, errorHTML[errorPagesCount], ERRRO_PAGE_SIZE);
			strcat(errorHTML[errorPagesCount], "<span style=\"display: none;\">\0");
			errorPagesCount++;
		} else {
			//Message to show if there are more configured exceptions than are allowed
			sprintf(szBuffer, "Unable to store error pages (maximum of %d already reached).", MAX_ERROR_PAGES);
			writeToLog(CRITICAL_MSG, szBuffer);
		}
	}

	// Close the note.
	if (error = NSFNoteClose (note_handle)) {
        return (ERR(error));
	}

	/* End of subroutine. */
    return (NOERROR);
}

int loadRewrites()
{  
    DBHANDLE		db_handle;      
	char			formula[] = "Form=\"RewriteRule\"";	
    FORMULAHANDLE   formula_handle;				
    WORD			wdc;
    STATUS			error = NOERROR;

	//Open the database (prod version and if it does not exists - dev version)
    if (error = NSFDbOpen (db_filename, &db_handle)) {
		if(error = NSFDbOpen (db_filenamedev, &db_handle)) {
			PrintAPIError (error);
			return (1);
		}
	}
    
    /* Compile the selection formula. */
    if (error = NSFFormulaCompile (NULL, (WORD) 0, formula,
                (WORD) strlen(formula), &formula_handle, &wdc, &wdc,
                &wdc, &wdc, &wdc, &wdc)) {
        NSFDbClose (db_handle);
        PrintAPIError (error);  
        return (1);
    }

	//Call "store_rewrites" for each document matching the search
    if (error = NSFSearch (db_handle, formula_handle, 
							NULL, 0, NOTE_CLASS_DOCUMENT,
							NULL, store_rewrites, &db_handle, 
							NULL)) {
        NSFDbClose (db_handle);
        PrintAPIError (error);  
        return (1);
    }

	/* Free the memory allocated to the compiled formula. */
    OSMemFree (formula_handle);

	/* Close the database. */
    if (error = NSFDbClose (db_handle)) {
        PrintAPIError (error);  
        return (1);
    } 

	return (0);
}

STATUS LNPUBLIC store_rewrites(void far *db_handle, SEARCH_MATCH far *pSearchMatch, ITEM_TABLE far *summary_info)
{
    SEARCH_MATCH SearchMatch;
    NOTEHANDLE   note_handle;
    WORD         field_len;
    char         field_text[URL_BUFFER_SIZE];
	char		 szBuffer[DEFAULT_BUFFER_SIZE]={0};
    STATUS       error;

	//Message to show if there are more configured exceptions than are allowed
	sprintf(szBuffer, "Unable to store rewrites (maximum of %d already reached).", MAX_REWRITE);

    memcpy( (char*)&SearchMatch, (char*)pSearchMatch, sizeof(SEARCH_MATCH) );

	/* Open the note. */
    if (error = NSFNoteOpen (
                *(DBHANDLE far *)db_handle,		/* database handle */
                SearchMatch.ID.NoteID,			/* note ID */
                0,								/* open flags */
                &note_handle)					/* note handle (return) */
				) {
		return (ERR(error));
	}
    
	field_len = NSFItemGetText(note_handle, "From", field_text, 
		(WORD) sizeof (field_text));
	
	if (field_len > 0) {
		if (rewriteCount < MAX_REWRITE) {
			strcpy(rewriteFrom[rewriteCount],field_text);
		} else {
			writeToLog(CRITICAL_MSG, szBuffer);
		}
	}

	field_len = NSFItemGetText(note_handle, "To", field_text,
		(WORD) sizeof (field_text));
	
	if (field_len > 0) {
		if (rewriteCount < MAX_REWRITE) {
			strcpy(rewriteTo[rewriteCount],field_text);
		} else {
			writeToLog(CRITICAL_MSG, szBuffer);
		}
	}
	rewriteCount++;

	/* Close the note. */
	if (error = NSFNoteClose (note_handle)) {
        return (ERR(error));
	}

	/* End of subroutine. */
    return (NOERROR);
}

/*************************************************************************

    FUNCTION:   PrintAPIError

    PURPOSE:    This function prints the Lotus C API for Domino and Notes 
		error message associated with an error code.

**************************************************************************/

void PrintAPIError (STATUS api_error)
{
    STATUS  string_id = ERR(api_error);
    char    error_text[200];
    WORD    text_len;

    /* Get the message for this Lotus C API for Domino and Notes error code
       from the resource string table. */
    text_len = OSLoadString (NULLHANDLE, string_id, error_text,
                             sizeof(error_text));

    /* Print it. */
	writeToLog(ERROR_MSG, error_text);
}