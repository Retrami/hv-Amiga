/*
** Hex Viewer for AmigaOS 3.2
** Version 1.0.0
*/

///
/* INCLUDES */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dos.h>

#include <dos/dos.h>
#include <dos/rdargs.h>

///
/* DEFINES */
#define ARGTEMPLATE "FILE/A"
#define ARG_FILE 0
#define MAX_ARGS 1
#define ZERO 0

///
/* GLOBALS */
int RetCode = RETURN_OK;
char InFilename[256];

///
/* PROTOTYPES */
#include <proto/dos.h>

BOOL init(int, char **);
void cleanup(void);
void dumpFile(BPTR);

///
/* main */
int main(int argc, char *argv[])
{
    atexit(cleanup);

    if(init(argc, argv))
    {
        BPTR InFile;

        InFile = Open(InFilename, MODE_OLDFILE);
        if(InFile != ZERO)
        {
            struct FileInfoBlock *fib;

            // We can only accept files of less than 64KB.
            fib = AllocDosObjectTags(DOS_FIB, TAG_END);
            if(fib)
            {
                if(ExamineFH(InFile, fib))
                {
                    if(fib->fib_Size & 0xFFFF0000)
                    {
                        puts("ERROR: Sorry. Only files less than 64KB are accepted.");
                        RetCode = 5;
                    }
                    else
                    {
                        dumpFile(InFile);
                    }
                }
                else
                {
                    PrintFault(IoErr(), "ERROR:");
                    RetCode = 10;
                }
                FreeDosObject(DOS_FIB, fib);
            }
            else
            {
                PrintFault(IoErr(), "ERROR:");
                RetCode = 10;
            }
        }
        else
        {
            printf("ERROR: Could not open %s for input!\n", InFilename);
        }
    }
    return RetCode;
}

///
/* init */
BOOL init(int argc, char *argv[])
{
    struct RDArgs *rdArgs;
    LONG args[MAX_ARGS];
    BOOL ok = TRUE;

    // This program is to be run from the CLI only.
    if(argc == 0)
    {
        ok = FALSE;
    }
    else
    {
        // Get the arguments from the command line.
        rdArgs = ReadArgs(ARGTEMPLATE, args, NULL);
        if(rdArgs != NULL)
        {
            char *fileName = (char *)args[ARG_FILE];
            if(strlen(fileName) < 256)
            {
                strncpy(InFilename, fileName, 256);
            }
            else
            {
                puts("ERROR: Filename is too long\n");
                ok = FALSE;
                RetCode = 10;
            }
            FreeArgs(rdArgs);
        }
        else
        {
            printf("USAGE: %s %s\n", argv[0], ARGTEMPLATE);
            ok = FALSE;
        }
    }
    
    return ok;
}

///
/* cleanup */
void cleanup(void)
{
    if(InFile != ZERO) Close(InFile);
}

///
/* dumpFile */
void dumpFile(BPTR fh)
{
    int i;
    int offset = 0;
    BOOL done = FALSE;
    char hexLine[80];
    char hex8Str[3];
    UBYTE asciiChar;

    while(!done)
    {
        for(i=0; i<16; ++i)
        {
            LONG ch = FGetC(fh);

            if(ch == -1)
            {
                LONG err = IoErr();
                
                if(err != 0)
                    PrintFault(err, "ERROR: ");

                done = TRUE;
                break;
            }

            if(i == 0)
            {
                // First char of the line. Init and place address.
                sprintf(hexLine, "%04X:                                                 |                 ", offset);
            }

            // Write out the hex value.
            asciiChar = (UBYTE)ch;
            sprintf(hex8Str, "%02X", (UBYTE)ch);
            strncpy(&hexLine[6+(3*i)], hex8Str, 2);

            // Write out the ASCII value.
            if(asciiChar < ' ' || asciiChar > '~')
                asciiChar = '.';
            
            memcpy(&hexLine[56+i], &asciiChar, 1);
        }

        puts(hexLine);
        offset += 16;
    }
}
