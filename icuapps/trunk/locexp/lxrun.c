/**********************************************************************
*   Copyright (C) 1999-2002, International Business Machines
*   Corporation and others.  All Rights Reserved.
***********************************************************************/
#include "locexp.h"
#include <unicode/udata.h>

/************************ fcns *************************/
/** Called functions from the main() module **/
LXContext *theContext;

void initContext ( LXContext *ctx )
{
    /* INIT THE LX */
    memset(ctx, 0, sizeof(*ctx));
    strcpy(ctx->cLocale, "en");
    ctx->defaultRB = 0;
    ctx->ourCharsetName = "utf-8";
    ctx->locales = NULL;
    ctx->curLocale = NULL;
    ctx->parLocale = NULL;
    ctx -> numLocales = 0;
    /* END INIT LX */

    ctx->OUT = NULL;
    ctx->fOUT = NULL;
}

void initLX()
{
    /* set the path for FSWF */
    char newPath[500];
    UErrorCode myError = U_ZERO_ERROR;

#ifdef LX_STATIC
    /* try static data first .. then fall back to individual files */
    udata_setAppData( "locexp", (const void*) locexp_dat, &myError);
    if(U_SUCCESS(myError))
    {
        FSWF_setBundlePath("locexp");
        return;
    }
#endif

    strcpy(newPath, u_getDataDirectory());
    strcat(newPath, "locexp");
    FSWF_setBundlePath(newPath);
}

void closeLX(LXContext *theContext)
{
    FSWF_close();
    if(theContext != NULL)
    {
        destroyLocaleTree(theContext->locales);
        theContext->locales = NULL;
    }
}

/* Initialize the environment */
void setupLocaleExplorer(LXContext *lx)
{
    const char  *fileObj = NULL;
    UErrorCode status = U_ZERO_ERROR;
    char *tmp;

#ifdef WIN32
    if( setmode( fileno ( stdout ), O_BINARY ) == -1 ) {
        perror ( "Cannot set stdout to binary mode" );
        exit(-1);
    }
#endif


    /* init ...... */
/*
  uloc_setDefault("sr_NZ_EURO", &status);
*/ /* BASELINE. Don't use a real locale here - will mess up the fallback error codes [for now] */


    uloc_setDefault("en_US_CALIFORNIA", &status);


#ifdef  WIN32
/*  u_setDataDirectory("c:\\o\\icu\\source\\data\\");
 */ /* ONLY IF you need to force the path .... */
#endif


#if 0
   /** Below is useful for debugging. */
    fprintf(stderr, "PID=%d\n", getpid());  
    system("sleep 20");   
#endif

    status = U_ZERO_ERROR; 

    /* Set up some initial values, just in case something goes wrong later. */
    strcpy(lx->chosenEncoding, "utf-8");
    lx->ourCharsetName = "utf-8";

    lx->OUT = setLocaleAndEncodingAndOpenUFILE(lx, lx->chosenEncoding, &lx->setLocale, &lx->setEncoding, &fileObj);

    if(fileObj != NULL)
    {
        writeFileObject( lx, fileObj );
        lx->OUT = NULL; /* nothing to write */
        return;
    }


    if(!lx->OUT)
        doFatal(lx, "u_finit trying to open file", 0);
  
    lx->ourCharsetName = MIMECharsetName(lx->chosenEncoding); /* for some sanity */

    /** Setup the callbacks **/
    /* put our special error handler in */

    if(u_fgetConverter(lx->OUT) == NULL)
    {
        fprintf(stdout,"content-type: text/plain\r\n");
        fprintf(stdout,"\r\nFatal: can't open ICU Data (%s)\r\n", u_getDataDirectory());
        fprintf(stdout,"ICU_DATA=%s\n", getenv("ICU_DATA"));
        exit(0);

        lx->couldNotOpenEncoding = lx->chosenEncoding; /* uhoh */
    }
    else
    {
        /* important - clear out the ctx */
        memset(&lx->backslashCtx, sizeof(lx->backslashCtx), 0);
        ucnv_setFromUCallBack((UConverter*)u_fgetConverter(lx->OUT), 
                              UCNV_FROM_U_CALLBACK_BACKSLASH,
                              &lx->backslashCtx,
                              &lx->backslashCtx.subCallback,
                              &lx->backslashCtx.subContext,
                              &status);
    
    
        lx->backslashCtx.html = TRUE;
    
        memset(&lx->decomposeCtx, sizeof(lx->decomposeCtx), 0);
        ucnv_setFromUCallBack((UConverter*)u_fgetConverter(lx->OUT), 
                              UCNV_FROM_U_CALLBACK_DECOMPOSE,
                              &lx->decomposeCtx,
                              &lx->decomposeCtx.subCallback,
                              &lx->decomposeCtx.subContext,
                              &status);
    
        /* To do: install more cb's later. */
      
#if 0
        if(!strcmp(lx->chosenEncoding, "transliterated"))
        {
            memset(&lx->xlitCtx, sizeof(lx->xlitCtx), 0);
            lx->xlitCtx.html = FALSE;
            ucnv_setFromUCallBack((UConverter*)u_fgetConverter(lx->OUT), 
                                  UCNV_FROM_U_CALLBACK_TRANSLITERATED,
                                  &lx->xlitCtx,
                                  &lx->xlitCtx.subCallback,
                                  &lx->xlitCtx.subContext,
                                  &status);
        }

#endif

#if 0
        else
        
        {
            ucnv_setFromUCallBack((UConverter*)u_fgetConverter(lx->OUT), &UCNV_FROM_U_CALLBACK_COLLECT, &status);
            COLLECT_lastResortCallback =  UCNV_FROM_U_CALLBACK_DECOMPOSE;
        }
    

#ifdef LX_USE_NAMED
        /* overrides */
        ucnv_setFromUCallBack((UConverter*)u_fgetConverter(lx->OUT), &UCNV_FROM_U_CALLBACK_NAMED, &status);
#endif
    
#endif
    } /* end: if have a converter */

    /* setup the time zone.. */
    if (tmp && !strncmp(tmp,"SETTZ=",6))
    {
        const char *start = (tmp+6);
        const char *end;

        lx->newZone[0] = 0;

        end = strchr(tmp, '&');

        if(!end)
        {
            end = (start+strlen(start));
        }
        unescapeAndDecodeQueryField(lx->newZone,256,start);

        if(u_strlen(lx->newZone))
        {
            char junk[200];
            u_austrcpy(junk, lx->newZone);
            fprintf(lx->fOUT,"Set-Cookie: TZ=%s;path=/;\r\n", junk);
        }
    }


    if(lx->newZone[0] == 0x0000) /* if still no zone.. */
    {
        const char *cook;
        cook = getenv("HTTP_COOKIE");

        if((cook)&&(cook=strstr(cook,"TZ=")))
        {
            cook += 3;
            u_uastrcpy(lx->newZone,cook);
        }
    }
  
    /* Print the encoding and last HTTP header... */

    fprintf(lx->fOUT, "Content-Type: text/html;charset=%s\r\n\r\n", lx->ourCharsetName);
    fflush(lx->fOUT);


    /* 
       kore wa nandesuka?
       xi trid?

     
       {
       char langBuf[100];
       uloc_getLanguage(NULL,langBuf,100,&status);
       printf("Content-Language: %s\r\n", langBuf);
       }*/


    /* parse & sort the list of locales */
    setupLocaleTree(lx);
    /* Open an RB in the default locale */
    lx->defaultRB = ures_open(NULL, lx->cLocale, &status);

    if(!strcmp(lx->chosenEncoding, "transliterated"))
    {
        char id[200];
        UErrorCode transStatus = U_ZERO_ERROR;
        UTransliterator *trans;
        sprintf(id,"Any-%s", /* lx->curLocaleName,*/ lx->cLocale);
        if(!strcmp(lx->cLocale, "xol")) {
            sprintf(id, "Latn-Ital");
        }
        /* fprintf(stderr, "LC=[%s]\n", id);  */
        trans = utrans_open(id, UTRANS_FORWARD, NULL, -1, NULL, &transStatus);
        if(U_FAILURE(transStatus))	
        {
            fprintf(stderr,"Failed to open - %s\n", u_errorName(transStatus));
            /* blah blah balh*/
        }
        else
        {
            lx_setHTMLFilterOnTransliterator(trans, TRUE);
            u_fflush(lx->OUT);
            trans = u_fsettransliterator(lx->OUT, U_WRITE, trans, &transStatus);
            if(trans != NULL)
            {
                utrans_close(trans);
            }
        }
    }

/*  u_uastrcpy(lx->newZone, "Europe/Malta"); */
    u_uastrcpy(lx->newZone, "PST"); /* for now */


    if(lx->newZone[0] != 0x0000)
    {
        UTimeZone *tz;

        lx->timeZone = lx->newZone;

#ifndef LX_NO_USE_UTIMZONE
        tz = utz_open(lx->newZone); /* returns NULL for nonexistent TZ!! */
#else
        tz = NULL;
#endif

        if(tz)
        {
            utz_setDefault(tz);
            utz_close(tz);
        }
    }
    else
    {
        lx->timeZone = NULL;
    }

    fflush(lx->fOUT); /* and that, as they say, is that.  All UFILE from here.. */
}

/* Initialize and then run */
void runLocaleExplorer(LXContext *lx)
{
    setupLocaleExplorer(lx);
    if(lx->OUT) {
    	displayLocaleExplorer(lx);
    }
}

UResourceBundle *getCurrentBundle(LXContext *lx, UErrorCode *status) 
{
  if(U_FAILURE(*status)) {
    return NULL;
  }

  if(lx->curRB) {
    return(lx->curRB); 
  }

  if(!*(lx->curLocaleName)) { /* illegal arg */ return NULL; }
  lx->curRB = ures_open(NULL, lx->curLocaleName, status);

  return lx->curRB;
}

UResourceBundle *getDisplayBundle(LXContext *lx, UErrorCode *status) 
{
  if(U_FAILURE(*status)) {
    return NULL;
  }

  if(lx->defaultRB) {
    return(lx->defaultRB); 
  }

  if(!*(lx->cLocale)) { /* illegal arg */ return NULL; }
  lx->defaultRB = ures_open(NULL, lx->cLocale, status);
  return lx->defaultRB;
}

