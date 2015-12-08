# domino-dsapi-handler
DSAPI filter for Domino

1. DSAPI Filter Web Site Configuration

To make use of the DSAPI Filter, the SEOUrlHandler.dll file must be copied into the IBM Lotus Domino server program directory (where the nserver.exe file is located).  The Web Server configuration must also be updated to include the filter in the list of DSAPI Filters to be loaded when the http task is started (found in the Domino Administrator client, Configuration > Web > Internet Sites):
DSAPI Filters
DSAPI filter file names:	urlhandler

The Filter will pick up configuration settings from a database called "urlhand.nsf" - this database should be located in the Domino server data directory (where names.nsf is located).  If the database is not found, the filter will use default settings and no customized options or exceptions will be loaded.  The database does not need to be accessible by general users, and the ACL should be set to allow only Administrators access.

2. Release Build (64 bit)

-------------------------------------------------------------------------------------
version 1.6 [Release from 2013-03-15]
-------------------------------------------------------------------------------------
version 1.3 [Release from 2012-06-21]
-------------------------------------------------------------------------------------
version 1.2 [Release from 2012-01-27]
-------------------------------------------------------------------------------------
version 1.1 [Release from 2011-11-02]
-------------------------------------------------------------------------------------
version 1.0.1 [Release from 2011-10-05]
-------------------------------------------------------------------------------------
First version 2011-09-11
New version 2011-09-15
Version from 2011-10-05


3. DSAPI Filter Restrictions

The filter has been compiled with the following restrictions:
The configuration database must have the filename "urlhand.nsf" and be located in the Domino data directory.
The maximum supported number of exceptions is 512.
The maximum URL length is 4096 bytes, matching the default Domino setting.
If any of these items need to be changed then the filter will need to be rebuilt - for details see the document titled "How to build from source".

4. How to build from source

The supplied dll was built using Microsoft Visual Studio 2010 Professional.

External requirements:
The IBM Lotus C API 8.5.2 is required in order to build the filter from source.  This is available as a download from IBM.  For the included build this was located in the directory "C:\Data\Lotus\Lotus C API 8.5.2".

Compiler command line used for the build:

/I"C:\Data\Lotus\Lotus C API 8.5.2\notesapi\include" /Zi /nologo /W3 /WX- /O2 /Oi /GL /D "NT" /D "W32" /D "W64" /D "WIN32" /D "_WINDOWS" /D "NDEBUG" /D "_USRDLL" /D "_CRT_SECURE_NO_WARNINGS" /D "_WINDLL" /D "_UNICODE" /D "UNICODE" /D "_AFXDLL" /Gm- /EHsc /GS /Gy /fp:precise /Zc:wchar_t /Zc:forScope /Fp"x64\Release\SEOUrlHandler.pch" /Fa"x64\Release\" /Fo"x64\Release\" /Fd"x64\Release\vc100.pdb" /Gd /TC /errorReport:queue

New code from 2011-09-13, fixing the missing DLL:
/I"C:\Data\Lotus\Lotus C API 8.5.2\notesapi\include" /Zi /nologo /W3 /WX- /O2 /Oi /GL /D "NT" /D "W32" /D "W64" /D "WIN32" /D "_WINDOWS" /D "NDEBUG" /D "_USRDLL" /D "_CRT_SECURE_NO_WARNINGS" /D "_WINDLL" /D "_UNICODE" /D "UNICODE" /Gm- /EHsc /MT /GS /Gy /fp:precise /Zc:wchar_t /Zc:forScope /Fp"x64\Release\SEOUrlHandler.pch" /Fa"x64\Release\" /Fo"x64\Release\" /Fd"x64\Release\vc100.pdb" /Gd /TC /errorReport:queue

Linker command line used for the build:

/OUT:"C:\Users\Dmytro\Documents\Visual Studio 2010\Projects\SEOUrlHandler\x64\Release\SEOUrlHandler.dll" /INCREMENTAL:NO /NOLOGO /LIBPATH:"C:\Data\Lotus\Lotus C API 8.5.2\notesapi\lib\mswin64" /DLL "notes.lib" /MANIFEST /ManifestFile:"x64\Release\SEOUrlHandler.dll.intermediate.manifest" /ALLOWISOLATION /MANIFESTUAC:"level='asInvoker' uiAccess='false'" /DEBUG /PDB:"C:\Users\Dmytro\Documents\Visual Studio 2010\Projects\SEOUrlHandler\x64\Release\SEOUrlHandler.pdb" /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF /PGD:"C:\Users\Dmytro\Documents\Visual Studio 2010\Projects\SEOUrlHandler\x64\Release\SEOUrlHandler.pgd" /LTCG /TLBID:1 /DYNAMICBASE /NXCOMPAT /MACHINE:X64 /ERRORREPORT:QUEUE

6. License

The source code is released under the GPL-3.0 license.
