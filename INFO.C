/* -------------------------------------------------------------------- */
/*  INFO.C                        ACit                         91Sep26  */
/* -------------------------------------------------------------------- */
/*  This module deals with the fileinfo.cits and the .ri commands       */
/* -------------------------------------------------------------------- */

/* -------------------------------------------------------------------- */
/*  Includes                                                            */
/* -------------------------------------------------------------------- */
#include <string.h>
#include "ctdl.h"
#include "prot.h"
#include "glob.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/* $updateFile()    Update or add file to fileinfo.cit                  */
/* $newFile()       Add a new file to the fileinfo.cit                  */
/*  entercomment()  Update/add comment, high level (assumes cur room)   */
/*  setfileinfo()   menu level .as routine sets entry to aide's name    */
/*                  if none present or leaves original uploader         */
/* $getInfo()       get infofile slot for this file (current room)      */
/* -------------------------------------------------------------------- */

static void  updateFile(const char  *dir,const char  *file,
                        const char  *user,const char  *comment);
static void  newFile(const char  *file,const struct  fInfo *ours);
static unsigned char  getInfo(const char  *file,struct  fInfo *ours);

/* -------------------------------------------------------------------- */
/*  updateFile()    Update or add file to fileinfo.cit                  */
/* -------------------------------------------------------------------- */
static void updateFile(const char *dir, const char *file,
                       const char *user, const char *comment)
{
    struct fInfo info, ours;
    FILE *fl;
    char path[80];
    BOOL found = FALSE;
    long pos;

    /* setup the buffer for write */
    strcpy(ours.fn,       file);
    strncpy(ours.uploader, user, sizeof(ours.uploader));
    ours.uploader[sizeof(ours.uploader)-1] = '\0';
    strcpy(ours.comment,  comment);
    
    sprintf(path, "%s\\fileinfo.cit", dir);

    if ((fl = fopen(path, "r+b")) == NULL)
    {
        newFile(path, &ours);
        return;
    }

    while(fread(&info, sizeof(struct fInfo), 1, fl) == 1 && !found)
    {
        if (strcmpi(file, info.fn) == SAMESTRING)
        {
            /* seek back and overwrite it */
            pos = ftell(fl);
            pos -= (sizeof(struct fInfo));
            fseek(fl, pos, SEEK_SET);
            fwrite(&ours, sizeof(struct fInfo), 1, fl);
            
            found = TRUE;
        }
    }

    fclose(fl);

    if (!found)
    {
        newFile(path, &ours);
    }
}

/* -------------------------------------------------------------------- */
/*  newFile()       Add a new file to the fileinfo.cit                  */
/* -------------------------------------------------------------------- */
static void newFile(const char *file, const struct fInfo *ours)
{
    FILE *fl;

    if ((fl = fopen(file, "ab")) == NULL)
    {
        return; /* hu? */
    }

    fwrite(ours, sizeof(struct fInfo), 1, fl);

    fclose(fl);
}


/* -------------------------------------------------------------------- */
/*  entercomment()  Update/add comment, high level (assumes cur room)   */
/* -------------------------------------------------------------------- */
void entercomment(const char *filename, const char *uploader,
                  const char *comment)
{
    updateFile(roomBuf.rbdirname, filename, uploader, comment);
}

/* -------------------------------------------------------------------- */
/*  setfileinfo()   menu level .as routine sets entry to aide's name    */
/*                  if none present or leaves original uploader         */
/* -------------------------------------------------------------------- */
void setfileinfo(void)
{
    label filename;
    label uploader;
    char  comments[64];
    char  path[80];
    struct fInfo old;

    getNormStr("filename", filename, NAMESIZE, ECHO);

    sprintf(path, "%s\\%s", roomBuf.rbdirname, filename);

    /* no bad file names */
    if (checkfilename(filename,0) == ERROR)
    {
        mPrintf("\n No file %s.\n ", filename);
        return;
    }

    /* no file name? */
    if (!filexists(path))
    {
        mPrintf("\n No file %s.\n ", filename);
        return;
    }

    if (!getInfo(filename, &old))
    {
        strcpy(uploader, logBuf.lbname);
    } else {
        strcpy(uploader, old.uploader);
    }
   
    getString("comments", comments, 64, FALSE, TRUE, "");

    entercomment(filename, uploader, comments);

    sprintf(msgBuf->mbtext, "File info changed for file %s by %s",
            filename, logBuf.lbname);

    trap(msgBuf->mbtext, T_AIDE);
}

/* -------------------------------------------------------------------- */
/*  getInfo()       get infofile slot for this file (current room)      */
/* -------------------------------------------------------------------- */
static BOOL getInfo(const char *file, struct fInfo *ours)
{
    struct fInfo info;
    FILE *fl;
    char path[80];
    BOOL found = FALSE;

    sprintf(path, "%s\\fileinfo.cit", roomBuf.rbdirname);

    if ((fl = fopen(path, "rb")) == NULL)
    {
        return FALSE;
    }

    while(fread(&info, sizeof(struct fInfo), 1, fl) == 1 && !found)
    {
        if (strcmpi(file, info.fn) == SAMESTRING)
        {
            *ours = info;
            
            found = TRUE;
        }
    }

    fclose(fl);

    return found;
}
