/******************************************************************************
*
*   Copyright (C) 1999-2000, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************/
//
// uconv demonstration example of ICU and codepage conversion
// Purpose is to be a similar tool as the UNIX iconv program.
//
// Usage: uconv [flag] [file]
// -f [codeset]  Convert file from this codeset
// -t [codeset]  Convert file to this code set
// -l            Display all available converters
// -x [transliterator]  Run everything through a transliterator
// -L            Display all available transliterators
// If no file is given, uconv tries to read from stdin
// 
// To compile: c++ -o uconv -I${ICUHOME}/include -Wall -g uconv.cpp -L${ICUHOME}/lib -licuuc -licui18n
//
// Original contributor was Jonas Utterström <jonas.utterstrom@vittran.norrnod.se> in 1999
// Converted to the C conversion API by Yves Arrouye <yves@realnames.com>. 
//
// Permission is granted to use, copy, modify, and distribute this software
//

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "cmemory.h"

// This is the UConverter headerfile
#include "unicode/ucnv.h"

// This is the UnicodeString headerfile
#include "unicode/unistr.h"

// Our message printer..
#include "unicode/uwmsg.h"

#ifdef WIN32
#include <string.h>
#include <io.h>
#include <fcntl.h>
#endif

#ifdef USE_TRANSLIT
# include "unicode/translit.h"
#endif

static const size_t buffsize = 4096;

static UResourceBundle *gBundle = 0;

static void initMsg(const char *pname) {
    static int ps = 0;

    if (!ps) {
        char dataPath[500];
        UErrorCode err = U_ZERO_ERROR;

        ps = 1;

        /* Get messages. */
        
        strcpy(dataPath, u_getDataDirectory());
        strcat(dataPath, "uconvmsg");
        
        gBundle = u_wmsg_setPath(dataPath, &err);
        if(U_FAILURE(err))
            {
                fprintf(stderr, "%s: warning: couldn't open resource bundle %s: %s\n", 
                        pname,
                        dataPath,
                        u_errorName(err));
            }
    }
}

// Print all available codepage converters
static int printConverters(const char *pname, const char *lookfor, int canon)
{
    UErrorCode err = U_ZERO_ERROR;

    int32_t num;
    uint16_t num_stds;
    const char **stds;

    if (lookfor) {
        if (!canon) {
            printf("%s\n", lookfor);
            return 0;
        } else {
            /* We've done that already except for the default name. Oh well. */

            const char *truename = ucnv_getAlias(lookfor, 0, &err);
            if (U_SUCCESS(err)) {
                lookfor = truename;
            } else {
                err = U_ZERO_ERROR;
            }
        }
    }

    num = ucnv_countAvailable();
    num_stds = ucnv_countStandards();
    stds = (const char **) uprv_malloc(num_stds * sizeof(*stds));
    
    if (!stds) {
        u_wmsg("cantGetTag", u_wmsg_errorName(U_MEMORY_ALLOCATION_ERROR));
        return -1;
    } else {
        uint16_t s;
            
        for (s = 0; s < num_stds; ++s) {
            stds[s] = ucnv_getStandard(s, &err);
            if (U_FAILURE(err)) {
                u_wmsg("cantGetTag", u_wmsg_errorName(err));
                return -1;
            }
        }
    }
        
#if 0
    size_t numprint = 0;
    static const size_t maxline = 70;
#endif
        
    if (num <= 0)
        {
            initMsg(pname);   
            u_wmsg("cantGetNames");
            return -1;
        }
        
    for (int32_t i = 0; i<num; i++)
        {
            // ucnv_getAvailableName gets the codepage name at a specific
            // index
                
            const char *name = ucnv_getAvailableName(i);
            uint16_t num_aliases;

            if (lookfor && ucnv_compareNames(lookfor, name)) {
                continue;
            }
                
#if 0
            numprint += printf("%-20s", name);
            if (numprint>maxline)
                {
                    putchar('\n');
                    numprint = 0;
                }
#else
            err = U_ZERO_ERROR;
            num_aliases = ucnv_countAliases(name, &err);
            if (U_FAILURE(err)) {
                printf("%s", name);
                    
                UnicodeString str(name);
                putchar('\t');
                u_wmsg("cantGetAliases", str.getBuffer(), u_wmsg_errorName(err));
                return -1;
            } else {
                uint16_t a, s, t;
                    
                for (a = 0; a < num_aliases; ++a) {
                    const char *alias = ucnv_getAlias(name, a, &err);
                        
                    if (U_FAILURE(err)) {
                        UnicodeString str(name);
                        putchar('\t');
                        u_wmsg("cantGetAliases", str.getBuffer(), u_wmsg_errorName(err));
                        return -1;
                    }
                        
                    printf("%s", alias);
                        
                    /* Look (slowly) for a tag. */
                        
                    if (canon) {
                        for (s = t = 0; s < num_stds; ++s) {
                            const char *standard = ucnv_getStandardName(name, stds[s], &err);
                            if (U_SUCCESS(err) && standard) {
                                if (!strcmp(standard, alias)) {
                                    if (!t) {
                                        printf(" {");
                                        t = 1;
                                    }
                                    printf(" %s", stds[s]);
                                }
                            }
                        }
                        if (t) {
                            printf(" }");
                        }
                    }
                        
                    /* Move on. */
                        
                    if (a < num_aliases - 1) {
                        putchar(a || !canon ? ' ' : '\t');
                    }
                }
            }
            if (canon) {
                putchar('\n');
            } else if (i < num - 1) {
                putchar(' ');
            } 
                
#endif
        }

    return 0;
}

// Convert a file from one encoding to another
static UBool convertFile(const char* fromcpage, 
                 const char* tocpage, 
                 FILE* infile, 
                 FILE* outfile)
{
  UBool ret = TRUE;
    UConverter* convfrom = 0;
    UConverter* convto = 0;
    UErrorCode err = U_ZERO_ERROR;
    UBool  flush;
    const char* cbuffiter;
    char* buffiter;
    const size_t readsize = buffsize-1;
    char* buff = 0;

    const UChar* cuniiter;
    UChar* uniiter;
    UChar* unibuff = 0;

    size_t rd, totbuffsize;

#if USE_TRANSLIT
    const char *translit;

    Transliterator *t = NULL;

    translit = getenv("TRANSLIT");
    if(translit != NULL && *translit)
      {
        t = Transliterator::createInstance(UnicodeString(translit, ""));
        fprintf(stderr, "Opening transliterator: %s\n", translit, t);
      }
#endif

    // Create codepage converter. If the codepage or its aliases weren't
    // available, it returns NULL and a failure code
    convfrom = ucnv_open(fromcpage, &err);
    if (U_FAILURE(err))
    {
      UnicodeString str(fromcpage,"");
      u_wmsg("cantOpenFromCodeset",str.getBuffer(),
             u_wmsg_errorName(err));
      goto error_exit;
    }

    convto = ucnv_open(tocpage, &err);

    if (U_FAILURE(err))
    {
      UnicodeString str(tocpage,"");
      u_wmsg("cantOpenToCodeset",str.getBuffer(),
             u_wmsg_errorName(err));
      goto error_exit;
    }

    // To ensure that the buffer always is of enough size, we
    // must take the worst case scenario, that is the character in the codepage
    // that uses the most bytes and multiply it against the buffsize
    totbuffsize = buffsize * ucnv_getMaxCharSize(convto);
    buff = new char[totbuffsize];
    unibuff = new UChar[buffsize];
        
    do  
    {
        rd = fread(buff, 1, readsize, infile);
        if (ferror(infile) != 0)
        {
            UnicodeString str(strerror(errno), "");
            u_wmsg("cantRead",str.getBuffer());
            goto error_exit;
        }
            
        // Convert the read buffer into the new coding
        // After the call 'uniiter' will be placed on the last character that was converted
        // in the 'unibuff'. 
        // Also the 'cbuffiter' is positioned on the last converted character.
        // At the last conversion in the file, flush should be set to true so that
        // we get all characters converted
        //
        // The converter must be flushed at the end of conversion so that characters
        // on hold also will be written
        uniiter = unibuff;
        cbuffiter = buff;
        flush = rd!=readsize;
        ucnv_toUnicode(convfrom, &uniiter, uniiter + buffsize, &cbuffiter, cbuffiter + rd, 0, flush, &err);
            
        if (U_FAILURE(err))
        {
            u_wmsg("problemCvtToU", u_wmsg_errorName(err));
            goto error_exit;
        }
            
        // At the last conversion, the converted characters should be equal to number
        // of chars read.
        if (flush && cbuffiter!=(buff+rd))
        {
            u_wmsg("premEndInput");
            goto error_exit;
        }
            
        // Convert the Unicode buffer into the destination codepage
        // Again 'buffiter' will be placed on the last converted character
        // And 'cuniiter' will be placed on the last converted unicode character
        // At the last conversion flush should be set to true to ensure that 
        // all characters left get converted

        UnicodeString u(unibuff, uniiter-unibuff);
        buffiter = buff;
        cuniiter = unibuff;

#ifdef USE_TRANSLIT
        if(t) 
          {
            t->transliterate(u);
            u.extract(0, u.length(), unibuff, 0);
            uniiter = unibuff + u.length();
            
          }
#endif

        ucnv_fromUnicode(convto, &buffiter, buffiter + totbuffsize, &cuniiter, cuniiter + (size_t) (uniiter - unibuff), 0, flush, &err);
            
        if (U_FAILURE(err))
        {
           u_wmsg("problemCvtFromU", u_wmsg_errorName(err));
           goto error_exit;
        }
                        
        // At the last conversion, the converted characters should be equal to number
        // of consumed characters.
        if (flush && cuniiter!=(unibuff+(size_t)(uniiter-unibuff)))
        {
          u_wmsg("premEnd");
          goto error_exit;
        }
            
        // Finally, write the converted buffer to the output file
        rd =  (size_t)(buffiter-buff);
        if (fwrite(buff, 1, rd, outfile) != rd)
        {
          UnicodeString str(strerror(errno),"");
          u_wmsg("cantWrite", str.getBuffer());
            goto error_exit;
        }
        
    } while (!flush); // Stop when we have flushed the converters (this means that it's the end of output)

    goto normal_exit;
  error_exit:
    ret = TRUE;
  normal_exit:
    if (convfrom) ucnv_close(convfrom);
    if (convto) ucnv_close(convto);

#ifdef USE_TRANSLIT
    if ( t ) delete t;
#endif

    // Close the created converters
    if (buff) delete [] buff;
    if (unibuff) delete [] unibuff;
    return ret;
}

static void usage(const char *pname, int ecode)
{
  const UChar *msg;
  int32_t      msgLen;
  UErrorCode  err = U_ZERO_ERROR;
   
  initMsg(pname);
  msg = ures_getStringByKey(gBundle, ecode ? "lcUsageWord" : "ucUsageWord", &msgLen, &err);
  UnicodeString upname(pname);
  UnicodeString mname(msg, msgLen);

  u_wmsg("usage", mname.getBuffer(), upname.getBuffer());
  if (!ecode) {
    putchar('\n');
    u_wmsg("help");
  }

  exit(ecode);
}

int main(int argc, char** argv)
{
    FILE* file = 0;
    FILE* infile;
    int   ret = 0;
    const char* fromcpage = 0;
    const char* tocpage = 0;
    const char* infilestr = 0;

    char** iter = argv+1;
    char** end = argv+argc;    

    const char *pname = *argv;

    int printConvs = 0, printCanon = 0;
    const char *printName = 0;

    // First, get the arguments from command-line
    // to know the codepages to convert between
    for (; iter!=end; iter++)
    {
        // Check for from charset
        if (strcmp("-f", *iter) == 0 || !strcmp("--from-code", *iter))
        {
            iter++;
            if (iter!=end)
                fromcpage = *iter;
        }
        else if (strcmp("-t", *iter) == 0 || !strcmp("--to-code", *iter))
        {
            iter++;
            if (iter!=end)
                tocpage = *iter;
        }
        else if (strcmp("-l", *iter) == 0 || !strcmp("--list", *iter))
        {
            printConvs = 1;
        }
        else if (strcmp("--default-code", *iter) == 0)
        {
            printName = ucnv_getDefaultName();
        }
        else if (strcmp("--list-code", *iter) == 0) {
            iter++;
            if (iter!=end) {
                UErrorCode e = U_ZERO_ERROR;
                printName = ucnv_getAlias(*iter, 0, &e);
                if (U_FAILURE(e)) {
                    UnicodeString str(*iter);
                    initMsg(pname);
                    u_wmsg("noSuchCodeset", str.getBuffer());
                    return 2;
                }
            }
            else usage(pname, 1);
        }
        else if (strcmp("--canon", *iter) == 0) {
            printCanon = 1;
        }
        else if (strcmp("-h", *iter) == 0 || !strcmp("-?", *iter)|| !strcmp("--help", *iter))
        {
            usage(pname, 0);
        }
        else if (**iter == '-' && (*iter)[1]) {
            usage(pname, 1);
        } else if (!infilestr) {
            infilestr = *iter;
        } else {
            usage(pname, 1);
        }
    }

    if (printConvs || printName) {
        return printConverters(pname, printName, printCanon) ? 2 : 0;
    }

    if (fromcpage==0 && tocpage==0)
    {
        usage(pname, 1);
    }

    if (fromcpage==0)
    {
      initMsg(pname);
      u_wmsg("noFromCodeset");
      //"No conversion from codeset given (use -f)\n");
        goto error_exit;
    }
    if (tocpage==0)
    {
      initMsg(pname);
      u_wmsg("noToCodeset");
      // "No conversion to codeset given (use -t)\n");
      goto error_exit;
    }

    // Open the correct input file or connect to stdin for reading input
    if (infilestr!=0 && strcmp(infilestr, "-"))
    {
        file = fopen(infilestr, "rb");
        if (file==0)
        {
          UnicodeString str1(infilestr,"");
          UnicodeString str2(strerror(errno),"");
          initMsg(pname);
          u_wmsg("cantOpenInputF", 
                 str1.getBuffer(),
                 str2.getBuffer());
          return 1;
        }
        infile = file;
    }
    else {
        infile = stdin;
#ifdef WIN32
        if( setmode( fileno ( stdin ), O_BINARY ) == -1 ) {
                perror ( "Cannot set stdin to binary mode" );
                exit(-1);
        }
#endif
    }
#ifdef WIN32
  if( setmode( fileno ( stdout ), O_BINARY ) == -1 ) {
          perror ( "Cannot set stdout to binary mode" );
          exit(-1);
  }
#endif
    if (!convertFile(fromcpage, tocpage, infile, stdout))
        goto error_exit;

    goto normal_exit;
  error_exit:
    ret = 1;
  normal_exit:

    if (file!=0)
        fclose(file);
    return ret;
}


/*
 * Hey, Emacs, please set the following:
 *
 * Local Variables:
 * indent-tabs-mode: nil
 * End:
 *
 */
