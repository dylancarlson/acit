/* -------------------------------------------------------------------- */
/*  OUT.C                         ACit                         91Sep27  */
/*                           Output functions                           */
/* -------------------------------------------------------------------- */

#include <conio.h>
#include <string.h>
#include <stdarg.h>
#include "ctdl.h"
#include "prot.h"
#include "glob.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* $getWord()       Gets the next word from the buffer and returns it   */
/*  mFormat()       Outputs a string to modem and console w/ wordwrap   */
/*  putWord()       Writes one word to modem and console, w/ wordwrap   */
/*  termCap()       Does a terminal command                             */
/* $ansiCode()      adds the escape if needed                           */
/*  doBS()          does a backspace to modem & console                 */
/*  doCR()          does a return to both modem & console               */
/*  dospCR()        does CR for entry of initials & pw                  */
/* $doTAB()         prints a tab to modem & console according to flag   */
/*  echocharacter() echos bbs input according to global flags           */
/*  oChar()         is the top-level user-output function (one byte)    */
/* $updcrtpos()     updates crtColumn according to character            */
/*  mPrintf()       sends formatted output to modem & console           */
/*  cPrintf()       send formatted output to console                    */
/*  cCPrintf()      send formatted output to console, centered          */
/*  prtList()       Print a list of rooms, etc.                         */
/* -------------------------------------------------------------------- */

static int getWord(char *dest, char *source, int offset, int lim);
static void  ansiCode(const char  *str);
static void doTAB(void);
static void updcrtpos(char c);

/* -------------------------------------------------------------------- */
/*  Static data                                                         */
/* -------------------------------------------------------------------- */
static char buff[512];

/************************************************************************/
/*      getWord() fetches one word from buffer                          */
/************************************************************************/
static int getWord(char *dest, char *source, int offset, int lim)
{
    int i;
#if 0
    int j;
#else
    char *source_offset = &(source[offset]);
#endif

    /* skip leading blanks & tabs & newlines if any  */
    for (i = 0;
     (  (*source_offset ==' ')
     || (*source_offset =='\t')
     || (*source_offset =='\n'))
        && (i < lim);  i++, source_offset++);

    /* step over word              */
    for ( ; ((*source_offset != ' ')
         &&  (*source_offset != '\t')
         &&  (*source_offset != '\n'))
         && (i < lim) && *source_offset; i++, source_offset++);

#if 1
    /* no trailing ^A's */
    if (*source_offset == '\1')
    {
        if (source_offset[1] == '\0')     /* string ends with ^A, naughty */
            *source_offset = '\0';        /* so change to nul */
        i--;                         /* don't allow word to end with ^A */
    }
#endif

    /* copy word over */
#if 1
    memcpy(dest, &(source[offset]), i);
    dest[i] = '\0';        /* null to tie off string */
#else
    for (j  = 0; j < i; j++)
        dest[j] = (char)(source[offset+j]);    /* & 0x7F */
    dest[j] = 0;        /* null to tie off string */
#endif

    return(offset+i);
}

/* -------------------------------------------------------------------- */
/*  mFormat()       Outputs a string to modem and console w/ wordwrap   */
/* -------------------------------------------------------------------- */
#define MAXWORD 256     /* maximum length of a word */
void mFormat(char *string)
{
    static char wordBuf[MAXWORD + 8];
    int  i;

    for (i = 0;  string[i] && 
    (outFlag == OUTOK || outFlag == IMPERVIOUS || outFlag == OUTPARAGRAPH); )
    {
        i = getWord(wordBuf, string, i, MAXWORD);
        putWord(wordBuf);
        if (mAbort()) return;
    }
}

/************************************************************************/
/*      putWord() writes one word to modem & console                    */
/************************************************************************/
void putWord(const char *st)
{                         
    const char *s;
    int  newColumn;
    char paragraph = FALSE;    
    static char prev;

    setio(whichIO, echo, outFlag);
    
    for (newColumn = crtColumn, s = st;  *s; s++)  
    {
      /*
       * if      (*s != '\t') ++newColumn;
       * else if (*s == '\t') while ((++newColumn % 8) != 1);
       */

        if (*s == '\t')      while ((++newColumn % 8) != 1);
        else if (*s == 1)    --newColumn;     /* ANSI codes*/
        else                 ++newColumn;
 
        if ((prev == '\n') && (*s == ' '))  paragraph = TRUE;
        prev = *s;
    }

    if (newColumn >= (termWidth + 1)) 
    {
        if (!paragraph) 
        {
            while  ((*st == ' ' ) || (*st == '\t') || (*st == '\n')) st++;
            doCR();
        }
    }

    for ( ; *st; st++)
    {
        if (*st == 1)                   /* CTRL-A>nsi           */
        {
            st++;
            termCap(*st);
            continue;
        }

        /* worry about words longer than a line:   */
        if (crtColumn >= termWidth)  doCR();

        if (prevChar != '\n'  ||  (*st > ' '))   oChar(*st);
        else
        {
            /* end of paragraph: */
            if (outFlag == OUTPARAGRAPH)  
            {
                outFlag = OUTOK;
            }
            doCR();
            oChar(*st);
        }
    }
}

/* -------------------------------------------------------------------- */
/*  termCap()       Does a terminal command                             */
/* -------------------------------------------------------------------- */
void termCap(char c)
{
#if 1
    struct ansistuff
    {
        char *code;
        uchar ornum;
    };
    static struct ansistuff lower[8] =
    { {   "30m", 0  },
      {   "31m", 4  },
      {   "32m", 2  },
      {   "33m", 6  },
      {   "34m", 1  },
      {   "35m", 5  },
      {   "36m", 3  },
      {   "37m", 7  } };
    static struct ansistuff upper[8] =
    { {   "40m", 0x00  },
      {   "41m", 0x40  },
      {   "42m", 0x20  },
      {   "43m", 0x60  },
      {   "44m", 0x10  },
      {   "45m", 0x50  },
      {   "46m", 0x30  },
      {   "47m", 0x70  } };



    if (!ansiOn) return;

    setio(whichIO, echo, outFlag);

    if (c >= 'a' && c <= 'h')
    {
        c -= 'a';
        ansiCode(lower[c].code);
        ansiattr &= 0xF8;
        ansiattr |= lower[c].ornum;
    }
    else if (c >= 'A' && c <= 'H')
    {
        c -= 'A';
        ansiCode(upper[c].code);
        ansiattr &= 0x8F;
        ansiattr |= upper[c].ornum;
    }
    else
#endif
                                  
    switch (c)
    {
    case TERM_NORMAL:
        ansiCode("0m");
        ansiattr = cfg.attr;
        break;
    case TERM_BLINK:
        ansiCode("5m");
        ansiattr |= '\x80';
        break;
    case TERM_REVERSE:
        ansiCode("7m");
        ansiattr &= '\x88';
        ansiattr |= cfg.wattr;
        break;
    case TERM_BOLD:
        ansiCode("1m");
        ansiattr |= '\x08';
        break;
    case TERM_UNDERLINE:
        ansiCode("4m");
        ansiattr &= '\x88';
        ansiattr |= cfg.uttr;
        break;
    }
}

/* -------------------------------------------------------------------- */
/*   ansiCode() adds the escape if needed                               */
/* -------------------------------------------------------------------- */
static void ansiCode(const char *str)
{
    char tmp[30], *p;

    if (!ansiOn) return;
  
    sprintf(tmp, "%c[%s", 27, str);

    p = tmp;

    while(*p)
    {
        outMod(*p);
        p++;
    }
}

/* -------------------------------------------------------------------- */
/*  doBS()          does a backspace to modem & console                 */
/* -------------------------------------------------------------------- */
void doBS(void)
{
    oChar('\b');
    oChar(' ');
    oChar('\b');
}

/* -------------------------------------------------------------------- */
/*  doCR()          does a return to both modem & console               */
/* -------------------------------------------------------------------- */
void doCR(void)
{
    static numLines = 0;

    crtColumn = 1;

    setio(whichIO, echo, outFlag);

    domcr();
    doccr();

    if (printing)
        fprintf(printfile, "\n");

    prevChar    = ' ';

    /* pause on full screen */
    if (logBuf.linesScreen)
    {
        if (outFlag == OUTOK)
        {
            numLines++;
            if (numLines == (int)logBuf.linesScreen)
            {
                outFlag = OUTPAUSE;
                mAbort();
                numLines = 0;
            }
        } else {
            numLines = 0;
        }
    } else {
        numLines = 0;
    }
}

/* -------------------------------------------------------------------- */
/*  dospCR()        does CR for entry of initials & pw                  */
/* -------------------------------------------------------------------- */
void dospCR(void)
{
    char oldecho;
    oldecho = echo;

    echo = BOTH;
    setio(whichIO, echo, outFlag);

    if (cfg.nopwecho == 1)  doCR(); 
    else
    {
        if ((whichIO == CONSOLE))
        {
            if (gotCarrier()) domcr();
        }
        else  doccr();
    }
    echo = oldecho;
}

/* -------------------------------------------------------------------- */
/*  doTAB()         prints a tab to modem & console according to flag   */
/* -------------------------------------------------------------------- */
static void doTAB(void)
{
    int column, column2;

    column  = crtColumn;
    column2 = crtColumn;

    do { outCon(' '); } while ( (++column % 8) != 1);

    if (haveCarrier)
    {
        if (termTab)           outMod('\t');
        else
        do { outMod(' '); } while ((++column2 % 8) != 1);
    }
    updcrtpos('\t');
}    

/* -------------------------------------------------------------------- */
/*  echocharacter() echos bbs input according to global flags           */
/* -------------------------------------------------------------------- */
void echocharacter(char c)
{
    setio(whichIO, echo, outFlag);

    if (echo == NEITHER)
    {
        return;
    }
    else if (c == '\b') doBS();
    else if (c == '\n') doCR();
    else                oChar(c);
}

/* -------------------------------------------------------------------- */
/*  oChar()         is the top-level user-output function (one byte)    */
/*        sends to modem port and console both                          */
/*        does conversion to upper-case etc as necessary                */
/*        in "debug" mode, converts control chars to uppercase letters  */
/*      Globals modified:       prevChar                                */
/* -------------------------------------------------------------------- */
void oChar(register char c)
{
    static int UpDoWn=TRUE;   /* You dont want to know */

#if 1
    if (!logBuf.IBMGRAPH)
        c = filt_out[(uchar)c];
#endif
    
    prevChar = c;                       /* for end-of-paragraph code    */

    if (c == 1) c = 0;

    if (c == '\t')
    {
        doTAB();
        return;
    }

    if (backout)                /* You don't want to know */
    {
        if (UpDoWn)
            c = (char)toupper(c);
        else
            c = (char)tolower(c);
        UpDoWn=!UpDoWn;
    }

    if (termUpper)      c = (char)toupper(c);

    if (c == 10 /* newline */)  c = ' ';   /* doCR() handles real newlines */

    /* show on console */
    if (console)  outCon(c);

    /* show on printer */
    if (printing)  fputc(c, printfile);

    /* send out the modem  */
    if (haveCarrier && modem) outMod(c);

    updcrtpos(c);
}

/* -------------------------------------------------------------------- */
/*  updcrtpos()     updates crtColumn according to character            */
/* -------------------------------------------------------------------- */
static void updcrtpos(char c)
{
    if (c == '\b') 
        crtColumn--;
    else if (c == '\t')
        while((++crtColumn  % 8) != 1);
    else if ((c == '\n') || (c == '\r')) crtColumn = 1;
    else crtColumn++;
}

/* -------------------------------------------------------------------- */
/*  mPrintf()       sends formatted output to modem & console           */
/* -------------------------------------------------------------------- */
void mPrintf(const char *fmt, ... )
{
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buff, fmt, ap);
    va_end(ap);

    mFormat(buff);
}

/* -------------------------------------------------------------------- */
/*  cPrintf()       send formatted output to console                    */
/* -------------------------------------------------------------------- */
void cPrintf(const char *fmt, ... )
{
    register char *buf = buff;
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buff, fmt, ap);
    va_end(ap);

    while(*buf) {
        outCon(*buf++);
    }
}

/* -------------------------------------------------------------------- */
/*  cCPrintf()      send formatted output to console, centered          */
/* -------------------------------------------------------------------- */
void cCPrintf(const char *fmt, ... )
{
    extern volatile char far * const Row;

    va_list ap;
    int i;

    va_start(ap, fmt);
    vsprintf(buff, fmt, ap);
    va_end(ap);

    i = (80 - strlen(buff)) / 2;

    strrev(buff);

    while(i--)
        strcat(buff, " ");

    strrev(buff);

    (*stringattr)(*Row, buff, cfg.attr);
}


/* -------------------------------------------------------------------- */
/*  prtList()   Print a list of rooms, ext.                             */
/* -------------------------------------------------------------------- */
void prtList(const char *item)
{
    static int  listCol;
    static int  first;
    static int  num;
    
    if (item == LIST_START || item == LIST_END)
    {
        if (item == LIST_END)
        {
            if (num)
            {
                mPrintf(".");
                doCR();
            }
        }
        listCol = 0;
        num     = 0;
        first   = TRUE;
    }
    else
    {
        num++;
        
        if (first)
        {
            first = FALSE;
        }
        else
        {
            mPrintf(", ");
        }

        if (strlen(item) + 2 + crtColumn > termWidth)
        {
            doCR();
        }

        putWord(item);
    }
}
