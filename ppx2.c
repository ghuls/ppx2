/* -*-c-*- */
/*
  gcc -g -O0 -mthreads -o ppx2.exe -Wall ppx2.c -lKernel32
  gcc -mthreads -O2 -o ppx2.exe -Wall ppx2.c -lKernel32
  -Wl,--allow-multiple-definition


  gcc -g -O0 -mthreads -o ppx2.exe -Wall ppx2.c -lKernel32 P:\Temp\mpatrol\tags\REL_1_5_1\build\windows\libmpatrol.a -limagehlp
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define _GNU_SOURCE
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

WCHAR *pwReplaceText = NULL;
int nParallelProcesses = 1;
int nSplitByLine = 0;
int nVerbose = 0;
WCHAR *pwGlobalCmdLine = NULL;
HANDLE hSemaphore = NULL;
CRITICAL_SECTION sCriticalSection;

void *mymalloc( size_t length ) {
    void *pResult;

    EnterCriticalSection( &sCriticalSection );
    pResult = malloc( length );
    LeaveCriticalSection( &sCriticalSection );
    return( pResult );
}


void myfree( void *memory ) {
    EnterCriticalSection( &sCriticalSection );
    free( memory );
    LeaveCriticalSection( &sCriticalSection );
}

void print_wchar( WCHAR const *pStart, int length ) {
    int counter;
    for ( counter = 0; counter < length; counter++ ) {
        char cChar = *( pStart + counter ) & 0xFF;
        if ( ( cChar >= 20 ) && ( cChar <= 127 ) )
            printf( "%c", cChar );
        else
            printf( "%c", '.' );
    }
}

WCHAR const *find_paramw_end( WCHAR const *paramStart ) {
    WCHAR const *paramScan = paramStart;

    if ( ( *paramScan == '\"' ) || ( *paramScan == '\'' ) ) {
        WCHAR cMatch = *paramScan;

        paramScan++; /* skip opening quotes */

        while ( *paramScan && ( *paramScan != cMatch ) )
            paramScan++;

        if ( *paramScan == cMatch )
            return( paramScan + 1 );
        else
            return( paramScan );
    }

    while ( *paramScan && ( ! isspace( *paramScan ) ) )
        paramScan++;

    return( paramScan );
}

char const *find_param_end( char const *paramStart ) {
    char const *paramScan = paramStart;

    if ( ( *paramScan == '\"' ) || ( *paramScan == '\'' ) ) {
        char cMatch = *paramScan;

        paramScan++; /* skip opening quotes */

        while ( *paramScan && ( *paramScan != cMatch ) )
            paramScan++;

        if ( *paramScan == cMatch )
            return( paramScan + 1 );
        else
            return( paramScan );
    }

    while ( *paramScan && ( ! isspace( *paramScan ) ) )
        paramScan++;

    return( paramScan );
}

WCHAR const *find_paramw_start( WCHAR const *spaceStart ) {
    WCHAR const *paramScan = spaceStart;

    while ( *paramScan && ( isspace( *paramScan ) ) )
        paramScan++;

    return( paramScan );
}

char const *find_param_start( char const *spaceStart ) {
    char const *paramScan = spaceStart;

    while ( *paramScan && ( isspace( *paramScan ) ) )
        paramScan++;

    return( paramScan );
}

void initialise( void ) {
    InitializeCriticalSection( &sCriticalSection );

    pwReplaceText = mymalloc( sizeof(WCHAR) * 3 );
    pwReplaceText[0] = '{';
    pwReplaceText[1] = '}';
    pwReplaceText[2] = '\0';
}

WCHAR *extract_argumentw( WCHAR const *pArg ) {
    WCHAR const *pRealStart = pArg;
    WCHAR const *pRealEnd = pArg;

    if ( ( *pArg == '\"' ) || ( *pArg == '\'' ) ) {
        pRealStart = pArg + 1;
        pRealEnd = pRealStart;
        while ( *pRealEnd && ( *pRealEnd != *pArg ) )
            pRealEnd++;
    } else {
        pRealEnd = find_paramw_end( pRealStart );
    }

    int nLength = pRealEnd - pRealStart;

    WCHAR *pReturn = mymalloc( sizeof(WCHAR) * (1 + nLength) );
    memcpy( pReturn, pRealStart, sizeof(WCHAR) * nLength );
    pReturn[nLength] = '\0';
    return( pReturn );
}

char *extract_argument( char const *pArg ) {
    char const *pRealStart = pArg;
    char const *pRealEnd = pArg;

    if ( ( *pArg == '\"' ) || ( *pArg == '\'' ) ) {
        pRealStart = pArg + 1;
        pRealEnd = pRealStart;
        while ( *pRealEnd && ( *pRealEnd != *pArg ) )
            pRealEnd++;
    } else {
        pRealEnd = find_param_end( pRealStart );
    }

    int nLength = pRealEnd - pRealStart;

    char *pReturn = mymalloc( sizeof(char) * (1 + nLength) );
    memcpy( pReturn, pRealStart, sizeof(char) * nLength );
    pReturn[nLength] = '\0';
    return( pReturn );
}

char *extract_argumentw_narrow( WCHAR const *pArg ) {
    WCHAR const *pRealStart = pArg;
    WCHAR const *pRealEnd = pArg;

    if ( ( *pArg == '\"' ) || ( *pArg == '\'' ) ) {
        pRealStart = pArg + 1;
        pRealEnd = pRealStart;
        while ( *pRealEnd && ( *pRealEnd != *pArg ) )
            pRealEnd++;
    } else {
        pRealEnd = find_paramw_end( pRealStart );
    }

    int nLength = pRealEnd - pRealStart;

    char *pReturn = mymalloc( (1 + nLength) );
    char *pInto = pReturn;
    while ( pRealStart < pRealEnd ) {
        *pInto = ( *pRealStart & 0x7F );
        pInto++;
        pRealStart++;
    }
    *pInto = '\0';

    return( pReturn );
}

void cleanup( void ) {
    if ( pwReplaceText )
        myfree( (void *)pwReplaceText );

    if ( hSemaphore )
        CloseHandle( hSemaphore );

    DeleteCriticalSection( &sCriticalSection );
}

void exit_with_code( int code ) {
    cleanup();
    exit( code );
}

void print_error( void ) {
    DWORD dwLastError = GetLastError();

    char acBuffer[1024];

    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM, /* flags */
        NULL, /* source */
        dwLastError, /* message ID */
        0, /* language */
        acBuffer, /* buffer */
        sizeof(acBuffer) - 1, /* buffer size */
        NULL /* arguments */
    );

    fprintf( stderr, "%s", acBuffer );
}

struct threaddata {
    WCHAR *pwCmdLine;
};

DWORD WINAPI ThreadProc( LPVOID lpParameter ) {
    /* new thread */
    struct threaddata *pThreadData;

    pThreadData = (struct threaddata *)lpParameter;

    STARTUPINFOW sStartupInfo;
    PROCESS_INFORMATION sProcessInformation;
    memset( &sStartupInfo, 0, sizeof(sStartupInfo) );
    sStartupInfo.cb = sizeof( sStartupInfo );
    memset( &sProcessInformation, 0, sizeof(sProcessInformation) );

    /* create process */
    BOOL bResult = CreateProcessW(
        NULL, /* application name */
        pThreadData->pwCmdLine, /* command line */
        NULL, /* security attributes */
        NULL, /* thread attributes */
        FALSE, /* inherit handles from parent */
        0, /* creation flags */
        NULL, /* environment */
        NULL, /* current directory */
        &sStartupInfo, /* startup info */
        &sProcessInformation /* process information */
    );
    if ( bResult == TRUE ) {
        WaitForSingleObject( sProcessInformation.hProcess, INFINITE );
        CloseHandle( sProcessInformation.hProcess );
        CloseHandle( sProcessInformation.hThread );
    } else {
        fprintf( stderr, "  failed to create process\n" );
        print_error();
    }

    /* release memory */
    myfree( (void *)pThreadData->pwCmdLine );
    myfree( (void *)pThreadData );

    ReleaseSemaphore( hSemaphore, 1, NULL );
    return( 0 );
}

void do_argument( char const *pArg, WCHAR const *pwLocalCmdLine ) {
    if ( nVerbose ) {
        printf( "  do_argument( \"%s\" )\n", pArg );
    }

    WaitForSingleObject( hSemaphore, INFINITE );

    /* we have to interleave pArg into all occurances of pwReplaceText
       in pwCmdLine */

    /* first count number of instances of pwReplaceText in pwCmdLine */
    WCHAR const *pwFind = pwLocalCmdLine;
    int nOccurances = 0;
    while ( pwFind ) {
        pwFind = wcsstr( pwFind, pwReplaceText );
        if ( pwFind ) {
            nOccurances++;
            pwFind += wcslen( pwReplaceText );
        }
    }

    int nNewLength = (
        wcslen( pwLocalCmdLine ) +
        ( nOccurances * strlen( pArg ) ) -
        ( nOccurances * wcslen( pwReplaceText ) )
    );

    WCHAR *pwFullCmd = mymalloc( sizeof(WCHAR) * ( nNewLength + 1 ) );
    WCHAR *pwInto = pwFullCmd;
    WCHAR const *pwFrom = pwLocalCmdLine;
    pwFind = pwLocalCmdLine;
    while ( pwFind ) {
        pwFind = wcsstr( pwFind, pwReplaceText );
        if ( pwFind ) {
            memcpy( pwInto, pwFrom, sizeof(WCHAR) * ( pwFind - pwFrom ) );
            pwInto += ( pwFind - pwFrom );
            pwFind += wcslen( pwReplaceText );
            pwFrom = pwFind;

            char const *pFrom = pArg;
            while ( *pFrom ) {
                *pwInto = *pFrom;
                pwInto++;
                pFrom++;
            }
        }
    }

    pwFind = pwFrom;
    while ( *pwFind )
        pwFind++;

    memcpy( pwInto, pwFrom, sizeof(WCHAR) * ( pwFind - pwFrom ) );
    pwInto += ( pwFind - pwFrom );
    *pwInto = '\0';

    if ( nVerbose ) {
        printf( "  - full cmd \"" );
        print_wchar( pwFullCmd, wcslen( pwFullCmd ) );
        printf( "\"\n" );
    }

    struct threaddata *pThreadData = mymalloc( sizeof(struct threaddata) );
    pThreadData->pwCmdLine = pwFullCmd;

    /* create a thread */
    HANDLE hThread = CreateThread(
        NULL, /* security attributes */
        0, /* stack size */
        ThreadProc, /* pointer to thread procedure */
        pThreadData, /* data */
        0, /* immediately start thread */
        NULL /* thread identifier */
    );

    CloseHandle( hThread ); /* don't care about tracking thread */
}

void help( void ) {
    FILE *fout = stdout;

    static char acMessage[] = ""
    "ppx2 v0.1a\n"
    "\n"
    "An xargs-like clone for Windows with multi-processing\n"
    "\n"
    "Syntax:\n"
    "  ppx2 [-P <n>] [-I <string>] [-L <max-lines>] [--] <cmd-text>\n"
    "\n"
    "Where:\n"
    "  -P <n> - maximum number of simultaneous processes, default 1\n"
    "  -I <string> - text to replace in cmd-text with argument, default {}\n"
    "  -L <max-lines> - how many lines to treat as argument\n"
    "  --help - this help\n"
    "\n"
    "Notes:\n"
    "  By default this reads in items separated by whitespace or contained\n"
    "  in quotes or double quotes. If you want this to process line-by-line\n"
    "  (as many Windows filenames contain spaces) then use the \"-L 1\" option.\n"
    "\n"
    "Examples:\n"
    "  DIR /B *.MPG |PPX2 -P 4 -L 1 -I {} FFMPEG.EXE -i {} -quality:v 1 -an {}.mp4\n"
    "  - for a 4 processor system converting mpg files to mp4\n"
    "\n"
    "  DIR C:\\ /S /B |PPX2 -P 4 -I {} sha1sum.exe \"{}\"\n"
    "";

    fprintf( fout, "%s", acMessage );
}

int main( void ) {
    WCHAR const *pMyPtr = GetCommandLineW();
    WCHAR const *pScanStart;

    initialise();

    /* find out where the end of the calling parameter is */
    pScanStart = find_paramw_end( pMyPtr );
    WCHAR const *pArgsStart = find_paramw_start( pScanStart );

    /* find out where the real parameters begin */
    WCHAR const *pCmdStart = pArgsStart;
    while ( *pCmdStart ) {
        int index = 0;
        int nextisstart = 0;

        if ( ( *pCmdStart == '\"' ) || ( *pCmdStart == '\'' ) )
            index = 1;

        if ( pCmdStart[index] != '-' ) /* found beginning of command? */
            break;

        if ( pCmdStart[index + 1] == '-' ) { /* uh, oh, double switch */
            char *pArg = extract_argumentw_narrow( pCmdStart + index + 2 );

            if ( ! pCmdStart[index + 2] ) {
                nextisstart = 1;
            } else if ( strcmp( pArg, "help" ) == 0 ) {
                myfree( (void *)pArg );
                help();
                exit_with_code( 1 );
            } else {
                fprintf( stderr, "Error, unknown switch \"%s\"\n", pArg );
                myfree( (void *)pArg );
                exit_with_code( 1 );
            }

            myfree( (void *)pArg );
        } else if ( toupper( pCmdStart[index + 1] ) == 'I' ) {
            /* grab next argument */
            WCHAR const *pNextArgStart = pCmdStart;
            pNextArgStart = find_paramw_end( pNextArgStart );
            pNextArgStart = find_paramw_start( pNextArgStart );
            WCHAR const *pNextArgEnd = find_paramw_end( pNextArgStart );
            WCHAR const *pArgAfter = find_paramw_start( pNextArgEnd );

            if ( pwReplaceText )
                myfree( (void *)pwReplaceText );

            pwReplaceText = extract_argumentw( pNextArgStart );
            pCmdStart = pArgAfter;

            if ( ! *pwReplaceText ) {
                fprintf( stderr, "Error, must provide replacement text for -I switch\n" );
                exit_with_code( 1 );
            }

            continue;
        } else if ( pCmdStart[index + 1] == 'P' ) {
            /* grab next argument */
            WCHAR const *pNextArgStart = pCmdStart;
            pNextArgStart = find_paramw_end( pNextArgStart );
            pNextArgStart = find_paramw_start( pNextArgStart );
            WCHAR const *pNextArgEnd = find_paramw_end( pNextArgStart );
            WCHAR const *pArgAfter = find_paramw_start( pNextArgEnd );

            char *pNextArg = extract_argumentw_narrow( pNextArgStart );
            pCmdStart = pArgAfter;

            int nParallel = atoi( pNextArg );
            myfree( (void *)pNextArg );
            if ( nParallel < 1 ) {
                fprintf( stderr, "Error, max parallel must be a positive integer\n" );
                exit_with_code( 1 );
            }

            nParallelProcesses = nParallel;
            continue;
        } else if ( pCmdStart[index + 1] == 'L' ) {
            /* grab next argument */
            WCHAR const *pNextArgStart = pCmdStart;
            pNextArgStart = find_paramw_end( pNextArgStart );
            pNextArgStart = find_paramw_start( pNextArgStart );
            WCHAR const *pNextArgEnd = find_paramw_end( pNextArgStart );
            WCHAR const *pArgAfter = find_paramw_start( pNextArgEnd );

            char *pNextArg = extract_argumentw_narrow( pNextArgStart );
            pCmdStart = pArgAfter;

            int nLines = atoi( pNextArg );
            myfree( (void *)pNextArg );
            if ( nLines < 0 ) {
                fprintf( stderr, "Error, max-lines must be a positive or zero integer\n" );
                exit_with_code( 1 );
            }

            nSplitByLine = nLines;
            continue;
        } else if ( pCmdStart[index + 1] == 't' ) {
            /* verbose */
            nVerbose = 1;
        } else {
            fprintf( stderr, "Error, unsupported switch \"%c\"\n", pCmdStart[index + 1] );
            exit_with_code( 1 );
        }

        pCmdStart = find_paramw_end( pCmdStart );
        pCmdStart = find_paramw_start( pCmdStart );
        if ( nextisstart )
            break;
    }

    if ( ! *pCmdStart ) {
        fprintf( stderr, "Error, no command line provided to execute\n" );
        exit_with_code( 1 );
    }

    if ( nVerbose ) {
        printf( "- args start = " );
        print_wchar( pArgsStart, wcslen( pArgsStart ) );
        printf( "\n" );

        printf( "- cmd start = " );
        print_wchar( pCmdStart, wcslen( pCmdStart ) );
        printf( "\n" );

        printf( "- replace text = \"" );
        print_wchar( pwReplaceText, wcslen( pwReplaceText ) );
        printf( "\"\n" );

        printf( "- parallel = %d\n", nParallelProcesses );
    }

    /* create global semaphore for indicating thread availability */
    hSemaphore = CreateSemaphore(
        NULL, /* security attributes */
        (LONG)nParallelProcesses, /* initial count */
        (LONG)nParallelProcesses, /* maximum count */
        NULL /* no name */
    );

    /* process each line, and within each line */
    char acLineBuffer[8196];
    while ( fgets( acLineBuffer, sizeof(acLineBuffer)-1, stdin ) ) {
        char const *pStart = acLineBuffer;
        char const *pEnd = pStart;

        if ( nSplitByLine == 0 ) {
            while ( *pEnd ) {
                char *pArg = extract_argument( pStart );
                pEnd = find_param_end( pStart );
                pStart = find_param_start( pEnd );

                if ( *pArg )
                    do_argument( pArg, pCmdStart ); /* create thread/process */

                myfree( (void *)pArg );
            }
        } else {
            while ( *pEnd && ( *pEnd != '\n' ) && ( *pEnd != '\r' ) )
                pEnd++;

            char *pArg = mymalloc( 1 + (pEnd - pStart) );
            memcpy( pArg, pStart, pEnd - pStart );
            pArg[pEnd - pStart] = '\0';

            do_argument( pArg, pCmdStart );

            myfree( (void *)pArg );
        }
    }

    /* wait for all other threads to finish by waiting for clear semaphores */
    int procs;
    for ( procs = 0; procs < nParallelProcesses; procs++ )
        WaitForSingleObject( hSemaphore, INFINITE );

    /* release acquired semaphores */
    for ( procs = 0; procs < nParallelProcesses; procs++ )
        ReleaseSemaphore( hSemaphore, 1, NULL );

    cleanup();
    return( 0 );
}
