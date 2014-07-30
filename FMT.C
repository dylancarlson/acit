/* -------------------------------------------------------------------- */
/*  FMT.C                          ACit                        91Sep30  */
/* -------------------------------------------------------------------- */
/*  Contains string handling stuff                                      */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <string.h>
#include <stdarg.h>
#include "ctdl.h"
#include "prot.h"
#include "glob.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  sformat()       Configurable format                                 */
/*  normalizeString() deletes leading & trailing blanks etc.            */
/*  parse_it()      parse strings separated by white space              */
/*  qtext()         Consumes quoted strings and expands escape chars    */
/*  strpos()        find a character in a string                        */
/*  substr()        is string 1 in string 2?                            */
/*  u_match()       Unix wildcarding                                    */
/* $cclass()        Used with u_match()                                 */
/* -------------------------------------------------------------------- */

static char  *cclass(const char  *p,int  sub);


/* -------------------------------------------------------------------- */
/*  sformat()       Configurable format                                 */
/* -------------------------------------------------------------------- */
/*
 *  sformat, a 10 minute project
 *  by Peter Torkelson
 *
 *    passes a number of arguments this lets you make configurable things..
 *    call it with:
 *      str    a string buffer
 *      fmt    the format string
 *      val    valid % escapes
 *    here is an example:
 *      sformat(str, "Hello %n, I am %w. (%%, %c)", "nw", "you", "me");
 *    gives you:
 *      "Hello you, I am me. (%, (NULL))"
 *
 */ 
void sformat(char *str, const char *fmt, const char *val, ... )
{
    int i;

    char s[2];
    va_list ap;

    s[1] = 0;
    *str = 0;

    while(*fmt)
    {
        if (*fmt != '%')
        {
            *s = *fmt;
            strcat(str, s);
        }else{
            fmt++;
            if (*fmt == '\0')     /*  "somthing %", not nice */
                return;  
            if (*fmt == '%')      /*  %% = %                 */
            {
                *s = *fmt;
                strcat(str, s);
            }else{                      /*  it must be a % something */
                i = strpos(*fmt, val) - 1;
                if (i != -1)
                {
                    va_start(ap, val);
                    while(i--)
                        va_arg(ap, char *);

                    strcat(str, va_arg(ap, char *));
                    va_end(ap);
                }else{
                    strcat(str, "(NULL)");
                }
    
            } /* fmt == '%' */
    
        } /* first fmt == '%' */
    
        fmt++;

    } /* end while */  
}

/* -------------------------------------------------------------------- */
/*  normalizeString() deletes leading & trailing blanks etc.            */
/* -------------------------------------------------------------------- */
void normalizeString(char *s)
{
    char *pc;

    pc = s;

    /* find end of string   */
    while (*pc)  
    {
        if (*pc < ' ')   *pc = ' ';   /* zap tabs etc... */
        pc++;
    }

    /* no trailing spaces: */
    while (pc>s  &&  isspace(*(pc-1))) pc--;
    *pc = '\0';

    /* no leading spaces: */
    while (*s == ' ')
    {
        for (pc=s;  *pc;  pc++)    *pc = *(pc+1);
    }

    /* no double blanks */
    for (; *s;)  
    {
        if (*s == ' ' &&  *(s+1) == ' ')
        {
            for (pc=s;  *pc;  pc++)  *pc = *(pc+1);
        }
        else s++;
    }
}

/* -------------------------------------------------------------------- */
/*  parse_it()      routines to parse strings separated by white space  */
/* -------------------------------------------------------------------- */
/*                                                                      */
/* strategy:  called as                                                 */
/*            count = parse_it(workspace,input);                        */
/*                                                                      */
/* where workspace is a two-d char array of the form:                   */
/*                                                                      */
/* char *workspace[MAXWORD];                                            */
/*                                                                      */
/* and input is the input string to be parsed.  it returns              */
/* the actual number of things parsed.                                  */
/*                                                                      */
/* -------------------------------------------------------------------- */
int parse_it(char *words[], char input[])
{
/* states ofmachine... */
#define INWORD          0
#define OUTWORD         1
#define INQUOTES        2

/* characters */
#undef  TAB
#define TAB             9
#define BLANK           ' '
#define QUOTE           '\"'
#define QUOTE2          '\''
#define MXWORD         128

    int  i,state,thisword;

    input[strlen(input)+1] = 0;         /* double-null */

    for (state = OUTWORD, thisword = i = 0; input[i]; i++) 
    {
        switch (state) 
        {
            case INWORD:
                if (isspace(input[i])) 
                {
                    input[i] = 0;
                    state = OUTWORD;
                }
                break;
            case OUTWORD:
                if (input[i] == QUOTE || input[i] == QUOTE2) 
                {
                    state = INQUOTES;
                } else if (!isspace(input[i])) 
                {
                    state = INWORD;
                }

                /* if we are now in a string, setup, otherwise, break */

                if (state != OUTWORD) 
                {
                    if (thisword >= MXWORD)
                    {
                        return thisword;
                    }

                    if (state == INWORD) 
                    {
                        words[thisword++] = (input + i);
                    } else {
                        words[thisword++] = (input + i + 1);
                    }
                }
                break;
            case INQUOTES:
                i += qtext(input + i, input + i,input[i - 1]);
                state = OUTWORD;
                break;
        }
    }
    return thisword;
}

/* -------------------------------------------------------------------- */
/*  qtext()         Consumes quoted strings and expands escape chars    */
/* -------------------------------------------------------------------- */
int  qtext(char *buf, const char *line, char end)
{
    int index = 0;
    int slash = 0;
    char chr;

    while (line[index] != '\0' && (line[index] != end || slash != 0)) 
    {
        if (slash == 0) 
        {
            if (line[index] == '\\') 
            {
                slash = 1;
            } 
            else 
            if (line[index] == '^') 
            {
                slash = 2;
            }
            else 
            {
                *(buf++) = line[index];
            }
        } 
        else 
        if (slash == 1) 
        {
            switch (line[index]) 
            {
                default:
                    *(buf++) = line[index];
                    break;
                case 'n':                       /* newline */
                    *(buf++) = '\n';
                    break;
                case 't':                       /* tab */
                    *(buf++) = '\t';
                    break;
                case 'r':                       /* carriage return */
                    *(buf++) = '\r';
                    break;
                case 'f':                       /* formfeed */
                    *(buf++) = '\f';
                    break;
                case 'b':                       /* backspace */
                    *(buf++) = '\b';
                    break;
            }
            slash = 0;
        } 
        else /* if (slash == 2 ) */
        {
            if (line[index] == '?') 
            {
                chr = 127;
            } 
            else
#if 1
            if (isalpha(line[index]))
                chr = (char)(tolower(line[index]) - 'a' + 1);
#else
            if (line[index] >= 'A' && line[index] <= 'Z') 
            {
                chr = (char)(line[index] - 'A' + 1);
            }
            else 
            if (line[index] >= 'a' && line[index] <= 'z') 
            {
                chr = (char)(line[index] - 'a' + 1);
            }
#endif
            else 
            {
                chr = line[index];
            }

            *(buf++) = chr;
            slash = 0;
        }

        index++;
    }

    *buf = 0;
    return line[index] == end ? index + 1 : index;
}

/* -------------------------------------------------------------------- */
/*  strpos()        find a character in a string                        */
/* -------------------------------------------------------------------- */
int strpos(char ch, const char *str)
{
#if 1
    char *loc;

    loc = strchr(str, ch);
    return ((loc == NULL) ? 0 : (loc-str) + 1);
#else
    int i;
  
    for(i=0; i < (int)strlen(str); i++)
        if (ch == str[i])
            return (i+1);
    return 0;

#endif

}

/* -------------------------------------------------------------------- */
/*  substr()        is string 2 in string 1?                            */
/*                   this function is called only by partialexist()     */
/* -------------------------------------------------------------------- */
int substr(const char *str1, char *str2)
{
    label tmp;

    strcpy(tmp, str1);
    strlwr(tmp);
    strlwr(str2);

    if(strstr(tmp, str2) == NULL)
        return FALSE;
    else
        return TRUE;
}

/* -------------------------------------------------------------------- */
/*  u_match()       Unix wildcarding                                    */
/* -------------------------------------------------------------------- */
/*
 * int u_match(string, pattern)
 * char *string, *pattern;
 *
 * Match a pattern as in sh(1).
 */

#define         CMASK   0377
#undef  QUOTE
#define QUOTE   0200
#define         QMASK   (CMASK&~QUOTE)
#define         NOT     '!'     /* might use ^ */


int u_match(const char *s, char *p)
{
        register int sc, pc;

        if (s == NULL || p == NULL)
                return(0);
        while ((pc = *p++ & CMASK) != '\0') {
                sc = *s++ & QMASK;
                switch (pc) {
                case '[':
                        if ((p = cclass(p, sc)) == NULL)
                                return(0);
                        break;

                case '?':
                        if (sc == 0)
                                return(0);
                        break;

                case '*':
                        s--;
                        do {
                if (*p == '\0' || u_match(s, p))
                                        return(1);
                        } while (*s++ != '\0');
                        return(0);

                default:
            if (tolower(sc) != (tolower(pc&~QUOTE)))
                                return(0);
                }
        }
        return(*s == 0);
}

/* -------------------------------------------------------------------- */
/*  cclass()        Used with u_match()                                 */
/* -------------------------------------------------------------------- */
static char *cclass(const char *p, int sub)
{
        register int c, d, not, found;

    sub = tolower(sub);

    if ((not = *p == NOT) != 0)
                p++;
        found = not;
        do {
                if (*p == '\0')
                        return(NULL);
        c = tolower(*p & CMASK);
                if (p[1] == '-' && p[2] != ']') {
            d = tolower(p[2] & CMASK);
                        p++;
                } else
                        d = c;
                if (c == sub || c <= sub && sub <= d)
                        found = !not;
        } while (*++p != ']');
        ++p;
        return(found ? (char *)p : NULL);
}
