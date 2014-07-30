/************************************************************************/
/*  INFO2.C                       ACit                         91Sep30  */
/*                 Infofile handling routines for citadel               */
/************************************************************************/

#include <io.h>
#include <malloc.h>
#include <string.h>
#include "ctdl.h"
#include "prot.h"
#include "glob.h"

/************************************************************************/
/*                              contents                                */
/*                                                                      */
/* $addinfo()               adds an entry to fileinfo.cit               */
/* $fillinfo()              allocates buffer and reads fileinfo.cit     */
/* $infoslot()              returns slot of filename in info-buffer     */
/*  killinfo()              removes comment from fileinfo.cit           */
/*  readinfofile()          menu level .ri .rvi routine                 */
/* $showinfo()              show info-buffer according to verbose       */
/*  updateinfo()            removes all non-existant entries            */
/*  batchinfo()             prompts for comments on new files when      */
/*                          TRUE, FALSE adds null comments              */
/*  moveFile()              copy info-buffer & move file                */
/************************************************************************/

static void addinfo(const char *filename, const char *uploader,
                    const char *comment);
static void fillinfo(void);
static int infoslot(const char *filename);
static void showinfo(char verbose);

/************************************************************************/
/*      addinfo()  appends comment fileinfo.cit                         */
/************************************************************************/
static void addinfo(const char *filename, const char *uploader,
                    const char *comment)
{  
    struct fInfo info;
    FILE *fd;

    if (changedir(roomBuf.rbdirname) == -1 )  return;

    strcpy(info.fn,       filename);
    strncpy(info.uploader, uploader, 30);
    info.uploader[29] = '\0';
    strcpy(info.comment,  comment);

    if ( (fd = fopen("fileinfo.cit", "ab" )) == NULL) return;

    fwrite( &info, sizeof(*fileinfo), 1, fd);

    fclose(fd);

    changedir(cfg.homepath);
}

/************************************************************************/
/*      fillinfo()  allocates buffer and reads fileinfo.cit in ram      */
/************************************************************************/
static void fillinfo(void)
{
    FILE *fd;

    if (changedir(roomBuf.rbdirname) == -1 )
    {
        infolength = 0;             /* no infofile if can't change to dir */
        changedir(cfg.homepath);
        return;
    }

    if ( (fd = fopen("fileinfo.cit", "rb" )) == NULL)
    {
        infolength = 0;                /* 0 length if file not there */
        changedir(cfg.homepath);
        return;
    }

    infolength = (int)filelength(fileno(fd));

    if(infolength == 0)
    {
        fclose(fd);         /* if found and 0 length don't halloc */
        return;
    }                

    chkptr(fileinfo);
    fileinfo = _fcalloc (infolength, 1);

    if (fileinfo == NULL)
    {
         doCR();
         mPrintf(" Error allocating fileinfo ");
         doCR();
    }
    else  fread( fileinfo, infolength, 1, fd );

    fclose(fd);
}

/************************************************************************/
/*      infoslot()  returns slot of specified filename in info-buffer   */
/************************************************************************/
static int infoslot(const char *filename)
{
    int i;
    int numrecords;

    if (infolength == 0) return(ERROR);     /* don't try if 0 length */

    numrecords = infolength / ( sizeof(*fileinfo) );

    for (i = 0;  i < numrecords; ++i)
    {
        if (strcmpi(filename, fileinfo[i].fn) == SAMESTRING)
            return(i);
    }
    return(ERROR);
}


/************************************************************************/
/*      killinfo()  removes comment from fileinfo.cit                   */
/************************************************************************/
void killinfo(const char *filename)
{
    int slot, numslots;
    FILE *fd;

    if (changedir(roomBuf.rbdirname) == -1 )  return;

    fillinfo();

    if(infolength ==0)              /* don't make blank infofile.cit's */
    {                            /* don't hfree if 0 lenght wasn't halloc'ed */
        changedir(cfg.homepath);
        return;
    }

    numslots = (infolength) / sizeof(*fileinfo);

    slot = infoslot(filename);

    if (slot != ERROR)
    {
        if( (fd = fopen( "fileinfo.cit", "wb" )) == NULL)
        {
            changedir(cfg.homepath);
            return;
        }

        _fmemmove( &fileinfo[slot], &fileinfo[slot+1],
             (unsigned)(sizeof(*fileinfo) * (( numslots - slot) - 1)) );

        fwrite( fileinfo, (infolength - sizeof(*fileinfo)), 1, fd);

        fclose(fd);

        if((infolength - sizeof(*fileinfo)) ==0)
             unlink("fileinfo.cit");       /* remove if 0 length file */
    }

    _ffree((void *)fileinfo);        fileinfo = NULL;

    changedir(cfg.homepath);
}

/************************************************************************/
/*      readinfofile()  menu level .ri .rvi routine                     */
/************************************************************************/
void readinfofile(char verbose, char which, char reverse)
{
    label filename;

    getNormStr("", filename, NAMESIZE, ECHO);

    if (changedir(roomBuf.rbdirname) == -1 )  return;

    /* if there is no info-file, just do a normal disk directory */
    if (!filexists("fileinfo.cit"))
    {
        if (strlen(filename))  dir(filename,verbose, which, reverse);
        else                   dir("*.*",   verbose, which, reverse);
        changedir(cfg.homepath);
        return;
    }
        
    if (strlen(filename))  filldirectory(filename, TRUE, which, reverse);
    else                   filldirectory("*.*",    TRUE, which, reverse);

    /* check for matches */
    if ( !filedir[0].entry[0]) 
    {
        if (!strlen(filename)) strcpy(filename, "*.*");

        mPrintf("\n No file %s", filename);

        /* free file directory structure */
        if(filedir != NULL)
            _ffree((void *)filedir);
        filedir = NULL;

        changedir(cfg.homepath);
        return;
    }

    /* allocate & read in fileinfo buffer */
    fillinfo();

    /* display info-buffer according to verbose */
    showinfo(verbose);

    /* free file directory structure */
    if(filedir != NULL)
        _ffree((void *)filedir);
    filedir = NULL;

    /* free info-buffer */

    if(infolength != 0 /* NULL */)
        _ffree((void *)fileinfo);
    fileinfo = NULL;
}


/************************************************************************/
/*      showinfo()  display info-buffer according to verbose            */
/************************************************************************/
static void showinfo(char verbose)
{
    int i, slot;

    char comment[64];
    label uploader;
    char filename[15];
    char size[10];
    char downtime[10];
    char date[20];
    char time[20];

    outFlag = OUTOK;

    if (changedir(roomBuf.rbdirname) == -1 )  return;

    /* check for matches */
    if ( !filedir[0].entry[0]) 
    {
        mPrintf("\n No file %s", filename);
        /* go to our home-path */
        changedir(cfg.homepath);
        return;
    }

    if (!verbose)
    {
        mPrintf("Filename        Size  Comments");
        doCR();/*--------.--- -------  ------------------... */
    }
    else
    {
        doCR();
    }

    for (i = 0; 
         (filedir[i].entry[0] && (outFlag != OUTSKIP) && !mAbort() );
         ++i)
    {    

        /* get rid of asterisks */
        filedir[i].entry[0]  = ' ';
        filedir[i].entry[13] = ' ';

        sscanf( filedir[i].entry, 
                "%s %s %s %s %s",
                filename,
                date,
                size,
                downtime,
                time );

        slot = infoslot(filename);

        if (slot != ERROR)
        {
            strcpy( comment,  fileinfo[slot].comment );
            strcpy( uploader, fileinfo[slot].uploader);
        }
        else
        {
            comment[ 0] = '\0';
            uploader[0] = '\0';
        }

        if(verbose)
        {
            doCR();
            mPrintf(" %-13s %s", "Filename:", filename); 

            doCR();
            mPrintf(" %-13s %s (%s minutes to download)", "Size:",
            size, downtime);

            doCR();
            mPrintf(" %-13s %s %s", "When:", date, time);

            doCR();
            mPrintf(" %-13s %s", "Uploaded By:", uploader);

            doCR();
            mPrintf(" %-13s %s", "Comments:", comment); 
            doCR();
        }
        else
        {
            mPrintf("%-12s %7s  %s", filename, size, comment);
            doCR();       
        }
    }

    /* go to our home-path */
    changedir(cfg.homepath);
}

/************************************************************************/
/*      updateinfo()  removes all non-existant entries from fileinfo cit*/
/************************************************************************/
void updateinfo(void)
{
    int i, k, numrecords, nuke;
    char flname[15];
    FILE *fd;

    if (changedir(roomBuf.rbdirname) == -1 )  return;

    fillinfo();

    if(infolength ==0)   /* don't make blank infofile.cit's */
    {
        changedir(cfg.homepath);
        return;
    }

    numrecords = infolength / ( sizeof(*fileinfo) );

    filldirectory("*.*", TRUE, OLDaNDnEW, FALSE);

    for (i = 0;  i < numrecords; ++i )
    {
                           /* if (!filexists(fileinfo[i].fn))  */
        nuke = TRUE;
        for(k=0; filedir[k].entry[0]; k++)
        {
            filedir[k].entry[13] = ' ';
            sscanf(filedir[k].entry, "%s", flname);
            if(strcmpi(fileinfo[i].fn, flname) == SAMESTRING)
            {
                nuke = FALSE;
                break;
            }
        }
        if(nuke)
        {

            _fmemmove( &fileinfo[i], &fileinfo[i+1],
               (unsigned)(sizeof(*fileinfo) * (( numrecords - i) - 1)));
            i--;
            numrecords--;
        }
    }

    if( (fd = fopen( "fileinfo.cit", "wb" )) == NULL)
    {
        changedir(cfg.homepath);

        /* free file directory structure */
        if(filedir != NULL)
            _ffree((void *)filedir);
        filedir = NULL;

        _ffree((void *)fileinfo);         fileinfo = NULL;
        return;
    }

    fwrite( fileinfo, (numrecords * sizeof(*fileinfo)), 1, fd);

    fclose(fd);

    if((numrecords * sizeof(*fileinfo))==0)
        unlink("fileinfo.cit");

    /* free file directory structure */
    if(filedir != NULL)
        _ffree((void *)filedir);
    filedir = NULL;

    _ffree((void *)fileinfo);         fileinfo = NULL;

    changedir(cfg.homepath);
}

/************************************************************************/
/*  batchinfo() askes for comments on all files not in fileinfo.cit     */
/*              when TRUE, adds null fields when FALSE                  */
/************************************************************************/
int batchinfo(int askuser)
{
    int i, slot, total = 0;
    char comments[64];
    label uploader;
    char filename[15];
    char size[10];
    char date[20];
    char tmp[90];

    sprintf(msgBuf->mbtext, "Batch upload by %s\n", logBuf.lbname);

    if (changedir(roomBuf.rbdirname) == -1 )  return(0);

    filldirectory("*.*", TRUE, OLDaNDnEW, FALSE);

    fillinfo();

    if(askuser)
        strcpy(uploader, logBuf.lbname);
    else
        uploader[0] = '\0';

    for(i=0; filedir[i].entry[0];i++)
    {
        filedir[i].entry[13] = ' ';
        sscanf(filedir[i].entry, "%s %s %s", filename, date, size);
        slot = infoslot(filename);
        if(slot == ERROR)
        {
            if(askuser)
            {
                doCR();
                mPrintf("%-12s %7s %s", filename, size, date);
                doCR();
                getString("comments", comments, 64, FALSE, TRUE, "");
                addinfo(filename, uploader, comments);
                total++;
                if (!comments[0])
                   sprintf(tmp, " %s\n", filename);
                else
                   sprintf(tmp, " %s: %s\n", filename, comments);
                strcat(msgBuf->mbtext, tmp);

                sprintf(tmp, "Batch upload of %s in room %s]",
                              filename, roomBuf.rbname);
                trap(tmp, T_UPLOAD);
            }
            else
            {
                comments[0] = '\0';
                addinfo(filename, uploader, comments);
            }
        }
    }
    if(infolength !=0) {
        _ffree((void *)fileinfo);     fileinfo = NULL;  }

    changedir(cfg.homepath);

    /* free file directory structure */
    if(filedir != NULL)
        _ffree((void *)filedir);
    filedir = NULL;

    return total;
}

/************************************************************************/
/*      moveFile()  copy info-buffer & move file                        */
/************************************************************************/
void moveFile(void)
{
    struct fInfo info;
    FILE *fd;

    char source[20], destination[64];
    char temp[84];

    int i, slot;

    char size[10];
    char downtime[10];
    char date[20];
    char time[20];

    doCR();
    getNormStr("source filename", source, NAMESIZE, ECHO);
    if (!strlen(source)) return;

    getNormStr("destination path", destination, 64, ECHO);
    if (!strlen(destination)) return;

    if (changedir(destination) == -1 )
    {
        mPrintf("\n Invalid pathname.");
        changedir(cfg.homepath);
        return;
    }

    if (changedir(roomBuf.rbdirname) == -1 )  return;

    if ( (checkfilename(source, 0) == ERROR)
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

    sprintf(temp, "%s\\%s", destination, source);
    if (filexists(temp))
    {
        mPrintf("\n File exists."); 
        changedir(cfg.homepath);
        return;
    }

    filldirectory(source, TRUE /* verbose */, OLDaNDnEW, FALSE );

    /* check for matches */
    if ( !filedir[0].entry[0])
    {
        mPrintf("\n No file %s", source);

        /* free file directory structure */
        if(filedir != NULL)
            _ffree((void *)filedir);
        filedir = NULL;

        /* go to our home-path */
        changedir(cfg.homepath);
        return;
    }

    fillinfo();

    for (i = 0; ( filedir[i].entry[0]);  ++i)
    {    

        /* get rid of asterisks */
        filedir[i].entry[0]  = ' ';
        filedir[i].entry[13] = ' ';

        sscanf(filedir[i].entry, "%s %s %s %s %s",
        source,
        date,
        size,
        downtime,
        time );

        slot = infoslot(source);

        if (slot != ERROR)
        {
            strcpy(info.fn,       source);
            strcpy(info.uploader, fileinfo[slot].uploader);
            strcpy(info.comment,  fileinfo[slot].comment );
        }
        else
        {
            strcpy(info.fn, source);
            info.comment[ 0] = '\0';
            info.uploader[0] = '\0';
        }

    }

    /* free file directory structure */
    if(filedir != NULL)
        _ffree((void *)filedir);
    filedir = NULL;

    /* free info-buffer */
    if(infolength != 0 /* NULL */)  {
        _ffree((void *)fileinfo);       fileinfo = NULL; }

    changedir(destination);

    if ( (fd = fopen("fileinfo.cit", "ab" )) == NULL) 
    {
        mPrintf("\n Not a %s directory!\n", softname);
        return;
    }

    fwrite( &info, sizeof(*fileinfo), 1, fd);

    fclose(fd);

    if (changedir(roomBuf.rbdirname) == -1 )  return;

    /* if successful */
    if ( rename(source, temp) == 0)
    {
        sprintf(msgBuf->mbtext,
        "File %s moved to %s in %s] by %s",
        source, destination, 
        roomBuf.rbname,
        logBuf.lbname );

        trap(msgBuf->mbtext, T_AIDE);

        aideMessage();

        killinfo(source);           /* kill old entry */
    } 
    else 
    {
        mPrintf("\n Cannot move %s\n", source);
    }

    /* go to our home-path */
    changedir(cfg.homepath);
}
