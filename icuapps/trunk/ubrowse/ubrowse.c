/*
*******************************************************************************
* Copyright (C) 1996-2000, International Business Machines Corporation and    *
* others. All Rights Reserved.                                                *
*******************************************************************************
* HTML Design by Josh Mast <josh@hivehaus.org>                                *
*******************************************************************************
*
* File ubrowse.c
*
* Modification History:
*
*   Date        Name        Description
*   06/11/99    stephen     Creation.
*   06/16/99    stephen     Modified to use uprint.
*   08/02/1999  srl         Unibrowse
*   12/02/1999  srl         Integrated design changes from Josh Mast
*******************************************************************************
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "unicode/utypes.h"
#include "unicode/ustring.h"

#include "unicode/ucnv.h"
#include "unicode/udat.h"
#include "unicode/ucal.h"
#include "unicode/uchar.h"
#include <unicode/ustdio.h>

#include "unicode/decompcb.h" /* from locexp/util */

#define HAVE_KANGXI
#define RADICAL_LIST
#include <kangxi.c> /* Kang-Xi Radical mapping table */

/* Protos */
int main(int argc, char **argv);
void doDecodeQueryField(const char *in, char *out);
 const char * up_u2c(char *buf, const UChar *src, int32_t len, UErrorCode *status);  
void printType(int8_t type);
void printScript(UCharScript script);
UChar32 doSearchScript(int32_t, UChar32 startFrom);
UChar32 doSearchType(int8_t, UChar32 startFrom);
void showSearchMenu(UChar32 startFrom);
void printCharName(UChar32 ch);

/* globals, current search.. */
int32_t gSearchType = -1;
int32_t gSearchScript = -1;
char    gSearchName[512];

UChar32 gSearchChar = 0xFFFF;
bool_t gSearchCharValid = FALSE;

bool_t anyDecompose = FALSE;

void printCharName(UChar32 c)
{
  char junkbuf[512];
  UErrorCode status = U_ZERO_ERROR;
  u_charName(c, U_UNICODE_CHAR_NAME, junkbuf, 512, &status);

  if(U_SUCCESS(status))
    puts(junkbuf);
  else
    printf("{U+%06X}", c);
}

/******************************************************** derived from ucnv_err.c */
#define ToOffset(a) a<=9?(0x0030+a):(0x0030+a+7)

/*Takes a int32_t and fills in  a UChar* string with that number "radix"-based
 * and padded with "pad" zeroes
 */
static void 
  itou (UChar * buffer, int32_t i, int32_t radix, int32_t pad)
{
  int32_t length = 0;
  int32_t num = 0;
  int8_t digit;
  int32_t j;
  UChar temp;

  while (i > radix)
    {
      num = i / radix;
      digit = (int8_t) (i - num * radix);
      buffer[length++] = (UChar) (ToOffset (digit));
      i = num;
    }

  buffer[length] = (UChar) (ToOffset (i));

  while (length < pad)
    buffer[++length] = (UChar) 0x0030;	/*zero padding */
  buffer[length--] = (UChar) 0x0000;

  /*Reverses the string */
  for (j = 0; j < (pad / 2); j++)
    {
      temp = buffer[length - j];
      buffer[length - j] = buffer[j];
      buffer[j] = temp;
    }

  return;
}
#if 0
void 
  SubstituteWithValueHTML (UConverter * _this,
					    char **target,
					    const char *targetLimit,
					    const UChar ** source,
					    const UChar * sourceLimit,
					    int32_t *offsets,
					    bool_t flush,
					    UErrorCode * err)
{
  UChar valueString[100];
  int32_t valueStringLength = 0;
  const UChar *mySource = *source;
  UChar codepoint[100];
  int32_t i = 0;
  /*Makes a bitwise copy of the converter passwd in */
  UConverter myConverter = *_this;
  char myTarget[100];
  char *myTargetAlias = myTarget;
  const UChar *myValueSource = NULL;
  UErrorCode err2 = U_ZERO_ERROR;


#ifdef WIN32
  if (!((*err == U_INVALID_CHAR_FOUND) || (*err == U_ILLEGAL_CHAR_FOUND)))    return;
#else
  if (CONVERSION_U_SUCCESS (*err))
    return;
#endif

  ucnv_reset (&myConverter);
  ucnv_setFromUCallBack (&myConverter,
			 (UConverterFromUCallback)   UCNV_FROM_U_CALLBACK_STOP, 
			 &err2);
  if (U_FAILURE (err2))
    {
      *err = err2;
      return;
    }
  u_uastrcpy(codepoint,"<I>\\u");

  while (i < _this->invalidUCharLength)
    {
      itou (codepoint + 5, _this->invalidUCharBuffer[i++], 16, 4);
      u_uastrcpy(codepoint+9, "</I>");
      memcpy (valueString + valueStringLength, codepoint, sizeof (UChar) * (5+4+4) );
      valueStringLength += (5+4+4) ;
    }

  myValueSource = valueString;

  /*converts unicode escape sequence */
  ucnv_fromUnicode (&myConverter,
		    &myTargetAlias,
		    myTargetAlias + 100,
		    &myValueSource,
		    myValueSource + (5+4+4),
		    NULL,
		    TRUE,
		    &err2);

  if (U_FAILURE (err2))
    {
      UCNV_FROM_U_CALLBACK_SUBSTITUTE (_this,
				       target,
				       targetLimit,
				       source,
				       sourceLimit,
				       offsets,
				       flush,
				       err);
      return;
    }

  valueStringLength = myTargetAlias - myTarget;

  /*if we have enough space on the output buffer we just copy
   * the subchar there and update the pointer
   */
  if ((targetLimit - *target) >= valueStringLength)
    {
      memcpy (*target, myTarget, valueStringLength);
      *target += valueStringLength;
      *err = U_ZERO_ERROR;
    }
  else
    {
      /*if we don't have enough space on the output buffer
       *we copy as much as we can to it, update that pointer.
       *copy the rest in the internal buffer, and increase the
       *length marker
       */
      memcpy (*target, valueString, (targetLimit - *target));
      memcpy (_this->charErrorBuffer + _this->charErrorBufferLength,
		  valueString + (targetLimit - *target),
		  valueStringLength - (targetLimit - *target));
      _this->charErrorBufferLength += valueStringLength - (targetLimit - *target);
      *target += (targetLimit - *target);
      *err = U_INDEX_OUTOFBOUNDS_ERROR;
    }

  return;
}
#endif
/*******************************************************end of borrowed code from ucnv_err.c **/


void printBasicBlockInfo(UChar32 theChar)
{
  char  theString[100];
  UChar chars[5];
  int n;
  UErrorCode status = U_ZERO_ERROR;
  int offset = 0;

  UTF_APPEND_CHAR_UNSAFE(chars, offset, theChar);
/*  chars[offset] = 0; */

  up_u2c(theString, chars, offset, &status);

  if(status == U_USING_FALLBACK_ERROR)
    {
      printf("<FONT COLOR=\"#00DD00\"><I>");
      anyDecompose = TRUE;
    }
  /*  fprintf(stderr, "%d", status); */

  puts("<FONT SIZE=+1>");
  puts(theString);

  if(status == U_USING_FALLBACK_ERROR)
      printf("</I></FONT>");
  puts("</FONT>");
}


UBool myEnumCharNamesFn(void *context,
                        UChar32 code,
                        UCharNameChoice nameChoice,
                        const char *name,
                        UTextOffset length)
{
  if(strstr(name, gSearchName))
  {
    printf("<LI><A HREF=\"?k1=%06X#here\"><TT>%06X</TT> - %s\r\n",
           code, code, name);
    
    printBasicBlockInfo( code );
    printf("</A>\r\n");
  }
  return TRUE;
}

int
main(int argc,
     char **argv)
{
  char *qs;
  char *pi;
  char *tmp;
  char theString[800];
  UChar chars[800];
  UChar32 theChar;
  int n,i,r,c,uc;
  bool_t searchedFor;
  UChar32 block = 0xFFFF;
  enum { ETOP, EBLOCK, ECOLUMN, ERADLST, ERADICAL, ENAME, EEXACTNAME } mode;
  UVersionInfo uvi;

  chars[1] = 0;
  pi = getenv("PATH_INFO");
  if(!pi)
    pi = "/iso-8859-1";


  pi++;

  tmp = strchr(pi, '/');
  if(tmp)
    *tmp = 0; /* terminate */

  {
    if(*pi==0) pi = NULL;
    ucnv_setDefaultName(pi);
    printf("content-type: text/html;charset=%s\r\n\r\n",pi);
  }

  printf("<HTML><HEAD>\r\n");
  if(!tmp) /* if there was no trailing '/' ... */
  {
    if(pi!=NULL) {
      printf("<BASE HREF=\"http://%s%s/%s/\">\r\n", getenv("SERVER_NAME"),
             getenv("SCRIPT_NAME"),
             pi);
    } else {
      printf("<BASE HREF=\"http://%s%s/\">\r\n", getenv("SERVER_NAME"),
             getenv("SCRIPT_NAME"));
    }
    
  }

  /* Now, see what we're supposed to do */
  qs = getenv("QUERY_STRING");
  
  mode = ETOP;

  if(qs)
    { 
      if (sscanf(qs,"n=%x", &block)== 1)
	{
	  block &= 0x1FFF00;
	  printf("<TITLE>ICU UnicodeBrowser: block U+%06X</TITLE>\r\n", block);
	  mode = EBLOCK;
	}
      else if(sscanf(qs, "k1=%x", &block) == 1)
	{
	  gSearchChar = block;
	  gSearchCharValid = TRUE;
	  block &= 0x1FFFF0;
	  printf("<TITLE>ICU UnicodeBrowser: column U+%06X</TITLE>\r\n", block);
	  mode = ECOLUMN;
	}
      else if(sscanf(qs, "k=%x", &block) == 1)
	{
	  block &= 0x1FFFF0;
	  printf("<TITLE>ICU UnicodeBrowser: column U+%06X</TITLE>\r\n", block);
	  mode = ECOLUMN;
	}
      else if(sscanf(qs, "scr=%d&b=%x", &gSearchScript, &block) == 2)
	{
	  block = doSearchScript(gSearchScript, block);
	  block &= 0x1FFFF0;
	  mode = ECOLUMN;
	}
      else if(sscanf(qs, "typ=%d&b=%x", &gSearchType, &block) == 2)
	{
	  block = doSearchType(gSearchType, block);
	  block &= 0x1FFFF0;
	  mode = ECOLUMN;
	}
      else if(sscanf(qs, "radlst=%d", &block) == 1)
	{
	  mode = ERADLST;
	}
      else if(sscanf(qs, "radical=%d", &block) == 1)
	{
	  mode = ERADICAL;
	}
      else if(sscanf(qs, "s=%200s", &gSearchName) == 1)
	{
	  mode = ENAME;
	}
      else if(sscanf(qs, "sx=%200s", &gSearchName) == 1)
        {
          mode = EEXACTNAME;
        }
    }

  if(mode == ETOP)
    {
      printf("<TITLE>ICU UnicodeBrowser</TITLE>\r\n");
    }


  printf("</HEAD>\r\n");
  
  printf("<BODY BGCOLOR=\"#FFFFFF\" link=\"green\" vlink=\"brown\">\r\n");
  
  printf("<table border=0><tr><td>"
         "<table border=0 cellpadding=0 cellspacing=0 width=100%><tr><td bgcolor=\"#000000\">\r\n"
         "<table border=0 cellpadding=1 cellspacing=1 width=100%><tr><td bgcolor=\"#cccccc\">\r\n");



  printf("<B>Encoding: %s.</B><BR>\r\n", pi);

  printf("</td></tr><tr><td bgcolor=\"#eeeeee\">\r\n");


  printf("<TABLE><TR><TD>");
  printf("<FORM>Jump to Unicode block: (hex) <INPUT NAME=n VALUE=\"%06X\"><INPUT TYPE=SUBMIT VALUE=\"Go\"></FORM><BR>", block);
  printf("<FORM>Jump to Unicode column: (hex) <INPUT NAME=k VALUE=\"%06X\"><INPUT TYPE=SUBMIT VALUE=\"Go\"></FORM><BR>", block);
  printf("</TD><TD><FORM><INPUT TYPE=SUBMIT NAME=n VALUE=\"Show All\"></FORM></TD></TR></TABLE>\r\n");

  printf("</td></tr></table>\r\n"
         "</td></tr></table>\r\n");

  printf("</td></tr><tr><td>\r\n");


  if(mode == ETOP) /* top level list of blocks */
    {
        printf("<table border=0 cellpadding=0 cellspacing=0><tr><td bgcolor=\"#000000\">\r\n"
               "<table border=0 cellpadding=1 cellspacing=1><tr><td bgcolor=\"#cccccc\">\r\n");

      printf("<b>Unicode Browser</b> - Click on a block to view it in more detail<br>\r\n");
      printf("</td></tr><tr><td bgcolor=\"#eeeeee\">\r\n"
             );

      printf("<TT>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;");
      for(n = 0;n<0x10;n++)
	{
	  printf("<B>_%X__</B> ", n);
	}
      for(n = 0x0000; n <= 0xFF00; n += 0x0100)
	{
	  if( (n% 0x1000) == (0x0000) )
	    printf("<BR>\r\n<B>%04X</B> ", n);
	  printf("<A HREF=\"?n=%06X\">%04X</A> ", n, n);
	}
      printf("<BR></TT>\r\n");

      printf(
             "</td></tr></table>\r\n"
             "</td></tr></table>\r\n"
             );

      printf("\r\n</td></tr><tr><td align=right>\r\n");
      
      showSearchMenu(0x0000);
    }      
  else if (mode == EBLOCK)
    {
      /* Unicode table  at block 'block' */

      printf("- Block: <A HREF=\"?n=%06X\">%06X</A> -\r\n",
	     (block & 0x1FFF00)-0x100, (block & 0x1FFF00)-0x100);
      printf("<A HREF=\"?n=%06X\">%06X</A> -\r\n",
	     (block & 0x1FFF00)+0x100, (block & 0x1FFF00)+0x100);

      n = 0;
      /*      for(n=0; n<0x100; n += 0x080) */
	{
	  printf("<TT><TABLE BORDER=1>");
	  printf("<TR><TD></TD>");
	  for(c = n;c<(n + 0x100);c+= 0x10)
	    {
	      printf("<TD><B><A HREF=\"?k=%06X\"><TT>%03X</TT></A></B></TD>", (block|c),  (block | c) >> 4   );
	    }
	  printf("</TR>\r\n");
	  for(r = 0; r < 0x10; r++)
	    {
	      printf("<TR><TD><B><TT>%X</TT></B></TD>", r);
	      for(c = n;c<(n + 0x100);c+= 0x10)
		{
		  theChar = (block | r | c );

		  printf("<TD ");
		  
		  if(u_charType(theChar) == U_UNASSIGNED)
		    {
		      printf(" BGCOLOR=\"#888888\" ");
		    }
		  
		  printf(" ALIGN=CENTER>\r\n");
		  
		  
		  /* print the simple data */
		  printBasicBlockInfo(theChar);
		  
		  printf("</TD>");
		}
	      printf("<TD><B><TT>%X</TT></B></TD>", r);
	    }
	}
	printf("</TR>");
	printf("<TR><TD COLSPAN=18 ALIGN=CENTER><I>Click on a column number to zoom in.</I></TD></TR>\r\n");
	printf("</TABLE></TT>");
      printf("<HR>\r\n");
      if(block <= 0xFFFF)
        {
          printf("<A HREF=\"http://charts.unicode.org/Unicode.charts/normal/U%04X.html#Glyphs\">this block on charts.unicode.org</A>\r\n", block);
        }
      showSearchMenu( block + 0x0100);
    }
  else if(mode == ECOLUMN )
    {
      /* Unicode table  at column 'block' */

      printf("<A HREF=\"?n=%06X\">View all of block %06X</A>\r\n",
	     block & 0x1FFF00, block & 0x1FFF00);
      /* should check view here */
      printf("- Column: <A HREF=\"?k=%06X\">%06X</A> -\r\n",
	     (block & 0x1FFFF0)-0x10, (block & 0x1FFFF0)-0x10);
      printf("<A HREF=\"?k=%06X\">%06X</A> -\r\n",
	     (block & 0x1FFFF0)+0x10, (block & 0x1FFFF0)+0x10);
      printf("<TT><TABLE BORDER=1><TR><TD></TD>");
      printf("<TD><B>%03X</B></TD>",  (block ) >> 4   );


      printf("<TD><B>Lower<BR>Upper<BR>Title</B></TD>");
      printf("<TD><B>#Digit<BR>DefinedDigit</B></TD>");
      printf("<TD><B>alph<BR>spc<BR>ctrl<BR>print</B></TD>");
      printf("<TD><B>base<BR>direction<BR>width<BR>type</B></TD>");
      printf("<TD><B>Script</B></TD>");

      printf("</TR>\r\n");
      for(r = 0; r < 0x10; r++) 
	{
	  theChar = (block | r );

	  printf("<TR><TD><B>%X</B></TD>", r);
	  printf("<TD ");

	  if( (u_charScript(theChar) == gSearchScript) || (u_charType(theChar) == gSearchType) || ((theChar == gSearchChar) && gSearchCharValid))
	    printf(" BGCOLOR=\"#EE0000\" ");
	  else if(u_charType(theChar) == U_UNASSIGNED)
	    {
	      printf(" BGCOLOR=\"#888888\" ");
	    }


	  printf(" ALIGN=CENTER>\r\n");
	  
	  if((theChar == gSearchChar) && gSearchCharValid)
	    printf("<A NAME=here></A>");
	  
	  /* print the simple data */
	  printBasicBlockInfo(theChar);
	  
	  printf("</TD>");

	  /* L/U/T */
	  printf("<TD>");
	  if(u_islower(theChar))
	    printf("L<BR>");
	  if(u_isupper(theChar))
	    printf("U<BR>");
	  if(u_istitle(theChar))
	    printf("T<BR>");

	  printf("</TD>");


	  /* digit/defined */
	  printf("<TD>");
	  if(u_isdigit(theChar))
	    printf("# %d<BR>", u_charDigitValue(theChar));
	  if(u_isdefined(theChar))
	    printf("Def<BR>");
	  printf("</TD>");


	  /* alpha.. */
	  printf("<TD>");
	  if(u_isalpha(theChar))
	    printf("alph<BR>");
	  if(u_isspace(theChar))
	    printf("spc<BR>");
	  if(u_iscntrl(theChar))
	    printf("ctrl<BR>");
	  if(u_isprint(theChar))
	    printf("print</BR>");
	  printf("</TD>");


	  /* base... */
	  searchedFor = (u_charType(theChar) == gSearchType);

	  printf("<TD ");
	  if(searchedFor)
	    printf(" BGCOLOR=\"#EE0000\" "); /* mark the one we were searching for */
	  
	  printf(" >");
	  if(u_isbase(theChar))
	    printf("base<BR>");
	  switch(u_charDirection(theChar))
	    {
	    case U_LEFT_TO_RIGHT: printf("LEFT_TO_RIGHT"); break; 
	    case U_RIGHT_TO_LEFT: printf("RIGHT_TO_LEFT"); break; 
	    case U_EUROPEAN_NUMBER: printf("EUROPEAN_NUMBER"); break; 
	    case U_EUROPEAN_NUMBER_SEPARATOR: printf("EUROPEAN_NUMBER_SEPARATOR"); break; 
	    case U_EUROPEAN_NUMBER_TERMINATOR: printf("EUROPEAN_NUMBER_TERMINATOR"); break; 
	    case U_ARABIC_NUMBER: printf("ARABIC_NUMBER"); break; 
	    case U_COMMON_NUMBER_SEPARATOR: printf("COMMON_NUMBER_SEPARATOR"); break; 
	    case U_BLOCK_SEPARATOR: printf("BLOCK_SEPARATOR"); break; 
	    case U_SEGMENT_SEPARATOR: printf("SEGMENT_SEPARATOR"); break; 
	    case U_WHITE_SPACE_NEUTRAL: printf("WHITE_SPACE_NEUTRAL"); break; 
	    case U_OTHER_NEUTRAL: printf("OTHER_NEUTRAL"); break; 
	    case U_LEFT_TO_RIGHT_EMBEDDING: printf("LEFT_TO_RIGHT_EMBEDDING"); break; 
	    case U_LEFT_TO_RIGHT_OVERRIDE: printf("LEFT_TO_RIGHT_OVERRIDE"); break; 
	    case U_RIGHT_TO_LEFT_ARABIC: printf("RIGHT_TO_LEFT_ARABIC"); break; 
	    case U_RIGHT_TO_LEFT_EMBEDDING: printf("RIGHT_TO_LEFT_EMBEDDING"); break; 
	    case U_RIGHT_TO_LEFT_OVERRIDE: printf("RIGHT_TO_LEFT_OVERRIDE"); break; 
	    case U_POP_DIRECTIONAL_FORMAT: printf("POP_DIRECTIONAL_FORMAT"); break; 
	    case U_DIR_NON_SPACING_MARK: printf("DIR_NON_SPACING_MARK"); break; 
	    case U_BOUNDARY_NEUTRAL: printf("BOUNDARY_NEUTRAL"); break; 
	    default: printf("Unknown Dir\n");
	    }
	  printf("<BR>\r\n");
	  switch(u_charCellWidth(theChar))
	    {
	    case U_ZERO_WIDTH: printf("ZERO_WIDTH"); break; 
	    case U_HALF_WIDTH: printf("HALF_WIDTH"); break; 
	    case U_FULL_WIDTH: printf("FULL_WIDTH"); break; 
	    case U_NEUTRAL_WIDTH: printf("NEUTRAL"); break; 
	    }
	  printf("<BR>");
	  if(searchedFor)
	    printf("<B>");
	  printType(u_charType(theChar));
	  if(searchedFor)
	    printf("</B>");

	  printf("<BR>");

	  printf("</TD>");

	  
	  /* script */
	  printf("<TD ");
	  searchedFor = (u_charScript(theChar) == gSearchScript);

	  if(searchedFor)
	    printf(" BGCOLOR=\"#EE0000\" "); /* mark the one we were searching for */

	  printf(" >");

	  if(searchedFor)
	    printf("<B>");

	  printScript(u_charScript(theChar));

	  if(searchedFor)
	    printf("</B>");

	  printf("</TD>");

	  printf("<TD>");
	  printCharName(theChar);
	  printf("</TD>");
	  printf("<TD><FONT SIZE=-1>U+%06X</FONT>", theChar);
          
          /*
              -- this works but it's kind of RUDE. -- [to link in w/o asking]
              
              printf("<IMG WIDTH=32 HEIGHT=32 SRC=\"http://charts.unicode.org/Unicode.charts/Small.Glyphs/%02X/U%04X.gif\">\r\n",
                 ((theChar&0x1FFF00)>>8),
                 theChar);

#error you're rude
          */

          printf("</TD></TR>");
	}
      

      printf("</TABLE></TT><HR>");

      showSearchMenu( block + 0x0010);

    }

#ifdef RADICAL_LIST
  else if(mode == ERADLST)
    {
      printf("<TABLE BORDER=1>");
      printf("<TR>");
      for(i=0;gKangXiRadicalTable[i];i+=2)
	{
	  if((i%16) == 0)
	    printf("</TR><TR>\r\n");
	  printf("<TD>");
	  printf("<A HREF=\"?radical=%d\">", (i/2)+1);
	  printBasicBlockInfo(gKangXiRadicalTable[i]);
	  printf("</A>");
	  printf("</TD>");

	}
      printf("</TR></TABLE>");
    }
  else if(mode == ERADICAL)
    {
      FILE *f;
      int u,stroke;
      char s[200];


      printf("<B>");
      printBasicBlockInfo(gKangXiRadicalTable[(block-1)*2]);
      printf(" (radical %d)</B> \r\n",block);
      printCharName(gKangXiRadicalTable[(block-1)*2]);
      sprintf(s,"/home/srl/icu/data/PSI/rad%03d.lst", block);
      printf("<TABLE BORDER=1>");
      f = fopen(s,"r");
      i=0;
      printf("<TR>");
      while(fscanf(f,"%06X\t%d", &u, &stroke) == 2)
	{
	  if(!( (i++)%10))
	    printf("</TR><TR>");
	  printf("<TD>");
	  printf("%d ", stroke);
	  printf("<A HREF=\"?k1=%06X#here\">", u);
	  printBasicBlockInfo(u);
	  printf("</A></TD>");
	}
      fclose(f);
      printf("</TR></TABLE>");
    }
#endif /* RADICAL_LIST */
  else if(mode == ENAME)
    {
      UChar32 c;
      char *p;
      char buf[512];
      UErrorCode status;

      for(p=gSearchName;*p;p++)
	*p = toupper(*p);

      /* "Be careful what you search for, you just might find it"
	 (and 0x10FFFF of it's close friends!)
      */

      printf("<H1>Searching for '%s'</H1>...\r\n", gSearchName);
      
      printf("<HR><UL>\r\n");

      u_enumCharNames(UCHAR_MIN_VALUE, UCHAR_MAX_VALUE,
                      myEnumCharNamesFn,
                      NULL,
                      U_UNICODE_CHAR_NAME,
                      &status);

      printf("</UL><HR>\r\n");
      showSearchMenu( 0x0000 ); 
    }
  else if(mode == EEXACTNAME)
    {
      UChar32 c;
      char *p;
      char buf[512];
      UErrorCode status = U_ZERO_ERROR;

      for(p=gSearchName;*p;p++)
        {
          *p = toupper(*p);
          if(*p == '+')
            *p = ' ';
        }

      printf("<H1>Searching for '%s'</H1>...\r\n", gSearchName);
      
      printf("<HR><UL>\r\n");

      c = u_charFromName(U_UNICODE_CHAR_NAME,
                         gSearchName,
                         &status);

      if(U_FAILURE(status))
        {
          printf("Not found - %s %d<P>\n", u_errorName(status), status);
        }
      else
        {
          printf("<LI><A HREF=\"?k1=%06X#here\"><TT>%06X</TT> - %s\r\n",
                c,c,gSearchName);
          printBasicBlockInfo(c);

        }

      printf("</UL><HR>\r\n");
      showSearchMenu( 0x0000 ); 
    }
  else
    {
      printf("Unknown search mode ! <HR>\r\n");
    }
  

  printf("</td></tr></table>\r\n\r\n");



  if(anyDecompose)
    {
      printf("Note: text in <FONT COLOR=\"#00DD00\">");
      printf("<I>Italic Green</I></FONT> is the best-try using decomposition. Underscore (_) denotes missing characters in the decomposition.<P>");

    }

  u_getUnicodeVersion(uvi);
  printf("<A HREF=\"http://www.unicode.org/\">Based on: ");
  for(uc=0;uc<U_MAX_VERSION_LENGTH;uc++) {
      printf("%d.",uvi[uc]);
  }
  printf("</A>\r\n");

  printf("<BR>Powered by <A HREF=\"http://oss.software.ibm.com/icu/\">ICU %s</A>\r\n", U_ICU_VERSION);
  
  printf("</td></tr></table>\r\n");

  printf("</BODY></HTML>\r\n");
}

/* still lazy evaluated. but it'll give us an alternate strcpy */


UConverter *gConverter = NULL;

const char *
up_u2c(char *buf, const UChar *src, int32_t len, UErrorCode *cnvStatus)
{
  /* borrowed from udate */
  int32_t sourceLen;
  const UChar *mySource;
  const UChar *mySourceEnd;
  const UChar *myConsumed;
  char *myTarget;
  int32_t arraySize;
  UErrorCode  status = U_ZERO_ERROR;

  /*  return u_austrcpy(buf,src);*/

  status = U_ZERO_ERROR;

  if(gConverter == NULL) /* open just one of these */
    {
      /* open a default converter */
      gConverter = ucnv_open(NULL, &status);
      ucnv_setSubstChars(gConverter,"_",1,&status);  /* DECOMPOSE calls SUBSTITUTE on failure. */
      if(U_FAILURE(status))
	{
	  puts("Could not open the default converter.");
	}
    }

  /* set up the conversion parameters */
  sourceLen    = len;
  mySource     = src;
  mySourceEnd  = mySource + sourceLen;
  myTarget     = buf;
  arraySize    = 0x000FFFFF; /* should be a param */

  /* put our special error handler in */
#if 0 
/* until it is implemented for 1.6 */
  ucnv_setFromUCallBack(gConverter, &UCNV_FROM_U_CALLBACK_DECOMPOSE, &status);
#endif


  /*  ucnv_setFromUCallBack(gConverter, &SubstituteWithValueHTML, &status); */

  /* perform the conversion */

  /* perform the conversion */
  ucnv_fromUnicode(gConverter,
		   &myTarget,
		   myTarget + arraySize,
		   &mySource, 
		   mySourceEnd, 
		   NULL,
		   TRUE,
		   cnvStatus);

    /* Write the converted data to the FILE* */

  *myTarget = 0; /* tie it off */

  return buf;
}

void printType(int8_t type)
{
  switch(type)
    {
    case U_UNASSIGNED: printf("UNASSIGNED"); break; 
	    case U_UPPERCASE_LETTER: printf("UPPERCASE_LETTER"); break; 
	    case U_LOWERCASE_LETTER: printf("LOWERCASE_LETTER"); break; 
    case U_TITLECASE_LETTER: printf("TITLECASE_LETTER"); break; 
    case U_MODIFIER_LETTER: printf("MODIFIER_LETTER"); break; 
	    case U_OTHER_LETTER: printf("OTHER_LETTER"); break; 
	    case U_NON_SPACING_MARK: printf("NON_SPACING_MARK"); break; 
	    case U_ENCLOSING_MARK: printf("ENCLOSING_MARK"); break; 
	    case U_COMBINING_SPACING_MARK: printf("COMBINING_SPACING_MARK"); break; 
	    case U_DECIMAL_DIGIT_NUMBER: printf("DECIMAL_DIGIT_NUMBER"); break; 
	    case U_LETTER_NUMBER: printf("LETTER_NUMBER"); break; 
	    case U_OTHER_NUMBER: printf("OTHER_NUMBER"); break; 
	    case U_SPACE_SEPARATOR: printf("SPACE_SEPARATOR"); break; 
	    case U_LINE_SEPARATOR: printf("LINE_SEPARATOR"); break; 
	    case U_PARAGRAPH_SEPARATOR: printf("PARAGRAPH_SEPARATOR"); break; 
	    case U_CONTROL_CHAR: printf("CONTROL"); break; 
	    case U_FORMAT_CHAR: printf("FORMAT"); break; 
	    case U_PRIVATE_USE_CHAR: printf("PRIVATE_USE"); break; 
	    case U_SURROGATE: printf("SURROGATE"); break; 
	    case U_DASH_PUNCTUATION: printf("DASH_PUNCTUATION"); break; 
	    case U_START_PUNCTUATION: printf("START_PUNCTUATION"); break; 
	    case U_END_PUNCTUATION: printf("END_PUNCTUATION"); break; 
	    case U_CONNECTOR_PUNCTUATION: printf("CONNECTOR_PUNCTUATION"); break; 
	    case U_OTHER_PUNCTUATION: printf("OTHER_PUNCTUATION"); break; 
	    case U_MATH_SYMBOL: printf("MATH_SYMBOL"); break; 
	    case U_CURRENCY_SYMBOL: printf("CURRENCY_SYMBOL"); break; 
	    case U_MODIFIER_SYMBOL: printf("MODIFIER_SYMBOL"); break; 
	    case U_OTHER_SYMBOL: printf("OTHER_SYMBOL"); break; 
	    case U_INITIAL_PUNCTUATION: printf("INITIAL_PUNCTUATION"); break; 
	    case U_FINAL_PUNCTUATION: printf("FINAL_PUNCTUATION"); break; 
    case U_GENERAL_OTHER_TYPES: printf("GENRERAL_OTHER_TYPES"); break;  /* sic */
    default: printf("Unknown type %d", type); break;
    }
}

void printScript(UCharScript script)
{
  switch(script)
    {
case U_BASIC_LATIN: printf("BASIC_LATIN"); return;
case U_LATIN_1_SUPPLEMENT: printf("LATIN_1_SUPPLEMENT"); return;
case U_LATIN_EXTENDED_A: printf("LATIN_EXTENDED_A"); return;
case U_LATIN_EXTENDED_B: printf("LATIN_EXTENDED_B"); return;
case U_IPA_EXTENSIONS: printf("IPA_EXTENSIONS"); return;
case U_SPACING_MODIFIER_LETTERS: printf("SPACING_MODIFIER_LETTERS"); return;
case U_COMBINING_DIACRITICAL_MARKS: printf("COMBINING_DIACRITICAL_MARKS"); return;
case U_GREEK: printf("GREEK"); return;
case U_CYRILLIC: printf("CYRILLIC"); return;
case U_ARMENIAN: printf("ARMENIAN"); return;
case U_HEBREW: printf("HEBREW"); return;
case U_ARABIC: printf("ARABIC"); return;
case U_SYRIAC: printf("SYRIAC"); return;
case U_THAANA: printf("THAANA"); return;
case U_DEVANAGARI: printf("DEVANAGARI"); return;
case U_BENGALI: printf("BENGALI"); return;
case U_GURMUKHI: printf("GURMUKHI"); return;
case U_GUJARATI: printf("GUJARATI"); return;
case U_ORIYA: printf("ORIYA"); return;
case U_TAMIL: printf("TAMIL"); return;
case U_TELUGU: printf("TELUGU"); return;
case U_KANNADA: printf("KANNADA"); return;
case U_MALAYALAM: printf("MALAYALAM"); return;
case U_SINHALA: printf("SINHALA"); return;
case U_THAI: printf("THAI"); return;
case U_LAO: printf("LAO"); return;
case U_TIBETAN: printf("TIBETAN"); return;
case U_MYANMAR: printf("MYANMAR"); return;
case U_GEORGIAN: printf("GEORGIAN"); return;
case U_HANGUL_JAMO: printf("HANGUL_JAMO"); return;
case U_ETHIOPIC: printf("ETHIOPIC"); return;
case U_CHEROKEE: printf("CHEROKEE"); return;
case U_UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS: printf("UNIFIED_CANADIAN_ABORIGINAL_SYLLABICS"); return;
case U_OGHAM: printf("OGHAM"); return;
case U_RUNIC: printf("RUNIC"); return;
case U_KHMER: printf("KHMER"); return;
case U_MONGOLIAN: printf("MONGOLIAN"); return;
case U_LATIN_EXTENDED_ADDITIONAL: printf("LATIN_EXTENDED_ADDITIONAL"); return;
case U_GREEK_EXTENDED: printf("GREEK_EXTENDED"); return;
case U_GENERAL_PUNCTUATION: printf("GENERAL_PUNCTUATION"); return;
case U_SUPERSCRIPTS_AND_SUBSCRIPTS: printf("SUPERSCRIPTS_AND_SUBSCRIPTS"); return;
case U_CURRENCY_SYMBOLS: printf("CURRENCY_SYMBOLS"); return;
case U_COMBINING_MARKS_FOR_SYMBOLS: printf("COMBINING_MARKS_FOR_SYMBOLS"); return;
case U_LETTERLIKE_SYMBOLS: printf("LETTERLIKE_SYMBOLS"); return;
case U_NUMBER_FORMS: printf("NUMBER_FORMS"); return;
case U_ARROWS: printf("ARROWS"); return;
case U_MATHEMATICAL_OPERATORS: printf("MATHEMATICAL_OPERATORS"); return;
case U_MISCELLANEOUS_TECHNICAL: printf("MISCELLANEOUS_TECHNICAL"); return;
case U_CONTROL_PICTURES: printf("CONTROL_PICTURES"); return;
case U_OPTICAL_CHARACTER_RECOGNITION: printf("OPTICAL_CHARACTER_RECOGNITION"); return;
case U_ENCLOSED_ALPHANUMERICS: printf("ENCLOSED_ALPHANUMERICS"); return;
case U_BOX_DRAWING: printf("BOX_DRAWING"); return;
case U_BLOCK_ELEMENTS: printf("BLOCK_ELEMENTS"); return;
case U_GEOMETRIC_SHAPES: printf("GEOMETRIC_SHAPES"); return;
case U_MISCELLANEOUS_SYMBOLS: printf("MISCELLANEOUS_SYMBOLS"); return;
case U_DINGBATS: printf("DINGBATS"); return;
case U_BRAILLE_PATTERNS: printf("BRAILLE_PATTERNS"); return;
case U_CJK_RADICALS_SUPPLEMENT: printf("CJK_RADICALS_SUPPLEMENT"); return;
case U_KANGXI_RADICALS: printf("KANGXI_RADICALS"); return;
case U_IDEOGRAPHIC_DESCRIPTION_CHARACTERS: printf("IDEOGRAPHIC_DESCRIPTION_CHARACTERS"); return;
case U_CJK_SYMBOLS_AND_PUNCTUATION: printf("CJK_SYMBOLS_AND_PUNCTUATION"); return;
case U_HIRAGANA: printf("HIRAGANA"); return;
case U_KATAKANA: printf("KATAKANA"); return;
case U_BOPOMOFO: printf("BOPOMOFO"); return;
case U_HANGUL_COMPATIBILITY_JAMO: printf("HANGUL_COMPATIBILITY_JAMO"); return;
case U_KANBUN: printf("KANBUN"); return;
case U_BOPOMOFO_EXTENDED: printf("BOPOMOFO_EXTENDED"); return;
case U_ENCLOSED_CJK_LETTERS_AND_MONTHS: printf("ENCLOSED_CJK_LETTERS_AND_MONTHS"); return;
case U_CJK_COMPATIBILITY: printf("CJK_COMPATIBILITY"); return;
case U_CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A: printf("CJK_UNIFIED_IDEOGRAPHS_EXTENSION_A"); return;
case U_CJK_UNIFIED_IDEOGRAPHS: printf("CJK_UNIFIED_IDEOGRAPHS"); return;
case U_YI_SYLLABLES: printf("YI_SYLLABLES"); return;
case U_YI_RADICALS: printf("YI_RADICALS"); return;
case U_HANGUL_SYLLABLES: printf("HANGUL_SYLLABLES"); return;
case U_HIGH_SURROGATES: printf("HIGH_SURROGATES"); return;
case U_HIGH_PRIVATE_USE_SURROGATES: printf("HIGH_PRIVATE_USE_SURROGATES"); return;
case U_LOW_SURROGATES: printf("LOW_SURROGATES"); return;
case U_PRIVATE_USE_AREA /* PRIVATE_USE */: printf("PRIVATE_USE_AREA /* PRIVATE_USE */"); return;
case U_CJK_COMPATIBILITY_IDEOGRAPHS: printf("CJK_COMPATIBILITY_IDEOGRAPHS"); return;
case U_ALPHABETIC_PRESENTATION_FORMS: printf("ALPHABETIC_PRESENTATION_FORMS"); return;
case U_ARABIC_PRESENTATION_FORMS_A: printf("ARABIC_PRESENTATION_FORMS_A"); return;
case U_COMBINING_HALF_MARKS: printf("COMBINING_HALF_MARKS"); return;
case U_CJK_COMPATIBILITY_FORMS: printf("CJK_COMPATIBILITY_FORMS"); return;
case U_SMALL_FORM_VARIANTS: printf("SMALL_FORM_VARIANTS"); return;
case U_ARABIC_PRESENTATION_FORMS_B: printf("ARABIC_PRESENTATION_FORMS_B"); return;
case U_SPECIALS: printf("SPECIALS"); return;
case U_HALFWIDTH_AND_FULLWIDTH_FORMS: printf("HALFWIDTH_AND_FULLWIDTH_FORMS"); return;
  /*case U_CHAR_SCRIPT_COUNT: printf("SCRIPT_COUNT"); return; */
case U_NO_SCRIPT: printf("NO_SCRIPT"); return;

    default: printf("Unknown script %d", script); return;
    }
}

UChar32 doSearchScript(int32_t script, UChar32 startFrom)
{
  UChar32 end = (startFrom-1);
  for(;startFrom != end; startFrom++)
  {
    if(startFrom > UCHAR_MAX_VALUE)
      startFrom = UCHAR_MIN_VALUE;
    if(u_charScript(startFrom) == script)
      return startFrom;
  }

  return startFrom;
}

UChar32 doSearchType(int8_t type, UChar32 startFrom)
{
  UChar32 end = (startFrom-1);
  for(;startFrom != end; startFrom++)
  {
    if(startFrom > UCHAR_MAX_VALUE)
      startFrom = UCHAR_MIN_VALUE;

    if(u_charType(startFrom) == type)
      return startFrom;
  }

  return startFrom;
}

void showSearchMenu(UChar32 startFrom)
{
  int32_t i;

  printf("<table border=0 cellpadding=0 cellspacing=0 width=100%><tr><td bgcolor=\"#000000\">\r\n"
         "<table border=0 cellpadding=1 cellspacing=1 width=100%><tr><td bgcolor=\"#cccccc\">\r\n"
         "\r\n");

  printf("<b>Search</b><br>\r\n"
         "\r\n"
         "</td></tr><tr><td bgcolor=\"#eeeeee\">\r\n"
         "<table border=0><tr><td>\r\n"
         "\r\n");

  printf("By Script: <FORM METHOD=GET>");
  printf("<SELECT NAME=scr>\r\n");
  for(i=U_BASIC_LATIN;i<=U_CHAR_SCRIPT_COUNT;i++)
    {
      printf("  <OPTION ");
      if(i == gSearchScript)
	printf(" SELECTED ");
      printf(" VALUE=\"%d\">", i);
      printScript(i);
      printf("\r\n");
    }
  printf("</SELECT>\r\n");
  printf("<INPUT TYPE=hidden NAME=b VALUE=%06X>\r\n", startFrom);
  printf("<INPUT TYPE=SUBMIT VALUE=\"Search\">");
  printf("</FORM>\r\n");

  printf("By Type: <FORM METHOD=GET>");
  printf("<SELECT NAME=typ>\r\n");
  for(i=U_UNASSIGNED;i<=U_GENERAL_OTHER_TYPES;i++)
    {
      printf("  <OPTION ");
      if(i == gSearchType)
	printf(" SELECTED ");
      printf(" VALUE=\"%d\">", i);
      printType(i);
      printf("\r\n");
    }
  printf("</SELECT>\r\n");
  printf("<INPUT TYPE=hidden NAME=b VALUE=%06X>\r\n", startFrom);
  printf("<INPUT TYPE=SUBMIT VALUE=\"Search\">");
  printf("</FORM>\r\n");

  printf("By Charname: <FORM METHOD=GET><INPUT NAME=s><INPUT TYPE=SUBMIT VALUE=\"Search\"></FORM>\r\n");

  printf("By Exact Name: <FORM METHOD=GET><INPUT NAME=sx><INPUT TYPE=SUBMIT VALUE=\"Search\"></FORM>\r\n");

#ifdef RADICAL_LIST
  printf("<A HREF=\"?radlst=1\">Radicals</A><P>");
#endif
  
  printf("\r\n"
         "</td></tr></table>\r\n"
         "</td></tr></table>\r\n");

}

