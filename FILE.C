/************************************************************************/
/*  FILE.C                        ACit                         91Sep30  */
/*                  File handling routines for ctdl                     */
/************************************************************************/

#include <string.h>
#include <malloc.h>
#include <dos.h>
#include "ctdl.h"
#include "prot.h"
#include "glob.h"

/************************************************************************/
/*                          contents                                    */
/*                                                                      */
/*  ambig()                 returns true if filename is ambiguous       */
/*  ambigUnlink()           unlinks ambiguous filenames                 */
/*  attributes()            aide fn to set file attributes              */
/*  bytesfree()             returns #bytes free on current drive        */
/*  checkfilename()         returns ERROR on illegal filenames          */
/*  dir()                   very high level, displays directory         */
/*  dltime()                computes dl time from size & global rate    */
/*  dump()                  does Unformatted dump of specified file     */
/*  dumpf()                 does Formatted dump of specified file       */
/* $entrycopy()             readable struct -> directory array          */
/* $entrymake()             dos transfer struct -> readable struct      */
/*  filldirectory()         fills our directory structure               */
/*  getattr()               returns a file attribute                    */
/*  hello()                 prints random hello blurbs                  */
/*  goodbye()               prints random logout (goodbye) blurbs       */
/*  readdirectory()         menu level .rd .rvd routine                 */
/*  renamefile()            aide fn to rename a file                    */
/*  setattr()               sets file attributes                        */
/*  tutorial()              handles wildcarded helpfile dumps           */
/*  unlinkfile()            handles the .au command                     */
/************************************************************************/

#define MAXWORD 256

/* our readable transfer structure */
static struct
{
    char filename[13];
    unsigned char attribute;
    char date[9];
    char time[9];  
    long size;
} directory;

static char *devices[] =
{  "CON", "AUX", "COM1", "COM2", "COM3", "COM4", "LPT1", "PRN", "LPT2", 
   "LPT3", "NUL", "CLOCK$", NULL
};

static char *badfiles[] =
{  "LOG.DAT", "MSG.DAT", "GRP.DAT", "CONFIG.CIT", "HALL.DAT", "ROOM.DAT",
   "FILEINFO.CIT", NULL
}; 

static void entrycopy(int element, char verbose);
static void entrymake(struct find_t *file_buf);

/************************************************************************/
/*      ambig() returns TRUE if string is an ambiguous filename         */
/************************************************************************/
int ambig(const char *filename)
{
#if 1
    if (strchr(filename, '*') || strchr(filename, '?') )
        return (TRUE);
#else
    int i;

    for (i = 0; i < (int)strlen(filename); ++i)
    {
        if ( (filename[i] == '*') || (filename[i] == '?') )
        return(TRUE);
    }
#endif
    return(FALSE);
}

/************************************************************************/
/*      ambigUnlink() unlinks ambiguous filenames                       */
/************************************************************************/
int ambigUnlink(char *filename, char change)    /* filldirectory(filename) */
{
    char file[15];
    int i=0;

    if(change)
        if (changedir(roomBuf.rbdirname) == -1)
            return(0);

    filldirectory(filename, TRUE, OLDaNDnEW, FALSE);

    while(filedir[i].entry[0])
    {
        filedir[i].entry[13] = ' ';
        sscanf(filedir[i].entry, "%s ", file);
        if(file[0])
            unlink(file);
        i++;
    }

    /* free file directory structure */
    if(filedir != NULL)
    {
        _ffree((void *)filedir);        filedir = NULL;
    }

    return(i);
}

/************************************************************************/
/*      attributes() aide fn to set file attributes                     */
/************************************************************************/
void attributes(void)
{
    label filename;
    char hidden = 0, readonly = 0;
    unsigned char attr, getattr();

    doCR();
    getNormStr("filename", filename, NAMESIZE, ECHO);

    if (changedir(roomBuf.rbdirname) == -1 )  return;

    if ( (checkfilename(filename, 0) == ERROR) 
    || ambig(filename))
    {
        mPrintf("\n Invalid filename.");
        changedir(cfg.homepath);
        return;
    }

    if (!filexists(filename))
    {
        mPrintf(" File not found\n"); 
        changedir(cfg.homepath);
        return;
    }

    attr = getattr(filename);

    readonly = (char)(attr & 1);
    hidden   = (char)( (attr & 2) == 2);

    /* set readonly and hidden bits to zero */
    attr = (attr ^ readonly);
    attr = (unsigned char)(attr ^ (hidden * 2));

    readonly = (char)getYesNo("Read only", readonly);
    hidden   = (char)getYesNo("Hidden",    hidden);

    /* set readonly and hidden bits */
    attr = (attr   | readonly);
    attr = (unsigned char)(attr   | (hidden * 2));

    setattr(filename, attr);

    sprintf(msgBuf->mbtext,
    "Attributes of file %s changed in %s] by %s",
    filename,
    roomBuf.rbname,
    logBuf.lbname );

    trap(msgBuf->mbtext, T_AIDE);

    aideMessage();

    changedir(cfg.homepath);
}

/************************************************************************/
/*      bytesfree() returns # bytes free on drive                       */
/************************************************************************/
long bytesfree(void)
{
#if 1
    struct diskfree_t drive;

    if (_dos_getdiskfree(0, &drive))
        return ((long)drive.avail_clusters *
                (long)drive.sectors_per_cluster *
                (long)drive.bytes_per_sector);
    return 0L;
#else
    char path[64];
    long bytes;
    union REGS REG;

    getcwd(path, 64);

    REG.h.ah = 0x36;      /* select drive */
    REG.h.dl = (unsigned char)(path[0] - '@');
    intdos(&REG, &REG);

    bytes = (long)( (long)REG.x.cx * (long)REG.x.ax * (long)REG.x.bx); 
    return(bytes);
#endif
}

/************************************************************************/
/*      checkfilename() checks a filename for illegal characters        */
/************************************************************************/
int checkfilename(const char *filename, char xtype)
{
    char i;
    char device[10];
    char *invalid;

    invalid = " '\"/\\[]:|<>+=;,";

    if (extrn[xtype-1].ex_batch)
        invalid++;

#if 1
    if (strpbrk(invalid, filename) != NULL)  return (ERROR);
#else
    for ( s = filename; *s; ++s)
        for (s2 = invalid; *s2; ++s2)
            if ((*s == *s2) || *s < 0x20)  return(ERROR);
#endif

    for (i = 0; i <= 6 && filename[i] != '.'; ++i)
    {
        device[i] = filename[i];
    }
    device[i] = '\0';

    for (i = 0; devices[i] != NULL; ++i)
        if (strcmpi(device, devices[i]) == SAMESTRING)
            return(ERROR);

    for (i = 0; badfiles[i] != NULL; ++i)
        if (strcmpi(filename, badfiles[i]) == SAMESTRING)
            return(ERROR);

    return(TRUE);
}

/***********************************************************************/
/*     dir() highest level file directory display function             */
/***********************************************************************/
void dir(char *filename, char verbose, char which, char reverse)
{                                               /* filldirectory(filename) */
    int i;

    outFlag = OUTOK;

    /* no bad files */
/*  if (checkfilename(filename, 0) == ERROR)
    {
        mPrintf("\n No file %s", filename);
        return;
    } */

    changedir(roomBuf.rbdirname);

    /* load our directory structure according to filename */
    filldirectory(filename, verbose, which, reverse);

    if (filedir[0].entry[0])
    {
        if (verbose)  mPrintf("\n Filename     Date      Size   D/L Time"); 
        else          doCR();
    }

    /* check for matches */
    if ( !filedir[0].entry[0]) 
        mPrintf("\n No file %s", filename);

    for (i = 0;
         ( filedir[i].entry[0] && (outFlag != OUTSKIP) && !mAbort() );
         ++i)
    {    
        if(verbose)
        {
            filedir[i].entry[0] = filedir[i].entry[13];
            filedir[i].entry[13] = ' ';
            filedir[i].entry[40] = '\0'; /* cut timestamp off */
            doCR();
        }
        /* display filename */
        mPrintf(filedir[i].entry);
    }

    if (verbose && outFlag != OUTSKIP )
    {
        doCR();
        mPrintf("        %d %s    %ld bytes free", i, 
            (i == 1) ? "File" : "Files", bytesfree());
    }

    /* free file directory structure */
    if(filedir != NULL)
    {
        _ffree((void *)filedir);          filedir = NULL;
    }

    /* go to our home-path */
    changedir(cfg.homepath);
}


/***********************************************************************/
/*    dltime()  give this routine the size of your file and it will    */
/*  return the amount of time it will take to download according to    */
/*  speed                                                              */
/***********************************************************************/
double dltime(long size)
{
    double time;
    static long fudge_factor[] = { 1800L, 7200L, 14400L, 28800L, 57600L };
    /* could be more accurate */
            
    time = (double)size / (double)(fudge_factor[speed]);

    return(time);
}



/************************************************************************/
/*      dump()  does unformatted dump of specified file                 */
/*      returns ERROR if carrier is lost, or file is aborted            */
/************************************************************************/
int dump(const char *filename)
{
    FILE *fbuf;
    int c, returnval = TRUE;

    /* last itteration might have been N>exted */
    outFlag = OUTOK;
    doCR();

    if ( (fbuf = fopen(filename, "r")) == NULL)
    {
        mPrintf("\n No file %s", filename);
        return(ERROR);
    }

    /* looks like a kludge, but we need speed!! */

    while ( (c = getc(fbuf) ) != ERROR && (c != 26 /* CPMEOF */ )
    && (outFlag != OUTNEXT) && (outFlag != OUTSKIP) && !mAbort() )
    {
        if (c == '\n')  doCR();
        else            oChar((char)c);
    }

    if ( outFlag == OUTSKIP) returnval = ERROR;
    
    fclose(fbuf);

    return  returnval;
}


/************************************************************************/
/*      dumpf()  does formatted dump of specified file                  */
/*      returns ERROR if carrier is lost, or file is aborted            */
/************************************************************************/
int dumpf(const char *filename)
{
    FILE *fbuf;
    char line[MAXWORD];
    int returnval = TRUE;

    /* last iteration might have been N>exted */
    outFlag = OUTOK;
    doCR();

    if ( (fbuf = fopen(filename, "r")) == NULL)
    {
        mPrintf("\n No helpfile %s", filename);
        return(ERROR);
    }
    /* looks like a kludge, but we need speed!! */

    while ( fgets(line, MAXWORD, fbuf) && (outFlag != OUTNEXT)
    && (outFlag != OUTSKIP) && !mAbort() )
    {
        mFormat(line);
    }
    if ( outFlag == OUTSKIP) returnval = ERROR;
    
    fclose(fbuf);

    return  returnval;
}

/************************************************************************/
/*   entrycopy()                                                        */
/*   This routine copies the single readable "directory" array to the   */
/*   to the specified element of the "dir" array according to verbose.  */
/************************************************************************/
static void entrycopy(int element, char verbose)
{

    if (verbose)
    {
        sprintf( filedir[element].entry, " %-12s %s %7ld %9.2f %s " ,    
        directory.filename,
        directory.date,
        directory.size,
        dltime(directory.size),
        directory.time
    );

        if  ((directory.attribute & 2))
        filedir[element].entry[13] = '*';

    }
    else sprintf( filedir[element].entry, "%-12s ", directory.filename); 
}


/************************************************************************/
/*   entrymake()                                                        */
/*   This routine converts one filename from the entry structure to the */
/*   "directory" structure.                                             */
/************************************************************************/
static void entrymake(struct find_t *file_buf)
{
    char string[10];

    /* copy filename   */
    strcpy( directory.filename, file_buf->name);
    strlwr( directory.filename);  /* make it lower case */

    /* copy attribute  */    
    directory.attribute = file_buf->attrib;

    /* copy date       */
    getdstamp(string, file_buf->wr_date);
    strcpy(directory.date, string);

    /* copy time       */
    gettstamp(string, file_buf->wr_time);
    strcpy(directory.time, string);

    /* copy filesize   */
    directory.size = file_buf->size;
}

/************************************************************************/
/*     filldirectory()  this routine will fill the directory structure  */
/*     according to wildcard                                            */
/************************************************************************/
void filldirectory(char *filename, char verbose,
                   char which, char reverse)     /* u_match(filename) */
{
    int i, ax;
    struct find_t  file_buf;
    int filetypes;
    int strip;

    which = which; reverse = reverse;   /* not in yet */

    /* allocate the first record of the file dir structure */
    chkptr(filedir);
    filedir = _fcalloc(sizeof(*filedir), cfg.maxfiles);

    /* return on error allocating */
    if(filedir == NULL)
    {
        cPrintf("Failed to allocate FILEDIR\n");
        return;
    }    

    filetypes = /*FA_NORMAL |*/ (aide ? _A_HIDDEN : 0);

    /* keep going till it errors, which is end of directory */
    for ( ax = _dos_findfirst("*.*", filetypes, &file_buf) , i = 0;
          ax == 0;
          ax = _dos_findnext(&file_buf), ++i )
    {
        /* Only cfg.maxfiles # of files files */
        if (i == cfg.maxfiles) break;

        /* translate dos's structure to something we can read */
        entrymake(&file_buf);

        if (!strpos('.', directory.filename))
        {
            strcat(directory.filename, ".");
            strip = TRUE;
        }else{
            strip = FALSE;
        }

        /* copy "directory" to "filedir" */
        /* NO zero length filenames */

        if ( (!(directory.attribute & 16))
            /* either aide or not a hidden file */
            /*&& (aide || !(directory.attribute & 2) ) */

            /* filename match wildcard? */
            && ( u_match(directory.filename, filename) )

            /* never the volume name either */
            && !(directory.attribute & 8) 

            /* never display fileinfo.cit */              
            && (strcmpi( directory.filename, "fileinfo.cit") != SAMESTRING) )

            /* if passed, put into structure, else loop again */
        {
            if (strip)
                directory.filename[strlen(directory.filename)-1] = '\0';
            entrycopy(i, verbose);
        }else{ 
            i--;
        }

    }
    filedir[i].entry[0] = '\0';  /* tie it off with a null */

    /* alphabetical order */
    qsort(filedir, i, 90, strcmp);
}

/************************************************************************/
/*      getattr() returns file attribute                                */
/************************************************************************/
unsigned char getattr(const char _far *filename)
{
    union REGS inregs;
    union REGS outregs;

    inregs.x.dx = FP_OFF(filename);
    inregs.h.ah = 0x43;      /* CHMOD */
    inregs.h.al = 0;         /* GETATTR */

    intdos(&inregs, &outregs);

    return((unsigned char)outregs.x.cx);
}

/************************************************************************/
/*      hello()  prints random hello blurbs                             */
/************************************************************************/
void hello(void)
{
    static uchar whichHello = 0;
    char helloname[14];

    expert = TRUE;

    /* for filexists() to work properly */
    if (changedir(cfg.helppath) == -1 ) return;
 
    wraparound:

    if (whichHello == 0)
    {
        tutorial("hello.blb");
    }
    else
    {
        sprintf(helloname, "hello%d.blb", whichHello);

        if (!filexists(helloname))
        {
            whichHello = 0;
            goto wraparound;
        }
        tutorial(helloname);
    }
    expert = FALSE;

    whichHello++;
}

/************************************************************************/
/*      goodbye()  prints random goodbye blurbs                         */
/************************************************************************/
void goodbye(void)
{
    static uchar whichBye     = 0;
    static uchar ansiWhichBye = 0;
    char byename[15];

    /* for filexists() to work properly */
    if (changedir(cfg.helppath) == -1 ) return;
 
    if(ansiOn && filexists("logout.bl@"))
    {
        if(ansiWhichBye == 0)
        {
            dump("logout.bl@");
        }
        else
        {
            sprintf(byename, "logout%d.bl@", ansiWhichBye);
            if (!filexists(byename))
            {
                dump("logout.bl@");
                ansiWhichBye = 0;
            }
            else
            {
                dump(byename);
            }
        }
    }
    else
    {
        if(whichBye == 0)
        {
            dumpf("logout.blb");
        }
        else
        {
            sprintf(byename, "logout%d.blb", whichBye);
            if (!filexists(byename))
            {
                dumpf("logout.blb");
                whichBye = 0;
            }
            else
            {
                dumpf(byename);
            }
        }
    }

    if(ansiOn && filexists("logout.bl@"))
        ansiWhichBye++;
    else
        whichBye++;

    changedir(cfg.homepath);
}

/************************************************************************/
/*      readdirectory()  menu level .rd .rvd routine  HIGH level routine*/
/************************************************************************/
void readdirectory(char verbose, char which, char reverse)
{
    label filename;

    getNormStr("", filename, NAMESIZE, ECHO);
             
    if (*filename)         dir(filename, verbose, which, reverse);
    else                   dir("*.*",    verbose, which, reverse);
}

/************************************************************************/
/*      renamefile()  aide fn to rename a file                          */
/************************************************************************/
void renamefile(void)
{      
    char source[20], destination[20];

    doCR();
    getNormStr("source filename",      source, NAMESIZE, ECHO);
    if (!strlen(source)) return;

    getNormStr("destination filename", destination, NAMESIZE, ECHO);
    if (!strlen(destination)) return;

    if (changedir(roomBuf.rbdirname) == -1 )  return;

    if ( (checkfilename(source, 0) == ERROR)   
    || (checkfilename(destination, 0) == ERROR)
    || ambig(source) || ambig(destination) )
    {
        mPrintf("\n Invalid filename.");
        changedir(cfg.homepath);
        return;
    }

    if (!filexists(source))
    {
        mPrintf(" No file %s", source); 
        changedir(cfg.homepath);
        return;
    }

    if (filexists(destination))
    {
        mPrintf("\n File exists."); 
        changedir(cfg.homepath);
        return;
    }

    /* if successful */
    if ( rename(source, destination) == 0)
    {
        sprintf(msgBuf->mbtext,
        "File %s renamed to file %s in %s] by %s",
        source, destination, 
        roomBuf.rbname,
        logBuf.lbname );

        trap(msgBuf->mbtext, T_AIDE);

        aideMessage();
    }
    else mPrintf("\n Cannot rename %s\n", source);

    changedir(cfg.homepath);
}

/************************************************************************/
/*      setattr() sets file attributes                                  */
/************************************************************************/
void setattr(const char _far *filename, unsigned char attr)
{
    union REGS inregs;
    union REGS outregs;

    inregs.x.dx = FP_OFF(filename);
    inregs.h.ah = 0x43;      /* CHMOD */
    inregs.h.al = 1;         /* SET ATTR */

    inregs.x.cx = attr;      /* attribute */

    intdos(&inregs, &outregs);
}

/************************************************************************/
/*      tutorial() dumps fomatted help files                            */
/*      this routine handles wildcarding of formatted text downloads    */
/************************************************************************/
void tutorial(char *filename)    /* filldirectory(filename) */
{
    int  i;
    char temp[14];
    
    outFlag     = OUTOK;

    if (!expert)  mPrintf("\n <J>ump <N>ext <P>ause <S>top\n");
    doCR();

    if (changedir(cfg.helppath) == -1 ) return;

    /* no bad files */
    if (checkfilename(filename, 0) == ERROR)
    {
        mPrintf("\n No helpfile %s", filename);
        changedir(cfg.homepath);
        return;
    }

    if (ambig(filename))
    {
        /* fill our directory array according to filename */
        filldirectory(filename, 0, OLDaNDnEW, FALSE);

        /* print out all the files */
        for (i = 0; filedir[i].entry[0] && 
        ( dumpf(filedir[i].entry) != ERROR) ; i++);

        if ( !i) mPrintf("\n No helpfile %s", filename);

        /* free file directory structure */
        if(filedir != NULL)
            _ffree((void *)filedir);
        filedir = NULL;
    }
    else
    {
       strcpy(temp, filename);
       temp[strlen(temp)-1] = '@';

       if (filexists(temp) && ansiOn)
         dump(temp);
       else
         dumpf(filename);
    }

    /* go to our home-path */
    changedir(cfg.homepath);
}

/************************************************************************/
/*      unlinkfile()  handles .au  aide unlink                          */
/************************************************************************/
void unlinkfile(void)
{
    label filename;

    getNormStr("filename", filename, NAMESIZE, ECHO);

    if (changedir(roomBuf.rbdirname) == -1 )  return;

    if (checkfilename(filename, 0) == ERROR) 
    {
        mPrintf(" No file %s", filename); 
        changedir(cfg.homepath);
        return;
    }

    if (!filexists(filename))
    {
        mPrintf(" No file %s", filename); 
        changedir(cfg.homepath);
        return;
    }

    /* if successful */
    if ( unlink(filename) == 0)
    {
        sprintf(msgBuf->mbtext,
        "File %s unlinked in %s] by %s",
        filename,
        roomBuf.rbname,
        logBuf.lbname );

        trap(msgBuf->mbtext, T_AIDE);

        aideMessage();

        killinfo(filename);

    }
    else mPrintf("\n Cannot unlink %s\n", filename);

    changedir(cfg.homepath);
}
