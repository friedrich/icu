/*
*******************************************************************************
*
*   Copyright (C) 2000, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*
* File wrtjava.c
*
* Modification History:
*
*   Date        Name        Description
*   01/11/02    Ram        Creation.
*******************************************************************************
*/

#include <assert.h>
#include "reslist.h"
#include "unewdata.h"
#include "unicode/ures.h"
#include "errmsg.h"
#include "filestrm.h"
#include "cstring.h"
#include "unicode/ucnv.h"
#include "genrb.h"
#include "rle.h"
#include "ucol_tok.h"
#include "uhash.h"
#include "uresimp.h"
#include "unicode/ustring.h"

void res_write_java(struct SResource *res,UErrorCode *status);


static const char copyRight[] = 
    "/* \n"
    " *******************************************************************************\n"
    " *\n"
    " *   Copyright (C) International Business Machines\n"
    " *   Corporation and others.  All Rights Reserved.\n"
    " *\n"
    " *******************************************************************************\n"
    " * $" "Source:  $ \n"
    " * $" "Date:  $ \n"
    " * $" "Revision:  $ \n"
    " *******************************************************************************\n"
    " */\n\n";
static const char warningMsg[] = 
    "/*********************************************************************\n"
    "######################################################################\n"
    "\n"
    "   WARNING: This file is generated by genrb Version " GENRB_VERSION ".\n"
    "            If you edit this file, please make sure that, the source\n"
    "            of this file (XXXX.txt in LocaleElements_XXXX.java)\n"
    "            is also edited.\n"
    "######################################################################\n"
    " *********************************************************************\n"
    " */\n\n";
static const char* openBrace="{\n";
static const char* closeBrace="}\n";
static const char* closeClass="    };\n"
                              "}\n";

static const char* javaClass =  "import java.util.ListResourceBundle;\n"
                                "import com.ibm.icu.impl.ICUListResourceBundle;\n\n"
                                "public class ";
 
static const char* javaClass1=  " extends ICUListResourceBundle {\n\n"
                                "    /**\n"
                                "     * Overrides ListResourceBundle \n"
                                "     */\n"
                                "    public final Object[][] getContents() { \n"
                                "          return  contents;\n"
                                "    }\n"
                                "    private static Object[][] contents = {\n";
static const char* javaClassICU= " extends ICUListResourceBundle {\n\n"
                                 "    public %s  () {\n"
                                 "          super.contents = data;\n"
                                 "    }\n"
                                 "    static final Object[][] data = new Object[][] { \n";
static int tabCount = 3;

static FileStream* out=NULL;
static struct SRBRoot* srBundle ;
static const char* outDir = NULL;

static const char* bName=NULL;
static const char* pName=NULL;

static void write_tabs(FileStream* os){
    int i=0;
    for(;i<=tabCount;i++){
        T_FileStream_write(os,"    ",4);
    }
}
static const char* enc ="";
static UConverter* conv = NULL;

#define MAX_DIGITS 10
static int32_t 
itostr(char * buffer, int32_t i, uint32_t radix, int32_t pad)
{
    int32_t length = 0;
    int32_t num = 0;
    int32_t save = i;
    int digit;
    int32_t j;
    char temp;
    
    /* if i is negative make it positive */
    if(i<0){
        i=-i;
    }
    
    do{
        digit = (int)(i % radix);
        buffer[length++]=(char)(digit<=9?(0x0030+digit):(0x0030+digit+7));
        i=i/radix;
    } while(i);

    while (length < pad){
        buffer[length++] = 0x0030;/*zero padding */
    }
    
    /* if i is negative add the negative sign */
    if(save < 0){
        buffer[length++]='-';
    }

    /* null terminate the buffer */
    if(length<MAX_DIGITS){
        buffer[length] =  0x0000;
    }

    num= (pad>=length) ? pad :length;
 

    /* Reverses the string */
    for (j = 0; j < (num / 2); j++){
        temp = buffer[(length-1) - j];
        buffer[(length-1) - j] = buffer[j];
        buffer[j] = temp;
    }
    return length;
}



static int32_t 
uCharsToChars( char* target,int32_t targetLen, UChar* source, int32_t sourceLen,UErrorCode* status){
    int i=0, j=0;
    char str[30]={'\0'};
    while(i<sourceLen){
        if (source[i] == '\n') {
            if (j + 2 < targetLen) {
                uprv_strcat(target, "\\n");
            }
            j += 2;
        }else if(source[i]==0x0D){
            if(j+2<targetLen){
                uprv_strcat(target,"\\f");
            }
            j+=2;
        }else if(source[i] == '"'){
            if(source[i-1]=='\''){
                if(j+2<targetLen){
                    uprv_strcat(target,"\\");
                    target[j+1]= (char)source[i];
                }
                j+=2;
            }else if(source[i-1]!='\\'){
                     
                if(j+2<targetLen){
                    uprv_strcat(target,"\\");
                    target[j+1]= (char)source[i];
                }
                j+=2;
            }else if(source[i-1]=='\\'){
                target[j++]= (char)source[i];
            }
        }else if(source[i]=='\\'){
            if(i+1<sourceLen){
                switch(source[i+1]){
                case ',':
                case '!':
                case '?':
                case '#':
                case '.':
                case '%':
                case '&':
                case ':':
                case ';':
                    if(j+2<targetLen){
                       uprv_strcat(target,"\\\\");
                    }
                    j+=2;
                    break;
                case '"':
                case '\'':
                    if(j+3<targetLen){
                       uprv_strcat(target,"\\\\\\");
                    }
                    j+=3;
                    break;
                default :
                    if(j<targetLen){
                        target[j]=(char)source[i];
                    }
                    j++;
                    break;
                }
            }else{
                if(j<targetLen){
                    uprv_strcat(target,"\\\\");
                }
                j+=2;
            }
        }else if(source[i]>=0x20 && source[i]<0x7F/*ASCII*/){
            if(j<targetLen){
                target[j] = (char) source[i];
            }
            j++;            
        }else{
            if(*enc =='\0' || source[i]==0x0000){
                uprv_strcpy(str,"\\u");
                itostr(str+2,source[i],16,4);
                if(j+6<targetLen){
                    uprv_strcat(target,str);
                }
                j+=6;
            }else{
                char dest[30] = {0};
                int retVal=ucnv_fromUChars(conv,dest,30,source+i,1,status);
                if(U_FAILURE(*status)){
                    return 0;
                }
                if(j+retVal<targetLen){
                    uprv_strcat(target,dest);
                }
                j+=retVal;
            }
        }
        i++;
    }
    return j;
}


static uint32_t 
strrch(const char* source,uint32_t sourceLen,char find){
    const char* tSourceEnd =source + (sourceLen-1);
    while(tSourceEnd>= source){
        if(*tSourceEnd==find){
            return (uint32_t)(tSourceEnd-source);
        }
        tSourceEnd--;
    }
    return (uint32_t)(tSourceEnd-source);
}

static void
str_write_java( uint16_t* src, int32_t srcLen, UBool printEndLine, UErrorCode *status){

    uint32_t length = srcLen*8;
    uint32_t bufLen = 0;
    char* buf = (char*) malloc(sizeof(char)*length);

    /* test for NULL */
    if(buf == NULL) {
        *status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    
    memset(buf,0,length);
  
    bufLen = uCharsToChars(buf,length,src,srcLen,status);

    if(printEndLine) write_tabs(out);

    if(U_FAILURE(*status)){
        return;
    }
    
    if(bufLen+(tabCount*4) > 70  ){
        uint32_t len = 0;
        char* current = buf;
        uint32_t add;
        while(len < bufLen){
            add = 70-(tabCount*4)-5/* for ", +\n */;
            current = buf +len;
            if (add < (bufLen-len)) {
                uint32_t index = strrch(current,add,'\\');
                if (index > add) {
                    index = add;
                } else {
                    int32_t num =index-1;
                    uint32_t seqLen;
                    while(num>0){
                        if(current[num]=='\\'){
                            num--;
                        }else{
                            break;
                        }
                    }
                    if ((index-num)%2==0) {
                        index--;
                    }
                    seqLen = (current[index+1]=='u') ? 6 : 2;
                    if ((add-index) < seqLen) {
                        add = index + seqLen;
                    }
                }
            }
            T_FileStream_write(out,"\"",1);
            if(len+add<bufLen){
                T_FileStream_write(out,current,add);
                T_FileStream_write(out,"\" +\n",4);
                write_tabs(out);
            }else{
                T_FileStream_write(out,current,bufLen-len);
            }
            len+=add;
        }
    }else{
        T_FileStream_write(out,"\"",1);
        T_FileStream_write(out, buf,bufLen);
    }
    if(printEndLine){
        T_FileStream_write(out,"\",\n",3);
    }else{
        T_FileStream_write(out,"\"",1);
    }
}

/* Writing Functions */
static void 
string_write_java(struct SResource *res,UErrorCode *status) {       
    if(uprv_strcmp(srBundle->fKeys+res->fKey,"%%UCARULES")==0 ){
        char fileName[1024] ={0};
        const char* file = "UCARules.utf8";
        FileStream* datFile = NULL;
        const char* type = "new ICUListResourceBundle.ResourceString(";
        char* dest  = (char*) uprv_malloc( 8 * res->u.fString.fLength);
        int32_t len = 0;
        const UChar* src = res->u.fString.fChars;
        uprv_strcat(fileName,outDir);
        if(outDir[uprv_strlen(outDir)-1]!=U_FILE_SEP_CHAR){
            uprv_strcat(fileName,U_FILE_SEP_STRING);
        }
        uprv_strcat(fileName,file);/* UCARULES.utf8 UTF-8 file */
        
        write_tabs(out);

        T_FileStream_write(out, type, (int32_t)uprv_strlen(type));
        T_FileStream_write(out, "\"", 1);
        T_FileStream_write(out, file, (int32_t)uprv_strlen(file));
        T_FileStream_write(out, "\")\n", 3);
        datFile=T_FileStream_open(fileName,"w");
        
        if(!dest){
            *status=U_MEMORY_ALLOCATION_ERROR;
        }
        
        u_strToUTF8(dest,8*res->u.fString.fLength,&len,res->u.fString.fChars,res->u.fString.fLength,status);
        if(U_FAILURE(*status)){
            T_FileStream_close(datFile);
            uprv_free(dest);
            return;
        }
        T_FileStream_write(datFile,dest,len);
        T_FileStream_close(datFile);
        uprv_free(dest);

    }else{
        str_write_java(res->u.fString.fChars,res->u.fString.fLength,TRUE,status);

        if(uprv_strcmp(srBundle->fKeys+res->fKey,"Rule")==0){
            UChar* buf = (UChar*) uprv_malloc(sizeof(UChar)*res->u.fString.fLength);
            uprv_memcpy(buf,res->u.fString.fChars,res->u.fString.fLength);      
            uprv_free(buf);
        }
    }

}

static void 
alias_write_java(struct SResource *res,UErrorCode *status) {
    char* str = "new ICUListResourceBundle.Alias(";
    write_tabs(out);
    T_FileStream_write(out,str,uprv_strlen(str));
    
    /*str_write_java(res->u.fString.fChars,res->u.fString.fLength,FALSE,status);*/
    /*if(*res->u.fString.fChars == RES_PATH_SEPARATOR) {
        /* there is a path included 
        locale = u_strchr(res->u.fString.fChars +1, RES_PATH_SEPARATOR);
        *locale = 0;
        locale++;
        
        T_FileStream_write(out,"\"/",2);
        T_FileStream_write(out,apName,(int32_t)uprv_strlen(apName));
        T_FileStream_write(out,"/",1);
        T_FileStream_write(out,abName,(int32_t)uprv_strlen(abName));
        T_FileStream_write(out,"/\"+",3);
        str_write_java(locale,res->u.fString.fLength-(locale-res->u.fString.fChars),FALSE,status);
    } else {
        str_write_java(res->u.fString.fChars,res->u.fString.fLength,FALSE,status);
    }*/
    
    str_write_java(res->u.fString.fChars,res->u.fString.fLength,FALSE,status);
    
    T_FileStream_write(out,"),\n",3);
}

static void 
array_write_java( struct SResource *res, UErrorCode *status) {

    uint32_t  i         = 0;
    const char* arr ="new String[] { \n";
    struct SResource *current = NULL;
    struct SResource *first =NULL;
    UBool decrementTabs = FALSE;
    UBool allStrings    = TRUE;

    if (U_FAILURE(*status)) {
        return;
    }

    if (res->u.fArray.fCount > 0) {

        current = res->u.fArray.fFirst;
        i = 0;
        while(current != NULL){
            if(current->fType!=RES_STRING){
                allStrings = FALSE;
                break;
            }
            current= current->fNext;
        }

        current = res->u.fArray.fFirst;
        if(allStrings==FALSE){
            const char* object = "new Object[]{\n";
            write_tabs(out);
            T_FileStream_write(out, object, (int32_t)uprv_strlen(object));
            tabCount++;
            decrementTabs = TRUE;
        }else{
            write_tabs(out);
            T_FileStream_write(out, arr, (int32_t)uprv_strlen(arr));
            tabCount++;
        }
        first=current;
        while (current != NULL) {
            /*if(current->fType==RES_STRING){
                write_tabs(out);
            }*/
            res_write_java(current, status);
            if(U_FAILURE(*status)){
                return;
            }
            i++;
            current = current->fNext;
        }
        T_FileStream_write(out,"\n",1);

        tabCount--;
        write_tabs(out);
        T_FileStream_write(out,"},\n",3);

    } else {
        write_tabs(out);
        T_FileStream_write(out,arr,uprv_strlen(arr));
        write_tabs(out);
        T_FileStream_write(out,"},\n",3);
    }
}

static void 
intvector_write_java( struct SResource *res, UErrorCode *status) {
    uint32_t i = 0;
    const char* intArr = "new Integer[] {\n";
    const char* intC   = "new Integer(";
    const char* stringArr = "new String[]{\n"; 
    char buf[100];
    int len =0;
    buf[0]=0;
    write_tabs(out);

    if(uprv_strcmp(srBundle->fKeys+res->fKey,"DateTimeElements")==0){
        T_FileStream_write(out, stringArr, (int32_t)uprv_strlen(stringArr));
        tabCount++;
        for(i = 0; i<res->u.fIntVector.fCount; i++) {
            write_tabs(out);
            len=itostr(buf,res->u.fIntVector.fArray[i],10,0);
            T_FileStream_write(out,"\"",1);
            T_FileStream_write(out,buf,len);
            T_FileStream_write(out,"\",",2);
            T_FileStream_write(out,"\n",1);
        }
    }else{
        T_FileStream_write(out, intArr, (int32_t)uprv_strlen(intArr));
        tabCount++;
        for(i = 0; i<res->u.fIntVector.fCount; i++) {
            write_tabs(out);
            T_FileStream_write(out, intC, (int32_t)uprv_strlen(intC));
            len=itostr(buf,res->u.fIntVector.fArray[i],10,0);
            T_FileStream_write(out,buf,len);
            T_FileStream_write(out,"),",2);
            T_FileStream_write(out,"\n",1);
        }
    }
    tabCount--;
    write_tabs(out);
    T_FileStream_write(out,"},\n",3);
}

static void 
int_write_java(struct SResource *res,UErrorCode *status) {
    const char* intC   =  "new Integer(";
    char buf[100];
    int len =0;
    buf[0]=0;

    /* write the binary data */
    write_tabs(out);
    T_FileStream_write(out, intC, (int32_t)uprv_strlen(intC));
    len=itostr(buf, res->u.fIntValue.fValue, 10, 0);
    T_FileStream_write(out,buf,len);
    T_FileStream_write(out,"),\n",3 );

}

static void 
bin_write_java( struct SResource *res, UErrorCode *status) {
    const char* type = "new ICUListResourceBundle.CompressedBinary(";
    const char* ext;
    int32_t srcLen=res->u.fBinaryValue.fLength;

    if(srcLen>0 ){
        uint16_t* target=NULL;
        uint16_t* saveTarget = NULL;
        int32_t tgtLen = 0;
        char* dest = NULL;

        if(uprv_strcmp(srBundle->fKeys+res->fKey,"%%CollationBin")==0 || uprv_strcmp(srBundle->fKeys+res->fKey,"BreakDictionaryData")==0){
            char fileName[1024] ={0};
            char fn[1024] =  {0};
            FileStream* datFile = NULL;
            if(uprv_strcmp(srBundle->fKeys+res->fKey,"BreakDictionaryData")==0){
                uprv_strcat(fileName,"BreakDictionaryData");
                ext = ".brk";
            }else{
                uprv_strcat(fileName,"CollationElements");
                ext=".col";
            }
            if(uprv_strcmp(srBundle->fLocale,"root")!=0){
                uprv_strcat(fileName,"_");
                uprv_strcat(fileName,srBundle->fLocale);
            }
            
            uprv_strcat(fileName,ext);

            uprv_strcat(fn,outDir);
            if(outDir[uprv_strlen(outDir)-1]!=U_FILE_SEP_CHAR){
                uprv_strcat(fn,U_FILE_SEP_STRING);
            }
            uprv_strcat(fn,fileName);
            type = "new ICUListResourceBundle.ResourceBinary(";
            write_tabs(out);
            T_FileStream_write(out, type, (int32_t)uprv_strlen(type));
            T_FileStream_write(out, "\"", 1);
            T_FileStream_write(out, fileName, (int32_t)uprv_strlen(fileName));
            T_FileStream_write(out, "\"),\n", 4);

            datFile=T_FileStream_open(fn,"w");
            T_FileStream_write(datFile, res->u.fBinaryValue.fData, res->u.fBinaryValue.fLength);
            T_FileStream_close(datFile);

        }else{
            if(uprv_strcmp(srBundle->fKeys+res->fKey, "%%")==0){
                srcLen = res->u.fBinaryValue.fLength/2;
                target = (uint16_t*)malloc(sizeof(uint16_t) * srcLen);
                if(target){
                    saveTarget  = target;
                    type = "new ICUListResourceBundle.CompressedString(\n";
                    tgtLen = usArrayToRLEString((uint16_t*)res->u.fBinaryValue.fData,
                                                 srcLen,target, srcLen, status);
                    if(U_FAILURE(*status)){
                         printf("Could not encode got error : %s \n", u_errorName(*status));
                         return;
                    }
#if DEBUG
                    {
                        /***************** Test Roundtripping *********************/
                        int32_t myTargetLen = rleStringToUCharArray(target,tgtLen,NULL,0,status);
                        uint16_t* myTarget = (uint16_t*) malloc(sizeof(uint16_t) * myTargetLen);

                        /* test for NULL */
                        if(myTarget == NULL) {
                            *status = U_MEMORY_ALLOCATION_ERROR;
                            return;    
                        }
                        
                        int i=0;
                        int32_t retVal=0;
                        uint16_t* saveSrc = (uint16_t*)res->u.fBinaryValue.fData;
                        *status = U_ZERO_ERROR;
                        retVal=rleStringToUCharArray(target,tgtLen,myTarget,myTargetLen,status);
                        if(U_SUCCESS(*status)){

                            for(i=0; i< srcLen;i++){
                                if(saveSrc[i]!= myTarget[i]){
                                    printf("the encoded string cannot be decoded Expected : 0x%04X Got : %: 0x%04X at %i\n",res->u.fBinaryValue.fData[i],myTarget[i], i);
                                }
                            }
                        }else{
                            printf("Could not decode got error : %s \n", u_errorName(*status));
                        }
                        free(myTarget);
                     }
#endif

                }else{
                    *status= U_MEMORY_ALLOCATION_ERROR;
                    return;
                }
            }else{
                srcLen = res->u.fBinaryValue.fLength;
                target = (uint16_t*)malloc(sizeof(uint16_t) * srcLen);
                saveTarget  = target;
                if(target){
                    tgtLen = byteArrayToRLEString(res->u.fBinaryValue.fData,
                                                  srcLen,target, srcLen,status);
                    if(U_FAILURE(*status)){
                         printf("Could not encode got error : %s \n", u_errorName(*status));
                         return;
                    }
#if DEBUG
                    /***************** Test Roundtripping *********************/
                    {
                        int32_t myTargetLen = rleStringToByteArray(target,tgtLen,NULL,0,status);
                        uint8_t* myTarget = (uint8_t*) malloc(sizeof(uint8_t) * myTargetLen);

                        /* test for NULL */
                        if(myTarget == NULL) {
                            *status = U_MEMORY_ALLOCATION_ERROR;
                            return; 
                        }

                        int i=0;
                        int32_t retVal=0;

                        *status = U_ZERO_ERROR;
                        retVal=rleStringToByteArray(target,tgtLen,myTarget,myTargetLen,status);
                        if(U_SUCCESS(*status)){

                            for(i=0; i< srcLen;i++){
                                if(res->u.fBinaryValue.fData[i]!= myTarget[i]){
                                    printf("the encoded string cannot be decoded Expected : 0x%02X Got : %: 0x%02X at %i\n",res->u.fBinaryValue.fData[i],myTarget[i], i);
                                }
                            }
                        }else{
                            printf("Could not decode got error : %s \n", u_errorName(*status));
                        }
                        free(myTarget);

                    }
#endif

                }else{
                    *status = U_MEMORY_ALLOCATION_ERROR;
                    return;
                }

            }
        
        

            write_tabs(out);
            T_FileStream_write(out, type, (int32_t)uprv_strlen(type));
            T_FileStream_write(out, "\n", 1);
            tabCount++;
            write_tabs(out);
            str_write_java(target, tgtLen,FALSE, status);
            tabCount--;
            T_FileStream_write(out, "),\n", 3);

            free(target);
        }
   }else{
        write_tabs(out);
        T_FileStream_write(out,type,uprv_strlen(type));
        T_FileStream_write(out,"null),\n",7);
   }

}


static UBool start = TRUE;

static void 
table_write_java(struct SResource *res, UErrorCode *status) {
    uint32_t  i         = 0;
    UBool allStrings =TRUE;
    struct SResource *current = NULL;
    struct SResource *save = NULL;
    const char* obj = "new Object[][]{\n";

    if (U_FAILURE(*status)) {
        return ;
    }
    
    if (res->u.fTable.fCount > 0) {
        if(start==FALSE){
            write_tabs(out);
            T_FileStream_write(out, obj, (int32_t)uprv_strlen(obj));
            tabCount++;
        }
        start = FALSE;
        save = current = res->u.fTable.fFirst;
        i       = 0;


        while (current != NULL) {
            assert(i < res->u.fTable.fCount);
            write_tabs(out);
            
            T_FileStream_write(out, "{\n", 2);


            tabCount++;
            allStrings=FALSE;

            write_tabs(out);

            T_FileStream_write(out, "\"", 1);
            T_FileStream_write(out, srBundle->fKeys+current->fKey,
                               (int32_t)uprv_strlen(srBundle->fKeys+current->fKey));
            T_FileStream_write(out, "\",\n", 2);

            T_FileStream_write(out, "\n", 1);
           
            res_write_java(current, status);
            if(U_FAILURE(*status)){
                return;
            }
            i++;
            current = current->fNext;
            tabCount--;
            write_tabs(out);
            T_FileStream_write(out, "},\n", 3);
        }
        if(tabCount>4){
            tabCount--;
            write_tabs(out);
            T_FileStream_write(out, "},\n", 3);
        }

    } else {
        write_tabs(out);
        T_FileStream_write(out,obj,uprv_strlen(obj));

        write_tabs(out);
        T_FileStream_write(out,"},\n",3);

    }

}

void 
res_write_java(struct SResource *res,UErrorCode *status) {
    
    if (U_FAILURE(*status)) {
        return ;
    }

    if (res != NULL) {
        switch (res->fType) {
        case RES_STRING:
             string_write_java    (res, status);
             return;
        case RES_ALIAS:
             alias_write_java     (res, status);
             return;
        case RES_INT_VECTOR:
             intvector_write_java (res, status);
             return;
        case RES_BINARY:
             bin_write_java       (res, status);
             return;
        case RES_INT:
             int_write_java       (res, status);
             return;
        case RES_ARRAY:
             array_write_java     (res, status);
             return;
        case RES_TABLE:
             table_write_java     (res, status);
             return;

        default:
            break;
        }
    }

    *status = U_INTERNAL_PROGRAM_ERROR;
}

void 
bundle_write_java(struct SRBRoot *bundle, const char *outputDir,const char* outputEnc, 
                  char *writtenFilename, int writtenFilenameLen, 
                  const char* packageName, const char* bundleName,
                  UErrorCode *status) {

    char fileName[256] = {'\0'};
    char className[256]={'\0'};
    char constructor[1000] = { 0 };    
    UBool j1 =FALSE;
    outDir = outputDir;

    bName = (bundleName==NULL) ? "LocaleElements" : bundleName;
    pName = (packageName==NULL)? "com.ibm.icu.impl.data" : packageName;
    
    uprv_strcpy(className, bName);
    srBundle = bundle;
    if(uprv_strcmp(srBundle->fLocale,"root")!=0){
        uprv_strcat(className,"_");
        uprv_strcat(className,srBundle->fLocale);
    }
    if(outputDir){
        uprv_strcpy(fileName, outputDir);
        if(outputDir[uprv_strlen(outputDir)-1] !=U_FILE_SEP_CHAR){
            uprv_strcat(fileName,U_FILE_SEP_STRING);
        }
        uprv_strcat(fileName,className);
        uprv_strcat(fileName,".java");
    }else{
        uprv_strcat(fileName,className);
        uprv_strcat(fileName,".java");
    }

    if (writtenFilename) {
        uprv_strncpy(writtenFilename, fileName, writtenFilenameLen);
    }

    if (U_FAILURE(*status)) {
        return;
    }
    
    out= T_FileStream_open(fileName,"w");

    if(out==NULL){
        *status = U_FILE_ACCESS_ERROR;
        return;
    }
    if(getIncludeCopyright()){
        T_FileStream_write(out, copyRight, (int32_t)uprv_strlen(copyRight));
        T_FileStream_write(out, warningMsg, (int32_t)uprv_strlen(warningMsg));
    }
    T_FileStream_write(out,"package ",(int32_t)uprv_strlen("package "));
    T_FileStream_write(out,pName,(int32_t)uprv_strlen(pName));
    T_FileStream_write(out,";\n\n",3);
    T_FileStream_write(out, javaClass, (int32_t)uprv_strlen(javaClass));
    T_FileStream_write(out, className, (int32_t)uprv_strlen(className));
    if(j1){
        T_FileStream_write(out, javaClass1, (int32_t)uprv_strlen(javaClass1));
    }else{
        sprintf(constructor,javaClassICU,className);
        T_FileStream_write(out, constructor, (int32_t)uprv_strlen(constructor));
    }

    if(outputEnc && *outputEnc!='\0'){
        /* store the output encoding */
        enc = outputEnc;
        conv=ucnv_open(enc,status);
        if(U_FAILURE(*status)){
            return;
        }
    }
    res_write_java(bundle->fRoot, status);

    T_FileStream_write(out, closeClass, (int32_t)uprv_strlen(closeClass));

    T_FileStream_close(out);

    ucnv_close(conv);
}

