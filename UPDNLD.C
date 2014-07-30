/************************************************************************/
/*  UPDNLD.C                      ACit                         91Sep30  */
/*                  File transfer routines for Citadel                  */
/************************************************************************/

#include <string.h>
#include <malloc.h>  /* for _ffree() */
#include <time.h>    /* for time() */
#include <io.h>      /* for filelength() */
#include "ctdl.h"
#include "prot.h"
#include "glob.h"

/************************************************************************/
/*                          contents                                    */
/*                                                                      */
/* $blocks()                displays how many blocks file is            */
/* $checkup()               returns TRUE if filename can be uploaded    */
/*  entertextfile()         menu level .et                              */
/* $hide()                  hides a file. for limited-access u-load     */
/*  readtextfile()          menu level .rt routine                      */
/* $textdown()              does wildcarded unformatted file dumps      */
/* $textup()                handles actual text upload                  */
/*  download()              handles the .d command                      */
/*  upload()                handles the .u command                      */
/* $upDownMnu()             handles .D? and .U? commands                */
/************************************************************************/

static void blocks(const char *filename, int bsize);
static int checkup(const char *filename);
static void hide(const char *filename);
static void textdown(char *filename, char verbose);
static void textup(const char *filename);
static void upDownMnu(char cmd);

/************************************************************************/
/*      blocks()  displays how many blocks file is upon download        */
/************************************************************************/
static void blocks(const char *filename, int bsize)
{
    FILE *stream;
    long length;
    int blocks;

    outFlag = OUTOK;

    stream = fopen(filename, "r");

    length = filelength(fileno(stream));

    fclose(stream);
 
    if (length == -1l) return;

    if (bsize)
      blocks = ((int)(length/(long)bsize) + 1);
    else
      blocks = 0;

    doCR();

    if (!bsize)
      mPrintf("File Size: %ld %s",
        length, (length == 1l) ? "byte" : "bytes" );
    else
      mPrintf("File Size: %d %s, %ld %s",
        blocks, (blocks == 1) ? "block" : "blocks",
        length, (length == 1l)? "byte" : "bytes" );

    doCR();
    mPrintf("Transfer Time: %.0f minutes", dltime(length));
    doCR();
}

/***********************************************************************/
/*      checkup()  returns TRUE if filename can be uploaded            */
/***********************************************************************/
static int checkup(const char *filename)
{
    if (ambig(filename) )  return(ERROR);

    /* no bad files */
    if (checkfilename(filename, 0) == ERROR)
    {
        mPrintf("\n Invalid filename.");
        return(ERROR);
    }

    if (changedir(roomBuf.rbdirname) == -1 )  return(ERROR);

    if (filexists(filename))
    {
        mPrintf("\n File exists."); 
        changedir(cfg.homepath);
        return(ERROR);
    }
    return(TRUE);
}

/************************************************************************/
/*   entertextfile()  menu level .et                                    */
/************************************************************************/
void entertextfile(void)
{
    label filename;
    char comments[64];

    doCR();
    getNormStr("filename", filename, NAMESIZE, ECHO);

    if (checkup(filename) == ERROR)  return;

    getString("comments", comments, 64, FALSE, TRUE, "");
             
    if (strlen(filename))  textup(filename);

    if (changedir(roomBuf.rbdirname) == -1 )  return;

    if (filexists(filename))
    {
        entercomment(filename, logBuf.lbname, comments);
        if (!comments[0])
          sprintf(msgBuf->mbtext, " %s uploaded by %s", filename, logBuf.lbname);
        else
          sprintf(msgBuf->mbtext, " %s uploaded by %s\n Comments: %s", 
            filename, logBuf.lbname, comments);
        specialMessage();
    }

    changedir(cfg.homepath);
}

/************************************************************************/
/*      hide()  hides a file. for limited-access u-load                 */
/************************************************************************/
static void hide(const char *filename)
{
    unsigned char attr, getattr();

    attr = getattr(filename);

    /* set hidden bit on */
    attr = (unsigned char)(attr | 2);

    setattr(filename, attr);
}

/************************************************************************/
/*      readtextfile()  menu level .rt  HIGH level routine              */
/************************************************************************/
void readtextfile(char verbose)
{
    label filename;

    doCR();
    getNormStr("filename", filename, NAMESIZE, ECHO);
             
    if (strlen(filename))  textdown(filename, verbose);
}

/************************************************************************/
/*      textdown() dumps a host file with no formatting                 */
/*      this routine handles wildcarding of text downloads              */
/************************************************************************/
static void textdown(char *filename, char verbose)
{                                 /* filldirectory(filename) */
    int i;
    int retval = 0;

    outFlag     = OUTOK;

    /* no bad files */
    if (checkfilename(filename, 0) == ERROR)
    {
        mPrintf("\n No file %s", filename);
        return;
    }

    if (changedir(roomBuf.rbdirname) == -1 ) return;


    if (ambig(filename))
    {
        /* fill our directory array according to filename */
        filldirectory(filename, 0, OLDaNDnEW, FALSE);

        /* print out all the files */
        for (i = 0; filedir[i].entry[0] && ( retval != ERROR); i++)
        {
            if (verbose)
                retval = dumpf(filedir[i].entry);
            else
                retval = dump(filedir[i].entry);
        }

        if ( !i) mPrintf("\n No file %s", filename);

        /* free file directory structure */
        if(filedir != NULL)
            _ffree((void *)filedir);
        filedir = NULL;
    }
    else
    {
        if (verbose)   dumpf(filename);
        else           dump(filename);
    }

    sprintf(msgBuf->mbtext, "Text download of file %s in room %s]",
        filename, roomBuf.rbname);

    trap(msgBuf->mbtext, T_DOWNLOAD);

    doCR();

    /* go to our home-path */
    changedir(cfg.homepath);
}

/************************************************************************/
/*      textup()  handles textfile uploads                              */
/************************************************************************/
static void textup(const char *filename)
{
    int i;

    if (!expert)  tutorial("textup.blb");

    changedir(roomBuf.rbdirname);

    doCR();

    if ((upfd = fopen( filename, "wt")) == NULL)
    {
        mPrintf("\n Can't create %s!\n", filename);
    }
    else
    {
        while(  ((i = iChar()) != 26 /* CNTRLZ */ )
                && outFlag != OUTSKIP 
                && ((whichIO == CONSOLE) || gotCarrier()) )
        {
            fputc(i, upfd);
        }
        fclose(upfd);

        sprintf(msgBuf->mbtext, "Text upload of file %s in room %s]",
        filename, roomBuf.rbname);

        if (limitFlag && filexists(filename))  
            hide(filename);

        trap(msgBuf->mbtext, T_UPLOAD);
    }

    changedir(cfg.homepath);
}

/************************************************************************/
/*      download()  menu level download routine                         */
/************************************************************************/
void download(char c)
{
    long    transTime1, transTime2;    /* to give no cost uploads       */
    char filename[81];
    char ch, xtype;
    char ich = '\0';
                
    if (!c) 
      ch=(char)tolower((ich = commandchar()));
    else
      ch = c;

    xtype = (char)strpos(ch, extrncmd);
    
    if (!xtype)
    {
        if (ch == '?')
        {
          oChar('?');
          upDownMnu('D');
        }
        else{
          oChar(ich);
          mPrintf(" ?");
          if (!expert)
            upDownMnu('D');
        }
        return;
    }else{
        mPrintf("%s", extrn[xtype-1].ex_name);    
    }

    doCR();

    if (changedir(roomBuf.rbdirname) == -1 )  return;

    getNormStr("filename", filename,
        (extrn[xtype-1].ex_batch) ? 80 : NAMESIZE, ECHO);
             
    if (extrn[xtype-1].ex_batch)
    {
      char *words[256];
      int count, i;
      char temp[81];

      strcpy(temp, filename);

      count = parse_it( words, temp);

      if (count == 0)
        return;

      for (i=0; i < count; i++)
      {
        if (checkfilename(words[i], 0) == ERROR)
        {
          mPrintf("\n No file %s", words[i]);
          changedir(cfg.homepath);
          return;
        }

        if (!filexists(words[i]) && !ambig(words[i]))
        {
          mPrintf("\n No file %s", words[i]);
          return;
        }

        if (!ambig(words[i]))
        {
          doCR();
          mPrintf("%s: ", words[i]);
          blocks(words[i], extrn[xtype-1].ex_block);
        }  
      }
    }else{
      if (checkfilename(filename, 0) == ERROR)
      {
        mPrintf("\n No file %s", filename);
        changedir(cfg.homepath);
        return;
      }
      if (ambig(filename))
      {
          mPrintf("\n Not a batch protocol");
          changedir(cfg.homepath);
          return;
      }
      if (!filexists(filename))
      {
          mPrintf("\n No file %s", filename);
          return;
      }
      blocks(filename, extrn[xtype-1].ex_block);
    }
  
    if (!strlen(filename))  return;

    if (!expert) tutorial("wcdown.blb");

    if (getYesNo("Ready for file transfer", 0))
    {
       time(&transTime1);
       wxsnd(roomBuf.rbdirname, filename, xtype);
       time(&transTime2);

       if (cfg.accounting && !logBuf.lbflags.NOACCOUNT && !specialTime)
       {
           calc_trans(transTime1, transTime2, 0);
       }

       sprintf(msgBuf->mbtext, "%s download of file %s in room %s]",
                      extrn[xtype-1].ex_name, filename, roomBuf.rbname);

       trap(msgBuf->mbtext, T_DOWNLOAD);
    }
    /* go back to home */
    changedir(cfg.homepath);
}

/************************************************************************/
/*      upload()  menu level routine                                    */
/************************************************************************/
void upload(char c)
{
    long    transTime1, transTime2;
    label filename;
    char comments[64];
    char ch, xtype;
    char ich = '\0';
                
    if (!c) 
      ch=(char)tolower((ich = commandchar()));
    else
      ch = c;

    xtype = (char)strpos(ch, extrncmd);
    
    if (!xtype)
    {
        if (ch == '?')
        {
          oChar('?');
          upDownMnu('U');
        }
        else{
          oChar(ich);
          mPrintf(" ?");
          if (!expert)
            upDownMnu('U');
        }
        return;
    }else{
        mPrintf("%s", extrn[xtype-1].ex_name);    
    }

    mPrintf("\n Free disk space: %ld bytes", bytesfree());
    doCR();

    if (!extrn[xtype-1].ex_batch)
    {
      getNormStr("filename", filename, NAMESIZE, ECHO);

      if (checkup(filename) == ERROR)  return;
             
      if (strlen(filename))
        getString("comments", comments, 64, FALSE, TRUE, "");
      else
        return;
    }
    else
        batchinfo(FALSE);

    if (!expert)  tutorial("wcup.blb");

    doCR();

    if (getYesNo("Ready for file transfer", 0))
    {
       time(&transTime1);          /* when did they start the Uload    */
       wxrcv(roomBuf.rbdirname, extrn[xtype-1].ex_batch ? "" : filename,
         xtype);
       time(&transTime2);          /* when did they get done           */

       if (cfg.accounting && !logBuf.lbflags.NOACCOUNT && !specialTime)
       {
           calc_trans(transTime1, transTime2, 1);
       }

       if (!extrn[xtype-1].ex_batch)
       {
          if (limitFlag && filexists(filename)) hide(filename);

          if (filexists(filename))
          {
             entercomment(filename, logBuf.lbname, comments);

             sprintf(msgBuf->mbtext, "%s upload of file %s in room %s]",
                       extrn[xtype-1].ex_name, filename, roomBuf.rbname);

             trap(msgBuf->mbtext, T_UPLOAD);
             
             if (comments[0])
                 sprintf(msgBuf->mbtext, "%s uploaded by %s\n Comments: %s",
                       filename, logBuf.lbname, comments);
             else
                 sprintf(msgBuf->mbtext, "%s uploaded by %s", filename,
                       logBuf.lbname);

             specialMessage();
          }
       }
       else
       {
           sprintf(msgBuf->mbtext, "%s file upload in room %s]",
               extrn[xtype-1].ex_name, roomBuf.rbname);

           trap(msgBuf->mbtext, T_UPLOAD);

           if (batchinfo(TRUE))
               specialMessage();
       }
    }
    changedir(cfg.homepath);
}

/************************************************************************/
/*  upload/download  menu                                               */
/************************************************************************/
static void upDownMnu(char cmd)
{
    int i;
  
    doCR();
    doCR();
    for (i=0; i<(int)strlen(extrncmd); i++)
        mPrintf(" .%c%c>%s\n", cmd, *(extrn[i].ex_name),
                               (extrn[i].ex_name + 1));
    mPrintf(" .%c?> -- this list\n", cmd);
    doCR();
}
