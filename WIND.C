/* -------------------------------------------------------------------- */
/*  WIND.C                        ACit                         91Sep30  */
/*            Machine dependent windowing routines for Citadel          */
/* -------------------------------------------------------------------- */

#include <conio.h>
#include <dos.h>
#include <malloc.h>
#include <string.h>
#include <time.h>
#include "ctdl.h"
#include "prot.h"
#include "glob.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* $clearline()     blanks out four lines with attr                     */
/*  cls()           clears the screen                                   */
/*  connectcls()    clears the screen upon carrier detect               */
/*  cursoff()       turns cursor off                                    */
/*  curson()        turns cursor on                                     */
/*  doccr()         Do CR on console, used to not scroll the window     */
/*  gmode()         checks for monochrome card                          */
/*  help()          toggles help menu                                   */
/*  position()      positions the cursor                                */
/* $positionW()     positions the cursor                                */
/* $scroll()        scrolls window up                                   */
/*  update25()      updates the 25th line                               */
/* $updatehelp()    updates the help window                             */
/*  directstring()  Direct screen write string w/attr at row            */
/*  directchar()    Direct screen write char with attr                  */
/*  biosstring()    BIOS print string w/attr at row                     */
/*  bioschar()      BIOS print char with attr                           */
/*  outCon()        put a character out to the console                  */
/*  setscreen()     Sets videotype flag                                 */
/* $clreol()        delete to end of line                               */
/* $ansi()          handles ansi escape sequences                       */
/*  save_screen()   allocates buffer and saves screen                   */
/*  restore_screen() restores screen and frees buffer                   */
/*  ScreenFree()    Handle screen swap between screen/logo              */
/* -------------------------------------------------------------------- */

volatile uchar far * const Column = (uchar far *) 0x450;  /* BIOS column */
volatile uchar far * const Row    = (uchar far *) 0x451;  /* BIOS row */
volatile uint  far * const Cursor = (uint  far *) 0x450;  /* speed combo */

static long f10timeout;               /* when was the f10 window opened?*/
static char far *screen;      /* memory address of RAM for direct scrn I/O */
static char far *saveBuffer;  /* memory buffer for screen saves            */
static uint s_cursor;         /* static var for savescreen, restorescreen  */
static uchar lines=24;        /* how many lines on screen (minus 1)        */

static void _fastcall clearline(uchar row, uchar attr);
static void _fastcall positionW(int);
static void _pascal scroll( uchar row, uchar howmany, uchar attr);
static void updatehelp(void);
static void clreol(void);
static char _fastcall ansi(char c);


/* -------------------------------------------------------------------- */
/*      clearline()  clears four lines to attr                          */
/* -------------------------------------------------------------------- */
#pragma optimize( "lge", off )
static void _fastcall clearline(uchar row, uchar attr)
{
    attr = attr;  /* get rid of warning */
    row = row;

    /* _fastcall makes AL=row, DL=attr */
    /* This has to be the tightest assembly I've ever written */
    _asm   mov dh, al      ; Store "lines" in DH
    _asm   dec dh          ; needs to be "lines-1"
    _asm   mov cx, 4       ; Full 16-bit counter, 4 iterations
    _asm   xor bx, bx      ; Set page BH=0 (both funcs), BL=0 for next instr
    _asm   xchg bl, dl     ; Store attr in BL, set DL=0 (col#)
    /* at this point, CL=4, BL=attr, BH=DL=0, DH=lines-1 */
    _asm redo:
    _asm   push cx         ; Store counter
    _asm   mov ah, 2       ; Position cursor, uses  DL,DH,BH
    _asm   int 0x10        ; they were all set!
    _asm   mov ax, 0x0900  ; Write char+attr, uses  AL,BH,BL,CX
    _asm   mov cl, 80      ; 80 characters,  CH=0 from above (CX=4)
    _asm   int 0x10
    _asm   dec dh          ; --row
    _asm   pop cx
    _asm   loop redo       ; --counter, jump if !0
}
#pragma optimize( "", on )


/* -------------------------------------------------------------------- */
/*      cls()  clears the screen depending on carrier present or not    */
/* -------------------------------------------------------------------- */
void cls(void)
{
    /* scroll everything but kitchen sink */
    scroll(lines, 0, cfg.attr);
    positionW(0);
}

/* -------------------------------------------------------------------- */
/*      connectcls()  clears the screen upon carrier detect             */
/* -------------------------------------------------------------------- */
void connectcls(void)
{
    if (anyEcho)
    {
        cls();
    }
    update25();
}


/* -------------------------------------------------------------------- */
/*      cursoff()  make cursor disapear                                 */
/* -------------------------------------------------------------------- */
#pragma optimize( "lge", off )
void cursoff(void)
{
    _asm  mov ah, 01
    _asm  xor bh, bh
    _asm  mov cx, 0x2607
    _asm  int 0x10
}
#pragma optimize( "", on )


/* -------------------------------------------------------------------- */
/*      curson()  Put cursor back on screen checking for adapter.       */
/* -------------------------------------------------------------------- */
#pragma optimize( "lge", off )
void curson(void)
{
    _asm   mov ah, 0x0F   ; get screen mode
    _asm   int 0x10
    _asm   mov cx, 0x0607 ; cursor = 6-7
    _asm   cmp al, 7      ; mono?
    _asm   jnz colored
    _asm   shl cx, 1      ; cursor = 12-14
    _asm   dec cx         ; cursor = 12-13
    _asm colored:
    _asm   mov ah, 1      ; set cursor
    _asm   xor bh, bh
    _asm   int 0x10
}
#pragma optimize( "", on )


/* -------------------------------------------------------------------- */
/*  doccr()         Do CR on console, used to not scroll the window     */
/* -------------------------------------------------------------------- */
void doccr(void)
{ 
    if (!console || !anyEcho) return;

    if (*Row >= scrollpos)
    {
        scroll( scrollpos, 1, ansiattr);
        position( scrollpos, 0);
    }
    else 
    {
        putch('\n');
        putch('\r');
    }
}

/* -------------------------------------------------------------------- */
/*      gmode()  Check for monochrome or graphics.                      */
/* -------------------------------------------------------------------- */
#pragma optimize( "lge", off )
int gmode(void)
{
    _asm   mov ah, 0x0F    /* get screen mode */
    _asm   int 0x10
    _asm   cbw             /* video mode=AL, return */
}
#pragma optimize( "", on )


/* -------------------------------------------------------------------- */
/*      help()  this toggles our help menu                              */
/* -------------------------------------------------------------------- */
void help(void)
{
    long time();
    uchar row = *Row;
    uchar col = *Column;

    if (scrollpos == lines-(uchar)1)  /* small window */
    {
        if (row > lines-(uchar)5u)
        {
            scroll( (uchar)(lines-1), (uchar)(row - lines + 5), cfg.wattr);
#if 0
            position((uchar)(lines-5), *Column);
#endif
        }
 
        if (cfg.bios)
        {
            clearline(lines, cfg.wattr);
        }
 
        scrollpos = lines-(uchar)5u;    /* big window */

        time(&f10timeout);
    }
    else  /* big window */
    {
        clearline(lines, cfg.attr);

        scrollpos = lines-(uchar)1;    /* small window */

        time(&f10timeout);
    }
    position(row, col);
}


/* -------------------------------------------------------------------- */
/*      positionW() positions the cursor                                */
/* -------------------------------------------------------------------- */
#pragma optimize( "lge", off )
void _fastcall positionW(int cursor)
{
    cursor = cursor;  /* get rid of warning */

    /* _fastcall implies ax=cursor */
    _asm  mov dx, ax
    _asm  mov ah, 2
    _asm  xor bh, bh         ; page 0
    _asm  int 0x10
}
#pragma optimize( "", on )

/* -------------------------------------------------------------------- */
/*      position()  positions the cursor                                */
/* -------------------------------------------------------------------- */
#pragma optimize( "lge", off )
void _fastcall position(uchar row , uchar column)
{
    row = row;   column = column;   /* get rid of warnings */
    /* _fastcall implies al=row, dl=column */
    _asm  mov ah, 2
    _asm  mov dh, al
    _asm  ; mov dl, column  ; already there
    _asm  xor bh, bh         ; page 0
    _asm  int 0x10
}
#pragma optimize( "", on )


/* -------------------------------------------------------------------- */
/*      scroll()  scrolls window up from specified line                 */
/* -------------------------------------------------------------------- */
#pragma optimize( "lge", off )
static void _pascal scroll( uchar row, uchar howmany, uchar attr)
{
    _asm   mov al, howmany
    _asm   xor cx, cx
    _asm   mov dh, row
    _asm   mov dl, 79
    _asm   mov ah, 6          ;scroll window
    _asm   mov bh, attr
    _asm   int 0x10
}
#pragma optimize( "", on )


/* -------------------------------------------------------------------- */
/*      update25()  updates the 25th line according to global variables */
/* -------------------------------------------------------------------- */
void update25(void)
{
    char string[83];
    char str2[80];
    char name[30];
    char flags[10];
    char carr[5];
    uint cursor;
    int i;

    if (scrollpos == lines-(uchar)5u) updatehelp();

    if(cfg.bios)  cursoff();
    
    cursor = *Cursor;

    if (loggedIn)  
    {
         strcpy( name, logBuf.lbname);
         for (i = 0; i < ((30 - strlen(logBuf.lbname)) / 2); i++)
           strcat(name, " ");
    }
    else
    {
#if 1
         sstrftime( name, 29, " ออออ %y%b%D  %H:%M:%S ออออ ", 0l);
#else
         strcpy( name, "ออออออ Not logged in ออออออ");
#endif
    }

    if      ( justLostCarrier)  strcpy( carr, "JL");
    else if ( haveCarrier)      strcpy( carr, "CD");
    else                        strcpy( carr, "NC");

    strcpy(flags, "       ");

    if ( aide )                     flags[0] = 'A';
    if ( twit )                     flags[1] = 'T';
    if ( sysop )                    flags[5] = 'S';

    if (loggedIn)
    {
        if ( logBuf.lbflags.PERMANENT ) flags[2] = 'P';
        if ( logBuf.lbflags.UNLISTED )  flags[4] = 'U';
        if ( logBuf.lbflags.NETUSER )   flags[3] = 'N';
        if ( logBuf.lbflags.NOMAIL )    flags[6] = 'M';
    }

    sprintf( string, " %29.29s ณ%sณ%sณ%4d bpsณ%sณ%c%c%c%cณ%sณ%sณ%sณ%s",
      name,
      (whichIO == CONSOLE) ? "Console" : " Modem ",
      carr,
      bauds[speed],
      (disabled)    ? "DS" : "EN",
      (cfg.noBells) ? ' ' : '',
      (backout)     ? '' : ' ',
      (debug)       ? '่' : ' ',
      (ConLock)     ? '' : ' ',
      (cfg.noChat)  ? ((chatReq) ? "rcht" : "    " ) : 
                      ((chatReq) ? "RCht" : "Chat" ),
      (printing)    ? "Prt"  : "   ",
      (sysReq)      ? "REQ"  : "   ",
      flags
    );

    sprintf(str2, "%-79s ", string);

    (*stringattr)(lines, str2, cfg.wattr);

    positionW(cursor);

    if(cfg.bios)  curson();

}


/* -------------------------------------------------------------------- */
/*      updatehelp()  updates the help menu according to global vars    */
/* -------------------------------------------------------------------- */
static void updatehelp(void)
{
    long time(), l;
    char bigline[81];
    uchar row, col;
    uchar n_lines = lines - (uchar)4;

    if ( f10timeout < (time(&l) - (long)(60 * 2)) ) 
    {
        help();
        return;
    }

    if(cfg.bios)  cursoff();

    strcpy(bigline, "ษอออออออออออออออัอออออออออออออออัอออออออ"
                    "อออออออัอออออออออออออออัอออออออออออออออป");

    row = *Row; col = *Column;

    position(n_lines, 0);

    (*stringattr)(n_lines, bigline, cfg.wattr);

    sprintf(bigline, "บ%sณ%sณ%sณ%sณ%sบ",
          " F1 þ Shutdown ", " F2 þ Startup  " , " F3 þ Request ",
                 (anyEcho) ? " F4 þ Echo-Off " : " F4 þ Echo-On  ",
      (whichIO == CONSOLE) ? " F5  þ Modem   " : " F5  þ Console ");

    (*stringattr)(++n_lines, bigline, cfg.wattr);

    sprintf(bigline, "บ%sณ%sณ%sณ%sณ%sบ",
    " F6 þ Sysop Fn ", (cfg.noBells) ? " F7 þ Bells-On " : " F7 þ Bells-Off" ,
    " F8 þ ChatMode",  (cfg.noChat)  ? " F9 þ Chat-On  " : " F9 þ Chat-Off ",
    " F10 þ Help    ");

    (*stringattr)(++n_lines, bigline, cfg.wattr);

    strcpy(bigline, "ศอออออออออออออออฯออออออออออออออัฯออออออั"
                    "ออัออออฯอออัออัออออัอออฯัอออัอออัออออออผ");

    (*stringattr)(++n_lines, bigline, cfg.wattr);

    position( (uchar)(row >= lines-(uchar)5u ? scrollpos : row), col);

    if(cfg.bios)  curson();
}


/* -------------------------------------------------------------------- */
/*      directstring() print a string with attribute at row             */
/* -------------------------------------------------------------------- */
void directstring(unsigned int row, const char *str, uchar attr)
{
#if 0
    register int i, j, l;

    l = strlen(str);

    for(i=(row*160), j=0; j<l; i +=2, j++)
    {
      screen[i] = str[j];
      screen[i+1] = attr;
    }
#else
    char *scrn = screen + row * 160;
    for (; *str; str++)
    {
        *scrn++ = *str;
        *scrn++ = attr;
    }
#endif
}


/* -------------------------------------------------------------------- */
/*      directchar() print a char directly with attribute at row        */
/* -------------------------------------------------------------------- */
void directchar(char ch, uchar attr)
{
    int i;
    uchar row, col;

    row = *Row; col = *Column;

    i = (row*160)+(col*2);

    screen[i] = ch;
    screen[i+1] = attr;

    position( row, (unsigned char)(++col));
}


/* -------------------------------------------------------------------- */
/*      biosstring() print a string with attribute                      */
/* -------------------------------------------------------------------- */
void biosstring(unsigned int row, const char *str, uchar attr)
{
    union REGS regs;
    union REGS temp_regs;
    register int i=0;

    regs.h.ah = 9;           /* service 9, write character # attribute */
    regs.h.bl = attr;        /* character attribute                    */
    regs.x.cx = 1;           /* number of character to write           */
    regs.h.bh = 0;           /* display page                           */

    while(str[i])
    {
      position((uchar)row, (uchar)i);/* Move cursor to the correct position */
      regs.h.al = str[i];            /* set character to write     0x0900   */
      int86( 0x10, &regs, &temp_regs);
      i++;
    }
}


/* -------------------------------------------------------------------- */
/*      bioschar() print a char with attribute                          */
/* -------------------------------------------------------------------- */
#pragma optimize( "lge", off )
void bioschar(char chr, uchar attr)
{
    _asm   mov ah, 9         ; write character with attribute
    _asm   mov bl, attr
    _asm   mov al, chr
    _asm   mov cx, 1         ; only one character
    _asm   xor bh, bh        ; page 0
    _asm   int 0x10

    positionW((*Cursor) + 1);
}
#pragma optimize( "", on )

/* -------------------------------------------------------------------- */
/*  outCon()        put a character out to the console                  */
/* -------------------------------------------------------------------- */
void outCon(char c)
{
    unsigned char row, col;
    static   char escape = FALSE;

    if (!console || !anyEcho || c == 26) return;

    if (c == 7   /* BELL */  && cfg.noBells)  return;
    if (c == 27 || escape) /* ESC || ANSI sequence */
    {
        escape = ansi(c);
        return;
    }
    /* if we dont have carrier then count what goes to console */
    if (!gotCarrier()) transmitted++;

    if (c == '\n')
        doccr();
    else
    if (c == '\r')
    {
        putch(c);
    } else {
        row = *Row; col = *Column;
        if (c == '\b' || c == 7)
        {
          if (c == '\b' && col == 0 && prevChar != 10)
              position((unsigned char)(row-1),80);  
          putch(c);
        } else {
            (*charattr)(c, ansiattr);
            if (col == 79)
            {
                position(row,col);
                doccr();
            }
        }
    }
}

/* -------------------------------------------------------------------- */
/*      setscreen() set video mode flag 0 mono 1 cga                    */
/* -------------------------------------------------------------------- */
void setscreen(void)
{
#if 1
   union REGS regs;
   uchar linediffs;

   volatile uchar far * const CrtRows    = (uchar far *) 0x00400084;

   linediffs = lines-scrollpos;
   
   /* --- Determine rows --- */
   regs.h.ah = 0x12;
   regs.h.bl = 0x10;
   int86(0x10, &regs, &regs);

   if (regs.h.bl == 0x10)
        lines = (uchar)24u;
   else
        lines = *CrtRows;
   scrollpos = lines - linediffs;
#endif
   
    if(7 == gmode())
        screen = (char far *)0xB0000000L;    /* mono */
    else
    {
        screen = (char far *)0xB8000000L;    /* cga */
        outp(0x03d9, cfg.battr);        /* set border color */
    }

    if(cfg.bios)
    {
        charattr = bioschar;
        stringattr = biosstring;
    }
    else
    {
        charattr = directchar;
        stringattr = directstring;
    }
    ansiattr = cfg.attr;

}

/* -------------------------------------------------------------------- */
/* clreol() delete to end of line                                       */
/* -------------------------------------------------------------------- */
static void clreol(void)
{
    uchar col, row;
    int i;

    col = *Column; row = *Row;

    for (i=col; i < 80; i++)
        putch(' ');

    position(col, row);
}

/* -------------------------------------------------------------------- */
/* Handle ansi escape sequences                                         */
/* -------------------------------------------------------------------- */
static char _fastcall ansi(char c)
{
    static char args[20], first = FALSE;
    static uchar c_x = 0, c_y = 0;
    uchar argc, a[5];
    int i;
    uchar x, y;
    char *p;
    static char str[80];    /* used for debugging */
  
    if (c == 27 /* ESC */)
    {
        strcpy(args, "");
        first = TRUE;
        return TRUE;
    }

    if (first)
    {
        first = FALSE;
        return (c == '[') ? (char)TRUE : (char)FALSE;
    }
    
    if (isalpha(c))
    {
        i=0; p=args; argc=0;
        while(*p)
        {
            if (isdigit(*p))
            {
                char done = 0;

                a[argc]=(uchar)atoi(p);
                while(!done)
                {
                    p++;
                    if (!(*p) || !isdigit(*p))
                    done = TRUE;
                }
                argc++;
            }else p++;
        }
        switch(c)
        {
        case 'J': /* cls */
                cls();
                update25();
                break;
        case 'K': /* del to end of line */
                clreol(); 
                break;
        case 'm':
                for (i = 0; i < (int)argc; i++)
                {
                    switch(a[i])
                    {
                    case 5:
                        ansiattr = ansiattr | (uchar)128u;  /* blink */
                        break;
                    case 4:
                        ansiattr = ansiattr | (uchar)1u;    /* underline */
                        break;
                    case 7:
                        ansiattr = cfg.wattr;       /* Reverse Vido */
                        break;
                    case 0:
                        ansiattr = cfg.attr;        /* default */
                        break;
                    case 1:
                        ansiattr = cfg.cattr;       /* Bold */
                        break;
                    default:
                        break;
                    }
                }
            break;
        case 's': /* save cursor */
                c_x = *Row;  c_y = *Column;
                break;
        case 'u': /* restore cursor */
                position(c_x, c_y);
                break;
        case 'A':
                x = *Row;
                if (argc)
                    x -= a[0];
                else
                    x--;
                x = (unsigned char)(x % lines);
                position(x, *Column);
                break;
        case 'B':
                x = *Row;
                if (argc)
                    x += a[0];
                else
                    x++;
                x = (unsigned char)(x % lines);
                position(x, *Column);
                break;
        case 'D':
                y = *Column;
                if (argc)
                    y -= a[0];
                else
                    y --;
                y = (unsigned char)(y % 80);
                position(*Row, y);
                break;
        case 'C':
                y = *Column;
                if (argc)
                    y += a[0];
                else
                    y ++;
                y = (unsigned char)(y % 80);
                position(*Row, y);
                break;
        case 'f':
        case 'H':
                if (!argc)
                {
                    positionW(0);
                    break;
                }
                if (argc == 1)
                {
                    if (args[0] == ';')
                    {
                        a[1] = a[0];
                        a[0] = 1;
                    }else{
                        a[1] = 1;
                    }
                    argc = 2;
                }
                if (argc == 2 && a[0] < 25 && a[1] < 80)
                {
                    position((uchar)(a[0]-1), (uchar)(a[1]-1));
                    break;
                }
        default:
                {
                    sprintf(str, "[%s%c %d %d %d ", args, c, argc, a[0], a[1]);
                    (*stringattr)(0, str, cfg.wattr);
                }
                break;
        }
        if (debug)
        {
            sprintf(str, "[%s%c %d %d %d ", args, c, argc, a[0], a[1]);
            (*stringattr)(0, str, cfg.wattr);
        }
        return FALSE;
    }else{
        {
            i = strlen(args);
            args[i]=c;
            args[i+1]=0 /* NULL */;
        }
        return TRUE;
    }
}       

/* -------------------------------------------------------------------- */
/*  save_screen() allocates a buffer and saves the screen               */
/* -------------------------------------------------------------------- */
void save_screen(void)
{
    saveBuffer = _fcalloc(4000, sizeof(char));
    if (saveBuffer == NULL)
        return;
    _fmemmove(saveBuffer, screen + (lines-24) * 160, 4000);
    s_cursor = *Cursor;
}


/* -------------------------------------------------------------------- */
/*   restore_screen() restores screen and free's buffer                 */
/* -------------------------------------------------------------------- */
void restore_screen(void)
{
    if (saveBuffer == NULL)
        return;
                       /*end of scrn*/   /* sizeof buffer */
    _fmemmove(screen + (lines-24) * 160, saveBuffer, 4000);
    _ffree((void *)saveBuffer);          saveBuffer = NULL;
    positionW(s_cursor);

    if (gmode() != 7)                     /* if color display */
    {
        outp(0x03d9, cfg.battr);        /* set border color */
    }

}

/* -------------------------------------------------------------------- */
/* ScreenFree() either saves the screen and displays the opening logo   */
/*      or restores, depending on anyEcho                               */
/* -------------------------------------------------------------------- */
void ScreenFree(void)
{
    static uchar row, col, helpVal = 0;

    if (anyEcho)
    {
        helpVal = (scrollpos == lines-(uchar)5u)  /* the help window is open */
                  ? (uchar)1u : (uchar)0u;

        save_screen();
        row = *Row; col = *Column;
        cursoff();
        logo();

        if (helpVal)
        {
            updatehelp();
        }
    }
    else
    {
        restore_screen();
        if (helpVal == 0 && 
            scrollpos == lines-(uchar)5u) /* window opened while locked */
        {
            if (row > lines-(uchar)5u)
            {
                scroll( (uchar)(lines-1), (uchar)(row - lines + 5), cfg.wattr);
                position((uchar)(lines-5), col);
            }
            updatehelp();

        }

        if (helpVal == 1 && 
            scrollpos == lines-(uchar)1u) /* window closed while locked */
        {
            clearline(lines, cfg.attr);
        }
        position(row, col);
        curson();
    }
}
