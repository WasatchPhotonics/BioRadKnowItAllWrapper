#pragma once

/*! @page SearchSDK

    @par GENERAL NOTES:

    To use the search SDK, retrieve the location of the KnowItAll executable as shown by the sample code in the SearchSDK_Test
    project and call LoadLibrary to load the SearchSDK.dll file.

    Call GetProcAddress to retrieve the function pointers as shown in the sample code.

    @par INTERFACE FOR RETURNING SEARCH RESULTS FROM KNOWITALL:

    When KnowItAll detects that a spectrum was passed into ID Expert from an external software (henceforth called "source"), 
    it adds a button to the user interface in ID Expert that allows a user to return search results to the source. 

    The following needs to be done in the source software to allow this transfer to happen:

    - In addition to the "/PlugInGuid=" command line parameter that is passed to 
      KnowItAll when transferring a spectrum, add the following two parameters:
        - "/SourceApplicationWindowHandle=<window handle, hex format>": pass the 
          handle of the window to KnowItAll that should receive the notification 
          message when results are transferred back to the source.
            - Example: /SourceApplicationWindowHandle=07a128bbc
        - "/SourceApplicationName=<name>": pass the name of the source software. This 
          name is used in messages and for user interface elements to identify the 
          source software.
            - Example: /SourceApplicationName="LabSpec 6"

    The window that was designated as the target for receiving results from 
    KnowItAll needs to register a window message by running the following code:

    \code
       UINT msgID = ::RegisterWindowMessage(L"BR:KnowItAllToExternalSourceResults");
    \endcode

    The message that is sent to the source contains a file mapping handle in its LPARAM value and the size of the 
    data in the file mapping object in its WPARAM value. Cast the LPARAM to a HANDLE and use code like the following:

    \code
       const void *pData=::MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0));
    \endcode

    The data is organized as follows:
    - 32 bit integer: number of results
    - For each result:
        - 32 bit integer: match flags (see SEARCHSDK_MATCHFLAG_* constants described above)
        - 64 bit floating point value: match percentage from 0 to 1.
        - 64 bit floating point value: mixture search component weight from 0 to 1.
        - 32 bit integer: number of characters (not bytes) in the following UTF-16 string.
        - UTF-16 string: name of the record, not zero-terminated.

    Do not call CloseHandle on the file mapping object when you are done accessing the data. This is done by
    KnowItAll. KnowItAll creates a file mapping object that should work within the address space of the 
    source process.
*/

typedef void* SEARCHSDK_HANDLE;

#define SEARCHSDK_TECHNIQUE_FTIR           0x00000001
#define SEARCHSDK_TECHNIQUE_ATRIR          0x00000002
#define SEARCHSDK_TECHNIQUE_RAMAN          0x00000003
#define SEARCHSDK_TECHNIQUE_VAPORPHASEIR   0x00000004
#define SEARCHSDK_TECHNIQUE_MS             0x00000005

#define SEARCHSDK_XUNIT_WAVENUMBERS        0x0001
#define SEARCHSDK_XUNIT_NANOMETERS         0x0002
#define SEARCHSDK_XUNIT_MOVERZ             0x0003

#define SEARCHSDK_YUNIT_ARBITRARYINTENSITY 0x0001
#define SEARCHSDK_YUNIT_ABSORBANCE         0x0002
#define SEARCHSDK_YUNIT_TRANSMITTANCE      0x0003

#define SEARCHSDK_MATCHFLAG_SPECTRALSEARCHRESULT 0x00000001 //!< Spectral search result
#define SEARCHSDK_MATCHFLAG_PEAKSEARCHRESULT     0x00000002 //!< Peak search result
#define SEARCHSDK_MATCHFLAG_COMPOSITE	         0x00000004 //!< Mixture search composite result
#define SEARCHSDK_MATCHFLAG_RESIDUAL             0x00000008 //!< Mixture search residual result
#define SEARCHSDK_MATCHFLAG_COMPONENT            0x00000010 //!< Mixture search component result
#define SEARCHSDK_MATCHFLAG_LOCKED               0x00000020 //!< Locked (unlicensed) result

#pragma pack(push, 1)
struct SearchSDK_Match
{
	double m_matchPercentage; //!< From 0 to 100.
	wchar_t *m_matchName;     //!< The string pointers are deleted automatically by the SearchSDK_CloseSearchFn function.
	bool m_bLocked;           //!< The match comes from an unlicensed database.
};
#pragma pack(pop)

//! Call at the earliest time possible, preferably after application start-up right after the DLL has been loaded.
//! This allows the scanning thread to find all available databases, thus speeding up the initialization part of the 
//! SearchSDK_RunSearch functions.
typedef void (*SearchSDK_InitFn)();

//! Call before application termination. This function exits all processing threads.
typedef void (*SearchSDK_ExitFn)();

//! Creates a search object and returns its handle.
//! @Return search handle.
typedef SEARCHSDK_HANDLE (*SearchSDK_OpenSearchFn)();

//! Deletes a search object and all name pointers from returned search results.
//! @param hSearch search handle.
typedef bool (*SearchSDK_CloseSearchFn)(SEARCHSDK_HANDLE hSearch);

/*! Runs a search synchronously on evenly spaced data. 
    @param hSearch   search handle
    @param technique one of the SEARCHSDK_TECHNIQUE_* constants as defined above.
    @param yArray    pointer to the Y axis (intensity) array.
    @param arrayCnt  number of entries in the yArray vector.
    @param firstX    X axis value of the first data point.
    @param lastX     X axis value of the last data point.
    @param xUnit     one of the SEARCHSDK_XUNIT_* constants as defined above.
    @param yUnit     one of the SEARCHSDK_YUNIT_* constants as defined above.
    @param pResults  pointer to an array of SearchSDK_Match objects. The array 
                     will be filled with the results from the search.
    @param pnResults on function entry, defines the size of the pResults array 
                     that is passed in. On function exit, contains the number of
                     entries with valid results. Note: the returned count may be
                     lower than the passed in count if the search did not find as
                     many entries as the array size that was passed in.
*/
typedef bool (*SearchSDK_RunSearchEvenlySpacedFn)(SEARCHSDK_HANDLE hSearch, 
                                                  unsigned int technique, 
                                                  const double* yArray, 
                                                  int arrayCnt, 
                                                  double firstX, 
                                                  double lastX, 
                                                  unsigned short xUnit, 
                                                  unsigned short yUnit, 
                                                  SearchSDK_Match* pResults, 
                                                  int* pnResults);

/*! Runs a search synchronously on unevenly spaced data. 

    @param hSearch   search handle.
    @param technique one of the SEARCHSDK_TECHNIQUE_* constants as defined above.
    @param xArray    pointer to the X axis (frequency) array.
    @param yArray    pointer to the Y axis (intensity) array.
    @param arrayCnt  number of entries in the xArray and yArray vectors.
    @param xUnit     one of the SEARCHSDK_XUNIT_* constants as defined above.
    @param yUnit     one of the SEARCHSDK_YUNIT_* constants as defined above.
    @param pResults  pointer to an array of SearchSDK_Match objects. The array 
                     will be filled with the results from the search.
    @param pnResults on function entry, defines the size of the pResults array 
                     that was passed in. On function exit, contains the number of
                     entries with valid results. Note: the returned count may be 
                     lower than the passed in count if the search did not find as
                     many entries as the array size that was passed in.
*/
typedef bool (*SearchSDK_RunSearchUnevenlySpacedFn)(SEARCHSDK_HANDLE hSearch, 
                                                    unsigned int technique, 
                                                    const double* xArray, 
                                                    const double* yArray, 
                                                    int arrayCnt, 
                                                    unsigned short xUnit, 
                                                    unsigned short yUnit, 
                                                    SearchSDK_Match* pResults, 
                                                    int* pnResults);

//! Causes a running search to be canceled. Call this function from a different thread than the one 
//! that called the search function.
//! @param hSearch search handle.
typedef bool (*SearchSDK_CancelSearchFn)(SEARCHSDK_HANDLE hSearch);

//! Retrieves a progress value for the currently running search. Call this function from a different thread than the one 
//! that called one of the search functions.
//! @param hSearch: search handle.
//! @return progress percentage from 0-100.
typedef double (*SearchSDK_GetProgressPercentageFn)(SEARCHSDK_HANDLE hSearch);
