/*
*******************************************************************************
*
*   Copyright (C) 2000, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*
*  Modification History:
*
*   Date        Name        Description
*   15-Jul-2001 Ed Batutis  Created.
*
* genmucm.cpp : Generate ucm file using Microsoft IMultiLang
* 
* genmucm [path_to_unicode_data_txt]
*
*
* Win32 Notes:
*   This program requires MSVC plus the Platform SDK from April 2000 or later.
*   Also required is MLANG.DLL version 5.5 or later which is installed
*   with IE 5.5. The MLANG.DLL that comes with Win2K will not work.
*/

// todo:
// Deal with surrogates and UChar32 - currently Windows does not support
// gb18030 or converters with surrogates, so no big hurry. Windows XP will.
// - eb

#include <stdio.h>
#include <time.h>
#include "convert.h"

void 
save_byte_range(byte_info_ptr byte_info, char *str, size_t offset, size_t size);


// utility function prototypes

/**
 * Create a Vector of strings of possible byte sequences from byte_info_ptr to try out.
 * @param cp_data A Vector of strings
 */
void gen_cp_strings(byte_info_ptr byte_range_ptr, UVector &cp_data);

/**
 * Given a byte sequence, write the escaped from to outBuff and return it.
 */
char *gen_hex_escape(char *str, char *outBuff, size_t size);

void print_icu_state(byte_info_ptr info, FILE* fp);
const char *print_icu_features(char *featureBuf, uint32_t features, const converter &cnv);
UBool print_ranges(int8_t arr[], size_t len, int print_threshold, int level, FILE* fp);

// UCM file info

FILE *open_ucm(const char* p_ucm_filename);
const char* gen_canonical_name(const converter &cnv);

void emit_ucm_header(FILE* fp,
                     const converter &cp,
                     encoding_info* encoding_info,
                     const cp_info& cp_inf,
                     uint32_t featureSet);

void emit_ucm_tail(FILE* fp);

const char *getPremadeStateTable(cp_id cp);

int probe_lead_bytes(converter& cnv, byte_info_ptr byte_range);

// Default reverse fallback character to Unicode
#define DEFAULT_RFB_CHAR 0x30FB

#define IS_PUA(unicode_char_32) (0xE000 <= unicode_char_32 && unicode_char_32 <= 0xF8FF)

typedef enum {
    ASCII           = 0x01, // Normal vanilla ASCII ISO-646
    VASCII          = 0x02, // Changed the standard mapping of 0x20-0x7E, like the dollar sign.
    EBCDIC          = 0x04, // Normal vanilla EBCDIC
    EBCDIC_NLLF     = 0x08, // Swapped the newline and linefeed on EBCDIC (os/390)
    PRIVATE_USE     = 0x10, // Uses the private use area of Unicode
    VCONTROLS       = 0x20, // Mapped the ISO control codes to something else like graphical characters
    VSUB            = 0x40, // Rotated the ISO control codes 1a->7f, 7f->1c, 1c->1a
    ERR_UNKNOWN     = 0x8000    // Didn't collect it properly
} EncodingFeature;

/**
 * Get the EncodingFeatures from the \u -> \x mappings.
 */
uint32_t getEncodingFeatures(UHashtable *uni_to_cp, UBool used_PUA);


int main(int argc, const char* const argv[])
{
    UErrorCode status = U_ZERO_ERROR;
    UVector encodings(200, status);
    UHashtable *pmap_encoding_info = uhash_openSize(uhash_hashLong, uhash_compareLong, 65537, &status);

    argc--; argv++;
    if (U_FAILURE(status))
    {
        printf("Error %s:%d %s", __FILE__, __LINE__, u_errorName(status));
    }

    // query the OS for a list of encodings to analyze
    
    if (converter::get_supported_encodings(&encodings, pmap_encoding_info, argc, argv))
    {
        return 1;
    }
    
    for (int i = 0; i < encodings.size(); i++) 
    {

        byte_info byte_range[MAX_BYTE_LEN];
        size_t min_byte_size = 0xffff;
        size_t max_byte_size = 0;
        UBool used_PUA = FALSE;
        
#if CP_ID_IS_INT
        cp_id cp = encodings.elementAti(i);
        encoding_info *encoding_info = (struct encoding_info *)uhash_iget(pmap_encoding_info, cp);
#else
        cp_id cp = (cp_id)encodings.elementAt(i);
        encoding_info *encoding_info = (struct encoding_info *)uhash_get(pmap_encoding_info, cp);
#endif
        
        memset(byte_range, 0, MAX_BYTE_LEN * sizeof(byte_info));
        
        // create a system Unicode <-> cp converter
        
        converter cnv(cp, encoding_info);

        // skip encodings we don't want to make a ucm file for
        if (cnv.is_ignorable())
        {
            fprintf(stdout, "\tSkipping %s (%s) (%s)\n", encoding_info->charset_description, encoding_info->web_charset_name, cnv.get_name());
            continue;
        }
        
        if ( 0 == cnv.get_status() )
        {
            fprintf(stdout, "%s - %s - %s, ", cnv.get_name(), encoding_info->charset_description, encoding_info->web_charset_name);
        }
        else
        {
            fprintf(stderr, "Failed opening converter for code page %s: %s (%s)\n", cnv.get_name(), encoding_info->charset_description, encoding_info->web_charset_name);
            continue;
        }

        // lookup tables 
        UHashtable *uni_to_cp = uhash_openSize(uhash_hashLong, uhash_compareLong, 65537, &status);
        UHashtable *cp_to_uni_by_uni = uhash_openSize(uhash_hashLong, uhash_compareLong, 65537, &status);
        UHashtable *cp_to_uni_by_cp = uhash_openSize(uhash_hashChars, uhash_compareChars, 65537, &status);
        uhash_setValueDeleter(uni_to_cp, uhash_freeBlock);
        uhash_setValueDeleter(cp_to_uni_by_uni, uhash_deleteUVector);
        uhash_setValueDeleter(cp_to_uni_by_cp, uhash_deleteUVector);
//        map<UChar32, string> uni_to_cp;                 // 1\uA -> n\xB fb
//        map<UChar32, vector<string> > cp_to_uni_by_uni; // 1\uA <- n\xB rt after fb
//        map<string, vector<UChar32> > cp_to_uni_by_cp;  // n\uA <- 1\xC rfb
        
        if (U_FAILURE(status)) {
            printf("Error %s:%d %s", __FILE__, __LINE__, u_errorName(status));
        }

        // Unicode to code page loop
        
        for (UChar32 unicode_char = 0; unicode_char <= MAX_UNICODE_VALUE; unicode_char++ )
        {
            UChar source_uni[4];
            char cp[80];
            size_t len_uni = 0;
            size_t targ_size;

            if (unicode_char == 0xD800) { // skip surrogates
                unicode_char = 0xE000;
            }
            status = U_ZERO_ERROR;
            
            UTF16_APPEND_CHAR_SAFE(source_uni, len_uni, sizeof(source_uni), unicode_char);
            source_uni[len_uni] = 0;
            
            targ_size = cnv.from_unicode(cp, cp+sizeof(cp), source_uni, source_uni+len_uni);
            cp[targ_size] = 0;   // NULL terminate just in case
            
            if (targ_size) 
            {
                char *scp = uprv_strdup(cp);
                
                uhash_iput(uni_to_cp, unicode_char, scp, &status);
                
                if (targ_size > MAX_BYTE_LEN)
                {
                    size_t u;
                    printf("targ_size overflow! Uni: ");
                    for (u = 0; u < len_uni; u++)
                        printf("%04X ", source_uni[u]);
                    fputs("cp: ", stderr);
                    for (u = 0; u < targ_size; u++)
                        printf("%02X ", ((unsigned char) cp[u]));
                    fputs("\n", stderr);
                }
                else {
                    save_byte_range(&byte_range[0], scp, 0, targ_size);
                    if (IS_PUA(unicode_char)) {
                        used_PUA = TRUE;
                    }
                }
                
                if (targ_size < min_byte_size)
                    min_byte_size = targ_size;
                
                if (targ_size > max_byte_size)
                    max_byte_size = targ_size;
                
            }
            
        } // unicode chars
        
        fprintf(stdout, "%d-%d bytes ", min_byte_size, max_byte_size);
        
        if (2 == max_byte_size && cnv.is_lead_byte_probeable())
        {
            probe_lead_bytes(cnv, byte_range);
        }
        // generate code page char iterator

        // A vector of strings
        UVector cp_data(uhash_freeBlock, NULL, 255, status);;
        if (U_FAILURE(status)) {
            printf("Error %s:%d %s", __FILE__, __LINE__, u_errorName(status));
        }

        gen_cp_strings(byte_range, cp_data);
        
        // code page to Unicode loop
        
        for (int j = 0; j < cp_data.size(); j++)
        {
            UChar32 uni32;
            UChar unibuff[16];
            char buff[MAX_BYTE_LEN + 4];
            const char* source = buff;
            size_t targ_size;
            
            // Oddity
            // odd behavior of the MLang converter from IE5.5: some characters
            // do not convert properly if they are alone in the buffer. Padding with
            // 2 or more NULL bytes seems to make the converter work properly. MLang
            // will simply drop the characters without the NULL padding.
            
            memset(buff, 0, sizeof(buff));
            strcpy(buff, (char*)cp_data[j]);
            
            memset(unibuff, 0, sizeof(unibuff));
            
            targ_size = cnv.to_unicode(unibuff, unibuff+(sizeof(unibuff)/sizeof(UChar)), source, source+sizeof(buff));
            
            // ignore trailing NULLs...
            
            while (targ_size && 0 == unibuff[targ_size-1]) 
            {
                targ_size--;
            }
            
            // ... unless the NULL is supposed to be there
            if (0 == *source && 0 == targ_size)
            {
                targ_size = 1;
            }
            
            if (targ_size)
            {
                if (targ_size > 1) 
                {
                    if (!UTF_IS_SURROGATE(unibuff[0])) {
                        int idx = 0;
                        puts("");
                        while (unibuff[idx])
                        {
                            printf("<U%04X>", unibuff[idx++]);
                        }

                        printf(" is not a surrogate. Ignoring this mapping.", unibuff[0]);
                        continue;
                    }
                    UTF16_GET_CHAR_SAFE(unibuff, 0, 0, 2, uni32, TRUE);
                }
                else
                {
                    uni32 = (UChar32)unibuff[0];
                }
                if (uni32 == 0xFFFF || uni32 == 0xFFFE || uni32 == 0xFFFD) {
                    /* looks suspicious */
                    char *cp_data_uni_to_cp = (char *)uhash_iget(uni_to_cp, uni32);
                    if (NULL == cp_data_uni_to_cp || strcmp(cp_data_uni_to_cp, buff)!=0) {
                        /* A reverse fallback to \uFFFF? That's bad */
                        printf("\nIgnoring the mapping to <U%04X>", uni32);
                        continue;
                    }
                }
                
                if (IS_PUA(uni32)) {
                    used_PUA = TRUE;
                }
//                if (cp_to_uni_by_uni[uni32].size() > 0) {
//                    printf("Double mapping %X\n", uni32);
//                }
//                cp_to_uni_by_uni[uni32].push_back(cp_data[j]);
                UVector *pvect = (UVector *)uhash_iget(cp_to_uni_by_uni, uni32);
                if (pvect == NULL)
                {
                    /* It's not there yet, let's add it. */
                    pvect = new UVector(1, status);
                    uhash_iput(cp_to_uni_by_uni, uni32, pvect, &status);
                }
                /* else we have a multiple mapping */
                pvect->addElement(cp_data[j], status);
                if (U_FAILURE(status)) {
                    printf("Error %s:%d %s", __FILE__, __LINE__, u_errorName(status));
                }
                
//                cp_to_uni_by_cp[cp_data[j]].push_back(uni32);
                pvect = (UVector *)uhash_get(cp_to_uni_by_cp, cp_data[j]);
                if (pvect == NULL)
                {
                    /* It's not there yet, let's add it. */
                    pvect = new UVector(1, status);
                    uhash_put(cp_to_uni_by_cp, cp_data[j], pvect, &status);
                }
                pvect->addElement(uni32, status);
                if (U_FAILURE(status)) {
                    printf("Error %s:%d %s", __FILE__, __LINE__, u_errorName(status));
                }
            }
        }
        
        // generate ucm file
        
        const char* p_ucm_filename = gen_canonical_name(cnv);
        if (uhash_count(uni_to_cp) < 0x7f) {
            fprintf(stdout, "Incomplete mapping.  Not generating %s\n", p_ucm_filename);
            continue;
        }
        FILE *fp = open_ucm(p_ucm_filename);
        cp_info cp_inf;
        
        fprintf(stdout, "-> %s\n", p_ucm_filename);
        
        // get some converter info from the converter itself
        
        cnv.get_cp_info(cp, cp_inf);
        
        // other converter info has been generated here
        
        cp_inf.min_byte_size = min_byte_size;
        cp_inf.max_byte_size = max_byte_size;
        cp_inf.byte_info = byte_range;
        
        uint32_t features = getEncodingFeatures(uni_to_cp, used_PUA);
        emit_ucm_header(fp, cnv, encoding_info, cp_inf, features);
        
        for ( UChar32 uni = 0 ; uni <= MAX_UNICODE_VALUE ; uni++ )
        {
//            UChar32 uni;
            static char hex_buff1[MAX_BYTE_LEN * 4]; // byte -> \xNN
            bool f_fallback;
            char *cp_data_uni_to_cp;
            
            cp_data_uni_to_cp = (char *)uhash_iget(uni_to_cp, uni);
            
            // check for primary or fallback mapping (uni -> code page)
            if ( cp_data_uni_to_cp != NULL ) 
            {
                // is it a primary mapping or a fallback?
//                vector<UChar32> uni_vector = cp_to_uni_by_cp[cp_data_uni_to_cp];
                UVector *uni_vector = (UVector*)uhash_get(cp_to_uni_by_cp, cp_data_uni_to_cp);
                if (uni_vector != NULL && uni_vector->size() > 0) {
                    if (uni_vector->size() > 1)
                    {
                        fprintf(stdout, "Too many mappings for <U%04X> %s\n", uni, gen_hex_escape(cp_data_uni_to_cp, hex_buff1, sizeof(hex_buff1)));
                    }
                    else
                    {
                        f_fallback = (uni_vector->elementAti(0) != uni);
                        fprintf(fp, "<U%04X> %s |%d\n", uni, gen_hex_escape(cp_data_uni_to_cp, hex_buff1, sizeof(hex_buff1)), f_fallback);
                    }
                }
                else {
                    gen_hex_escape(cp_data_uni_to_cp, hex_buff1, sizeof(hex_buff1));
                    printf("Missing mapping for <U%04X> %s\n", uni, hex_buff1);
                    fprintf(fp, "<U%04X> %s |1 # No roundtrip\n", uni, hex_buff1);
                }
            }
            
            // check for 'reverse fallback' mapping (code page -> uni)
            
            if (uni != cp_inf.default_uchar && uni != DEFAULT_RFB_CHAR)
            {
                UVector *pvect_cp_to_uni = (UVector *)uhash_iget(cp_to_uni_by_uni, uni);
        
                if ( pvect_cp_to_uni != NULL )
                {
                    int32_t cpIdx;
                    int32_t pvect_cp_to_uni_limit = pvect_cp_to_uni->size();
                    for (cpIdx = 0; cpIdx < pvect_cp_to_uni_limit; cpIdx++)
                    {
                        char *cpdataCP2Uni = (char *)pvect_cp_to_uni->elementAt(cpIdx);


                        if (!cpdataCP2Uni) {
                            printf("Error cpdataCP2Uni is NULL at %d %s:%d\n", cpIdx, __FILE__, __LINE__);
                            exit(1);
                        }
                        if (cp_data_uni_to_cp == NULL || strcmp(cpdataCP2Uni, cp_data_uni_to_cp) != 0)
                        {
                            // Oddity
/*                            if (!cp_data_uni_to_cp) {
                                if (uni == (UChar)((uint8_t)cpdataCP2Uni[0])) {
                                    // no roundtrip and it looks like only the first byte was converted!
                                    printf("reverse fallback <U%04X> ignored\n", uni);
                                    continue;
                                }
                                printf("Warning <U%04X> doesn't have a roundtrip\n", uni);
                            }*/
//                            string default_char = cp_inf.default_char;
                            fprintf(fp, "<U%04X> %s |3\n", uni, gen_hex_escape(cpdataCP2Uni, hex_buff1, sizeof(hex_buff1)));
                        }
                    }
                }
            }
        }
        
        uhash_close(uni_to_cp);
        uhash_close(cp_to_uni_by_uni);
        uhash_close(cp_to_uni_by_cp);
        emit_ucm_tail(fp);
        fclose(fp);
        
   } // encodings
   
   return 0;
}

//int32_t getEncodingFeatures(map<UChar32, string> uni_to_cp, UBool used_PUA)
uint32_t getEncodingFeatures(UHashtable *uni_to_cp, UBool used_PUA)
{
    uint32_t feature = 0;
    int32_t uni;

    if (uhash_count(uni_to_cp) < 0x7f) {
        fprintf(stdout, "Incomplete mapping\n");
        return ERR_UNKNOWN;
    }

    char *letterA = (char *)uhash_iget(uni_to_cp, 0x41);
    char *newline = (char *)uhash_iget(uni_to_cp, 0x0A);

    if ( letterA == NULL || newline == NULL) {
        fprintf(stdout, "Can't find A or \\n\n");
        return ERR_UNKNOWN;
    }

    if(letterA[1] == 0) {
        if(letterA[0]==0x41) {
            feature |= ASCII;
        } else if(letterA[0]==(char)0xc1) {
            feature |= EBCDIC;
        }
    }
    if(newline[1]==0) {
        if(newline[0]==0x25) {
            feature |= EBCDIC;
        } else if(newline[0]==0x15) {
            feature |= (EBCDIC | EBCDIC_NLLF);
        }
    }

    if ((feature & EBCDIC) == 0) {
        for (uni = 0; uni < 0x80; uni++) {
            char *cp_data_uni_to_cp = (char *)uhash_iget(uni_to_cp, uni);

            if ( cp_data_uni_to_cp == NULL ) 
            {
                // Didn't map the normal part of ASCII. Something else must be here.
                feature |= VASCII;
                continue;
            }
            // is it a primary mapping or a fallback?
            uint8_t cp_byte = cp_data_uni_to_cp[0];
            if (cp_byte && cp_data_uni_to_cp[1] != 0) {
                /* multibyte encoding for ASCII range 1-7f */
                feature |= VASCII;
            } else if (uni != cp_byte) {
                /* IBM PC rotation of SUB and other controls: 0x1a->0x7f->0x1c->0x1a */
                if (uni==0x1a && cp_byte==0x7f
                 || uni==0x1c && cp_byte==0x1a
                 || uni==0x7f && cp_byte==0x1c) {
                    feature |= (ASCII | VSUB);
                } else if (uni < 0x20 || uni == 0x7f) {
                    feature |= VCONTROLS;
                } else {
                    feature |= VASCII;
                }
            }
        }
    }

    if (used_PUA) {
        feature |= PRIVATE_USE;
    }
//    for (uni = 0xE000; uni < 0xF900; uni++) {
        // check for primary or fallback mapping (uni -> code page)
//        if ( uni_to_cp.find(uni) != uni_to_cp.end() ) 
//        {
//            feature |= PRIVATE_USE;
//            break;
//        }
//    }
    return feature;
}


// Generate input data for code page to Unicode conversion using recursion.

void
get_strings(char *str, byte_info_ptr pbyte_range, UVector &cp_data)
{
    UErrorCode status = U_ZERO_ERROR;
    size_t str_len = uprv_strlen(str);
    size_t str_size = str_len + 1;
    size_t new_str_size = str_size + 1;

    for (int l = 0 ; l < 0x100 ; l++)
    {
        if ( pbyte_range->byte[l] == BYTE_INFO_END )
        {
            char *final_str = (char*)uprv_malloc(new_str_size);
            uprv_strcpy(final_str, str);

            // strcat
            final_str[str_len] = (char)l;
            final_str[str_size] = 0;

            cp_data.addElement(final_str, status);
            if (U_FAILURE(status)) {
                printf("Error %s:%d %s", __FILE__, __LINE__, u_errorName(status));
            }
        }
        else if ( pbyte_range->byte[l] == BYTE_INFO_CONTINUE )
        {
            char new_str[MAX_BYTE_LEN + 1];
            uprv_strcpy(new_str, str);

            // strcat
            new_str[str_len] = (char)l;
            new_str[str_size] = 0;
            
            get_strings(new_str, pbyte_range+1, cp_data);
        }
    }
}

void 
gen_cp_strings(byte_info_ptr pbyte_range, UVector &cp_data)
{
    char start_str[MAX_BYTE_LEN + 1];
    memset(start_str, 0, sizeof(start_str));
    
    get_strings(start_str, pbyte_range, cp_data);
}

// generate hex escape sequences for code page data in ucm format

char *gen_hex_escape(char *str, char *outBuff, size_t size)
{
    static char hex[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    char buff[8];
    
    // quick fix - empty string really means one NULL
    
    if ( str[0] == 0 )
    {
        uprv_strcpy(outBuff, "\\x00");
    }
    else {
        size_t str_len = uprv_strlen(str);
        buff[0] = '\\';
        buff[1] = 'x';
        buff[4] = 0;
        outBuff[0] = 0; // NULL terminate just in case.
        for ( size_t i = 0 ; i < str_len ; i++ )
        {
            unsigned char c = (unsigned char)str[i];
            buff[2] = hex[(c >> 4) & 0xF];
            buff[3] = hex[c & 0xF];
        
    //        sprintf(buff, "\\x%02X", (unsigned char)cstr[i]);
        
            strcat(outBuff, buff);
        }
    }
    
    return outBuff;
}


void emit_ucm_header(FILE* fp, const converter &cnv, encoding_info* encoding_info, const cp_info& cp_inf, uint32_t features)
{
    unsigned int i = 0;
    time_t currTime;
    char timeBuf[64];
    char icuInfoBuf[1024];
    
    time(&currTime);
    strftime(timeBuf, sizeof(timeBuf), "%b %d %H:%M %Z %Y", localtime(&currTime));
    
    fputs("# ***************************************************************************\n", fp);
    fputs("# *\n", fp);
    fputs("# *   Copyright (C) 2001-2003, International Business Machines\n", fp);
    fputs("# *   Corporation and others.  All Rights Reserved.\n", fp);
    fputs("# *\n", fp);
    fputs("# ***************************************************************************\n", fp);
    fputs("#\n", fp);
    fprintf(fp, "# File created on %s\n", timeBuf);
    fputs("#\n", fp);
    fputs("# File created by genmucm tool.\n", fp);
    fprintf(fp, "# from %s %s using %s\n", converter::get_OS_vendor(), converter::get_OS_variant(), converter::get_OS_interface());
    fputs("#\n", fp);
    fputs("# Table Version : 1.0\n", fp);
    fputs("# The 1st column is the Unicode scalar value.\n", fp);
    fputs("# The 2nd column is the codepage byte sequence.\n", fp);
    fputs("# The 3rd column is the fallback indicator.\n", fp);
    fputs("# The fallback indicator can have one of the following values:\n", fp);
    fputs("#   |0 for exact 1-1 roundtrip mapping\n", fp);
    fputs("#   |1 for the best fallback codepage byte sequence.\n", fp);
    fputs("#   |2 for the substitution character\n", fp);
    fputs("#   |3 for the best reverse fallback Unicode scaler value\n", fp);
    fputs("#\n", fp);
    fprintf(fp, "# Encoding description: %s\n", encoding_info->charset_description);
    fprintf(fp, "# Encoding name: %s\n", encoding_info->web_charset_name);
    fputs("#\n", fp);
    fprintf(fp, "<code_set_name>               \"%s\"\n", gen_canonical_name(cnv));
    /* skip <char_name_mask>, no use for it */
    /* use memcmp later */
    fprintf(fp, "<mb_cur_max>                  %d\n", cp_inf.max_byte_size);
    /* SBCS and MBCS for now */
    fprintf(fp, "<mb_cur_min>                  %d\n", cp_inf.min_byte_size);
    fputs("<uconv_class>                 ", fp);
    if (cp_inf.max_byte_size <= 1) {
        fputs("\"SBCS\"\n", fp);
        fprintf(fp, "<subchar>                     \\x%02X\n", (unsigned char)cp_inf.default_char[0]);
        fputs(print_icu_features(icuInfoBuf, features, cnv), fp);
    } else {
        if (cp_inf.max_byte_size == 2 && cp_inf.min_byte_size == 2) {
            fputs("\"DBCS\"\n", fp);    // Might be dangerous because a certain EBCDIC state table is assumed!
        }
        else {
            fputs("\"MBCS\"\n", fp);
        }
        fputs("<subchar>                     ", fp);
        for (i = 0; i < strlen((char*)cp_inf.default_char); i++) {
            fprintf(fp, "\\x%02X", (unsigned char)cp_inf.default_char[i]);
        }
        fputs("\n", fp);
        fputs(print_icu_features(icuInfoBuf, features, cnv), fp);
        const char *premade_state_table = cnv.get_premade_state_table();
        if (premade_state_table) {
            fputs(premade_state_table, fp);
        }
        print_icu_state(cp_inf.byte_info, fp);
    }
    fputs("#\n", fp);
    fputs("CHARMAP\n", fp);
    fputs("#\n", fp);
    fprintf(fp, "#UNICODE %s\n", cnv.get_name());
    fputs("#_______ _________\n", fp);
}

void emit_ucm_tail(FILE* fp)
{
    /* emit ending */
    fputs("#\n", fp);
    fputs("END CHARMAP\n", fp);
    fputs("#\n", fp);
}


const char *print_icu_features(char *featureBuf, uint32_t features, const converter &cnv)
{
    const char *charset_family = NULL;
    char buffer[256];

    if (features & ASCII) {
        charset_family = "ASCII";
    } else if (features & EBCDIC) {
        charset_family = "EBCDIC";
    }
    if (charset_family) {
        sprintf(featureBuf, "<icu:charsetFamily>           \"%s\"\n", charset_family);
    }
    else {
        sprintf(featureBuf, "# Unknown <icu:charsetFamily>\n");
    }
    sprintf(buffer, "# Suggested ICU specific alias information\n#<icu:alias>                  \"%s-%s", converter::get_OS_vendor(), cnv.get_name());
    if ((features & (VASCII | EBCDIC_NLLF | PRIVATE_USE | VCONTROLS | VSUB)) == 0) {
        strcat(buffer, "_STD");
    } else {
        /* add variant indicators in alphabetic order */
        if (features & VASCII) {
            strcat(buffer, "_VASCII");
        }
        if (features & VCONTROLS) {
            strcat(buffer, "_VGCTRL");
        }
        if (features & EBCDIC_NLLF) {
            strcat(buffer, "_VLF");
        }
        if (features & VSUB) {
            strcat(buffer, "_VSUB");
        }
        if (features & PRIVATE_USE) {
            strcat(buffer, "_VPUA");
        }
    }
    strcat(buffer, "\"\n\n");
    strcat(featureBuf, buffer);
    return featureBuf;
}

const char* gen_canonical_name(const converter &cnv)
{
    static char buff[100];
    static char name[80];
    int32_t idx;
    int32_t name_size;

    strcpy(name, cnv.get_name());
    name_size = (int32_t)strlen(name);
    for (idx = 0; idx < name_size; idx++)
    {
        if (name[idx] == '-')
        {
            name[idx] = '_';
        }
    }

    sprintf(buff, "%s-%s-%s", converter::get_OS_vendor(), name, converter::get_OS_variant());

    return buff;
}

FILE *open_ucm(const char* p_ucm_filename)
{
    char name[80];
    
    strcpy(name, p_ucm_filename);
    strcat(name, ".ucm");
    
    return fopen(name, "wt");
}

// emit text for <icu:state> entries in ucm

void
print_icu_state(byte_info_ptr info, FILE* fp)
{
    fputs("\n# The following was the generated state table.\n# This does not account for unassigned characters\n" ,fp);
    for (int i = 0; i < MAX_BYTE_LEN && print_ranges(info->byte, 0x100, 1, i, fp); i++, info++ )
    {
    }
}

UBool print_ranges(int8_t arr[], size_t len, int print_threshold, int level, FILE* fp)
{
    UErrorCode status = U_ZERO_ERROR;
    typedef struct pair {
        int first;
        int second;
        pair(int f, int s) : first(f), second(s) {}
    } pair;
    UVector v_ranges(255, status);   // A vector of int pairs
    
    pair *p = new pair(0, 0);
    size_t i;
    int last_val = arr[0];
    
    if (U_FAILURE(status)) {
        printf("Error %s:%d %s", __FILE__, __LINE__, u_errorName(status));
    }

    for ( i = 1 ; i < len ; i++ )
    {
        if ( last_val == arr[i] )
            p->second = i;
        else
        {
            v_ranges.addElement(p, status);
            p = new pair(i, i);
            if (U_FAILURE(status) || p == NULL) {
                printf("Error %s:%d %s", __FILE__, __LINE__, u_errorName(status));
            }
        }
        
        last_val = arr[i];
    }
    
    v_ranges.addElement(p, status);
    if (U_FAILURE(status)) {
        printf("Error %s:%d %s", __FILE__, __LINE__, u_errorName(status));
    }
    
    // now print the ranges in icu:state style if there are any ranges to print
    
    int num_printed = 0;
    size_t vrange_size = (size_t)v_ranges.size();
    
    for ( i = 0; i < vrange_size; i++ )
    {
        p = (pair *)v_ranges[i];
        
        if ( arr[p->first] < print_threshold )
            continue;
        
        if ( num_printed )
            fprintf(fp, ", ");
        else
            fprintf(fp, "%-30s","#<icu:state>");
        
        if ( p->first == p->second )
            fprintf(fp, "%x", p->first);
        else
            fprintf(fp, "%x-%x", p->first, p->second);
        
        num_printed++;
        
        if ( arr[p->first] > 1 )
            fprintf(fp, ":%d", level+1);
        delete p;
    }
    
    if ( num_printed )
        fputs("\n", fp);
    return (UBool)(num_printed != 0);
}

void 
save_byte_range(byte_info_ptr byte_info, char *str, size_t offset, size_t size)
{
    if ( offset < size )
    {
        uint8_t c = (uint8_t)str[offset];
        
        if ( (offset + 1) < size ) 
        {
            if (byte_info->byte[c] == BYTE_INFO_END)
            {
                // You have a stateful encoding like ISCII, iso-2022, hz, EBCDIC_STATEFUL
                printf("\n Overwriting state info at %X ", c);
            }
            byte_info->byte[c] = BYTE_INFO_CONTINUE;
            
            save_byte_range(byte_info+1, str, offset+1, size);
        }
        else
        {
            if (byte_info->byte[c] == BYTE_INFO_CONTINUE)
            {
                // You have a stateful encoding like ISCII, iso-2022, hz, EBCDIC_STATEFUL
                printf("\n Overwriting state info at %X ", c);
            }
            byte_info->byte[c] = BYTE_INFO_END; 
        }
        
    }
    
}

int
probe_lead_bytes(converter& cnv, byte_info_ptr byte_range)
{
    int ret = 0;
    int8_t lead_byte, trail_byte;
    size_t targ_size;
    UChar uni[8];
    UChar* target = uni;
    UChar* target_limit = uni+1;
    char src_buff[8];
    const char* source;
    const char* source_limit;
    
    for ( int i = 0 ; i < 0x100 ; i++ )
    {
        if ( byte_range->byte[i] )
            continue;
        
        for ( int j = 0 ; j < 0x100 ; j++ )
        {
            
            if ( 0 == (byte_range+1)->byte[j] )
                continue;
            
            lead_byte = i;
            trail_byte = j;
            
            src_buff[0] = lead_byte;
            src_buff[1] = trail_byte;
            
            source = src_buff;
            source_limit = source + 2;
            
            target = uni;
            target_limit = uni+1;
            
            targ_size = cnv.to_unicode(target, target_limit, source, source_limit);
            
            /* Was the whole thing converted? */
            if ( targ_size && ( *target != (UChar)(uint8_t)trail_byte ) && ( *target != (UChar)(uint8_t)lead_byte ) )
            {
                printf("\nTesting additional lead byte at %02X", (uint8_t)lead_byte);
                byte_range->byte[i] = BYTE_INFO_CONTINUE;
                
                ret = 1;
                
                break;
            }
            
        }
    }
    
    return ret;
}
