/* -------------------------------------------------------------------- */
/*  TIME.C                        ACit                         91Sep30  */
/*               Functions relating to the time and date                */
/* -------------------------------------------------------------------- */

#include <dos.h>
#include <string.h>
#include <time.h>
#include "ctdl.h"
#include "prot.h"
#include "glob.h"

/* -------------------------------------------------------------------- */
/*                              Contents                                */
/* -------------------------------------------------------------------- */
/*  changedate()    Menu-level change date routine                      */
/*  dayofweek()     returns current day of week 0 - 6  sun = 0          */
/*  diffstamp()     display the diffrence from timestamp to present     */
/*  getdstamp()     get datestamp (text format)                         */
/*  gettstamp()     get timestamp (text format)                         */
/*  hour()          returns hour of day  0 - 23                         */
/* $set_date()       Gets the date from the user and sets it             */
/*  sstrftime()     formats a custom time and date string using formats */
/*  pause()         busy-waits N/100 seconds                            */
/*  systimeout()    returns 1 if time has been exceeded                 */
/* -------------------------------------------------------------------- */

static char *monthTab[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                             "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" } ;
static char *fullmnts[12] = {"January",   "February", "March",    "April",
                             "May",       "June",     "July",     "August",
                             "September", "October",  "November", "December" } ;
static char *days[7]      = {"Sun",  "Mon", "Tue", "Wed",
                             "Thu",  "Fri", "Sat" } ;
static char *fulldays[7]  = {"Sunday",   "Monday", "Tuesday", "Wednesday",
                             "Thursday", "Friday", "Saturday" } ;

static void set_date(void);

/* -------------------------------------------------------------------- */
/*  changedate()    Menu-level change date routine                      */
/* -------------------------------------------------------------------- */
void changeDate(void)
{
    char dtstr[30];

    doCR();

    sstrftime(dtstr, 29, "%x", 0l);
    mPrintf(" Current date is now: %s", dtstr);

    doCR();

    sstrftime(dtstr, 29, "%X", 0l);
    mPrintf(" Current time is now: %s", dtstr);

    doCR();

    if(!getYesNo(" Enter a new date & time", 0)) return;

    set_date();
}

/* -------------------------------------------------------------------- */
/*  dayofweek()     returns current day of week 0 - 6  sun = 0          */
/* -------------------------------------------------------------------- */
int dayofweek(void)
{
    long stamp;
    struct tm *timestruct;

    time(&stamp);

    timestruct = localtime(&stamp);

    return(timestruct->tm_wday);
}

/* -------------------------------------------------------------------- */
/*  diffstamp()     display the diffrence from timestamp to present     */
/* -------------------------------------------------------------------- */
void diffstamp(long oldtime)
{
    long currentime, diff;
    char tstr[10], dstr[10];
    int days;

    time(&currentime);

    diff = currentime - oldtime;

    diff = diff + 315561600L;
    
    sstrftime(dstr, 9, "%j", diff);
    sstrftime(tstr, 9, "%X", diff);

    /* convert it to integer */
    days = atoi(dstr) - 1;

    if (days)
        mPrintf(" %d %s ",  days, days == 1 ? "day" : "days");

    mPrintf(" %s", tstr);
}

/* -------------------------------------------------------------------- */
/*  getdstamp()     get datestamp (text format)                         */
/* -------------------------------------------------------------------- */
void getdstamp(char *buffer, unsigned int stamp)
{
    int  day, year, mth;

    char month[4];

    year = (( stamp >> 9) & 0x7f) + 80;
    mth  = ( stamp >> 5) & 0x0f;
    day = stamp & 0x1f;

    if(mth < 1 || mth > 12 || day < 1 || day > 31)
    {
        strcpy(buffer, "       ");
        return;
    }

    strcpy(month, monthTab[mth-1]);

    sprintf(buffer, "%d%s%02d", year, month, day);
}

/* -------------------------------------------------------------------- */
/*  gettstamp()     get timestamp (text format)                         */
/* -------------------------------------------------------------------- */
void gettstamp(char *buffer, unsigned int stamp)
{
    int hours, minutes, seconds;

    hours   =  (stamp >> 11) & 0x1f;
    minutes  = (stamp >> 5)  & 0x3f;
    seconds  = (stamp & 0x1f) * 2;

    sprintf(buffer, "%02d:%02d:%02d", hours, minutes, seconds);
}

/* -------------------------------------------------------------------- */
/*  hour()          returns hour of day  0 - 23                         */
/* -------------------------------------------------------------------- */
int hour(void)
{
    long stamp;
    struct tm *timestruct;

    time(&stamp);

    timestruct = localtime(&stamp);

    return(timestruct->tm_hour);
}

/* -------------------------------------------------------------------- */
/*  set_date()       Gets the date from the user and sets it             */
/* -------------------------------------------------------------------- */
static void set_date(void)
{
    union REGS inregs, outregs;
    int yr, mth, dy, hr, mn, sc;
    char dtstr[20];
    struct tm *timestruct;
    long stamp;

    time(&stamp);

    timestruct = localtime(&stamp);

    yr  = timestruct->tm_year;
    mth = timestruct->tm_mon + 1;
    dy  = timestruct->tm_mday;
    hr  = timestruct->tm_hour;
    mn  = timestruct->tm_min;
    sc  = timestruct->tm_sec;

    inregs.x.cx  = (uint) getNumber("Year",  85l, 99l,(long)yr ) + 1900;
    inregs.h.dh  = (uchar) getNumber("Month", 1l,  12l,(long)mth);
    inregs.h.dl  = (uchar) getNumber("Day",   1l,  31l,(long)dy);
    inregs.h.ah = 0x2b;
    intdos(&inregs, &outregs);

    inregs.h.ch  = (uchar) getNumber("Hour",   0l, 23l,(long)hr);
    inregs.h.cl  = (uchar) getNumber("Minute", 0l, 59l,(long)mn);
    inregs.h.dh  = (uchar) getNumber("Second", 0l, 59l,(long)sc);
    inregs.h.ah = 0x2d;
    intdos(&inregs, &outregs);

    doCR();

    sstrftime(dtstr, 19, "%x", 0l);
    mPrintf(" Current date is now: %s", dtstr);

    doCR();

    sstrftime(dtstr, 19, "%X", 0l);
    mPrintf(" Current time is now: %s", dtstr);

    doCR();
}

/* extern char *tzname[2]; */
/* -------------------------------------------------------------------- */
/*  sstrftime()     formats a custom time and date string using formats */
/* -------------------------------------------------------------------- */
void sstrftime(char *outstr, int maxsize, const char *formatstr, long tnow)
{
    int i, k;
    char temp[30];
    struct tm *tmnow;      

    if (tnow == 0l) time(&tnow);

    tmnow = localtime(&tnow);

    outstr[0] = '\0';

    for(i=0; formatstr[i]; i++)
    {
        if(formatstr[i] != '%')
            sprintf(temp, "%c", formatstr[i]);
        else
        {
            i++;
            temp[0] = '\0';
            if(formatstr[i])
            switch(formatstr[i])
            {
            case 'a': /* %a  abbreviated weekday name */
                    sprintf(temp, "%s", days[tmnow->tm_wday]);
                    break;
            case 'A': /*  %A  full weekday name */
                    sprintf(temp, "%s", fulldays[tmnow->tm_wday]);
                    break;
            case 'b': /*  %b  abbreviated month name */
                    sprintf(temp, "%s", monthTab[tmnow->tm_mon]);
                    break;
            case 'B': /*  %B  full month name */
                    sprintf(temp, "%s", fullmnts[tmnow->tm_mon]);
                    break;
            case 'c': /*  %c  standard date and time string */
                    sprintf(temp, "%s", ctime(&tnow));
                    temp[strlen(temp)-1] = '\0';
                    break;
            case 'd': /*  %d  day-of-month as decimal (1-31) */
                    sprintf(temp, "%d", tmnow->tm_mday);
                    break;
            case 'D': /*  %D  day-of-month as decimal (01-31) */
                    sprintf(temp, "%02d", tmnow->tm_mday);
                    break;
            case 'H': /*  %H  hour, range (00-23) */
                    sprintf(temp, "%02d", tmnow->tm_hour);
                    break;
            case 'I': /*  %I  hour, range (01-12) */
                    if(tmnow->tm_hour)
                        sprintf(temp, "02%d", tmnow->tm_hour > 12 ?
                                            tmnow->tm_hour  -12 :
                                            tmnow->tm_hour );
                    else
                        sprintf(temp, "%d", 12);
                    break;
            case 'j': /*  %j  day-of-year as a decimal (001-366) */
                    sprintf(temp, "%03d", tmnow->tm_yday + 1);
                    break;
            case 'm': /*  %m  month as decimal (01-12) */
                    sprintf(temp, "%02d", tmnow->tm_mon + 1);
                    break;
            case 'M': /*  %M  minute as decimal (00-59) */
                    sprintf(temp, "%02d", tmnow->tm_min);
                    break;
            case 'p': /*  %p  locale's equivaent af AM or PM */
                    sprintf(temp, "%s", tmnow->tm_hour > 11 ? "PM" : "AM");
                    break;
            case 'S': /*  %S  second as decimal (00-59) */
                    sprintf(temp,"%02d", tmnow->tm_sec);
                    break;
            case 'U': /*  %U  week-of-year, Sunday being first day (00-52) */
                    k = tmnow->tm_wday - (tmnow->tm_yday % 7);
                    if(k<0) k += 7;
                    if(k != 0)
                    {
                        k = tmnow->tm_yday - (7-k);
                        if(k<0) k = 0;
                    }
                    else
                        k = tmnow->tm_yday;
                    sprintf(temp, "%02d", k/7);
                    break;
            case 'W': /*  %W  week-of-year, Monday being first day (00-52) */
                    k = tmnow->tm_wday - (tmnow->tm_yday % 7);
                    if(k<0) k += 7;
                    if(k != 1)
                    {
                        if(k==0) k = 7;
                        k = tmnow->tm_yday - (8-k);
                        if(k<0) k = 0;
                    }
                    else
                        k = tmnow->tm_yday;
                    sprintf(temp, "%02d", k/7);
                    break;
            case 'w': /*  %w  weekday as a decimal (0-6, sunday being 0) */
                    sprintf(temp, "%d", tmnow->tm_wday);
                    break;
            case 'x': /*  %x  standard date string */
#if 1               
               if (! (tmnow->tm_yday))       /* 1st day of year */
                    sprintf(temp, "New Years Day '%02d",
                            tmnow->tm_year);
               else if ( !(tmnow->tm_mon) &&     /* January, Monday */
                        1 == tmnow->tm_wday &&
                        2 == (int)(tmnow->tm_mday-1)/7)  /* 3rd Monday */
                    sprintf(temp, "M. L. King, Jr. Day '%02d",
                            tmnow->tm_year);
               else if (1 == tmnow->tm_mon &&     /* February 14 */
                        14 == tmnow->tm_mday)
                    sprintf(temp, "Valentine's Day '%02d",
                            tmnow->tm_year);
               else if (1 == tmnow->tm_mon &&     /* February 18 */
                        18 == tmnow->tm_mday)
                    sprintf(temp, "President's Day '%02d",
                            tmnow->tm_year);
               else if (2 == tmnow->tm_mon &&     /* March 17 */
                        17 == tmnow->tm_mday)
                    sprintf(temp, "St. Patrick's Day '%02d",
                            tmnow->tm_year);
               else if (2 == tmnow->tm_mon &&     /* March 24 */
                        24 == tmnow->tm_mday)
                    sprintf(temp, "Err Head's Birthday '%02d",
                            tmnow->tm_year);
               else if (3 == tmnow->tm_mon &&     /* April 1 */
                        1 == tmnow->tm_mday)
                    sprintf(temp, "April Fool's Day '%02d",
                            tmnow->tm_year);
               else if (4 == tmnow->tm_mon &&     /* May, Sunday */
                        !(tmnow->tm_wday) &&
                        1 == (int)(tmnow->tm_mday-1)/7)  /* 2nd Sunday */
                    sprintf(temp, "Mother's Day '%02d",
                            tmnow->tm_year);
               else if (4 == tmnow->tm_mon &&     /* May, Saturday */
                        6 == tmnow->tm_wday &&
                        2 == (int)(tmnow->tm_mday-1)/7)  /* 3rd Saturday */
                    sprintf(temp, "Armed Forces Day '%02d",
                            tmnow->tm_year);
               else if (4 == tmnow->tm_mon &&     /* May 30 */
                        30 == tmnow->tm_mday)
                    sprintf(temp, "Memorial Day '%02d",
                            tmnow->tm_year);
               else if (5 == tmnow->tm_mon &&     /* June 14 */
                        14 == tmnow->tm_mday)
                    sprintf(temp, "Flag Day '%02d",
                            tmnow->tm_year);
               else if (5 == tmnow->tm_mon &&     /* June, Sunday */
                        !(tmnow->tm_wday) &&
                        2 == (int)(tmnow->tm_mday-1)/7)  /* 3rd Sunday */
                    sprintf(temp, "Father's Day '%02d",
                            tmnow->tm_year);
               else if (6 == tmnow->tm_mon &&     /* July 4 */
                        4 == tmnow->tm_mday)
                    sprintf(temp, "Independence Day '%02d",
                            tmnow->tm_year);
               else if (8 == tmnow->tm_mon &&     /* September, Monday */
                        1 == tmnow->tm_wday &&
                        0 == (int)(tmnow->tm_mday-1)/7)  /* 1st Monday */
                    sprintf(temp, "Labor Day '%02d",
                            tmnow->tm_year);
               else if (9 == tmnow->tm_mon &&     /* October 12 */
                        12 == tmnow->tm_mday)
                    sprintf(temp, "Columbus Day '%02d",
                            tmnow->tm_year);
               else if (9 == tmnow->tm_mon &&     /* October 31 */
                        31 == tmnow->tm_mday)
                    sprintf(temp, "Halloween '%02d",
                            tmnow->tm_year);
               else if (10 == tmnow->tm_mon &&     /* November, Tuesday */
                        2 == tmnow->tm_wday &&
                        0 == (int)(tmnow->tm_mday-1)/7)  /* 1st Tuesday */
                    sprintf(temp, "Election Day '%02d",
                            tmnow->tm_year);
               else if (10 == tmnow->tm_mon &&     /* November 11 */
                        11 == tmnow->tm_mday)
                    sprintf(temp, "Veteran's Day '%02d",
                            tmnow->tm_year);
               else if (10 == tmnow->tm_mon &&     /* November, Thursday */
                        4 == tmnow->tm_wday &&
                        3 == (int)(tmnow->tm_mday-1)/7)  /* 4th Thursday */
                    sprintf(temp, "Thanksgiving Day '%02d",
                            tmnow->tm_year);
               else if (11 == tmnow->tm_mon &&     /* December 24 */
                        24 == tmnow->tm_mday)      
                    sprintf(temp, "Christmas Eve '%02d", 
                            tmnow->tm_year);
               else if (11 == tmnow->tm_mon &&     /* December 25 */
                        25 == tmnow->tm_mday)
                    sprintf(temp, "Christmas Day '%02d", 
                            tmnow->tm_year);
               else if (11 == tmnow->tm_mon &&     /* December 31 */
                        31 == tmnow->tm_mday)
                    sprintf(temp, "New Years Eve '%02d", 
                            tmnow->tm_year);
#endif
               else
                    sprintf(temp, "%02d%s%02d", tmnow->tm_year,
                                                monthTab[tmnow->tm_mon],
                                                tmnow->tm_mday);
                    break;
            case 'X': /*  %X  standard time string */
                    sprintf(temp, "%02d:%02d:%02d", tmnow->tm_hour,
                                                    tmnow->tm_min,
                                                    tmnow->tm_sec);
                    break;
            case 'y': /*  %y  year in decimal without century (00-99) */
#if 1            
                    sprintf(temp, "%02d", tmnow->tm_year % 100);
#else
                    sprintf(temp, "%02d", tmnow->tm_year);
#endif
                    break;
            case 'Y': /*  %Y  year including century as decimal */
#if 1                    
                    sprintf(temp, "%d", 1900+tmnow->tm_year);
#else

                    if(tmnow->tm_year > 99)
                    {
                        tmnow->tm_year -= 100;
                        sprintf(temp, "20%02d", tmnow->tm_year);
                    }
                    else
                        sprintf(temp, "19%02d", tmnow->tm_year);
#endif
                    break;
            case 'Z': /*  %Z  timezone name */
                    tzset();
                    sprintf(temp, "%s", tzname[0]);
                    break;
            case '%': /*  %%  the percent sign */
                    sprintf(temp, "%%");
                    break;
            default:
                    temp[0] = '\0';
                    break;
            }  /* end of switch */

        }  /* end of if */

        if( (int)(strlen(temp) + strlen(outstr)) > maxsize)
            break;
        else
            if(strlen(temp))
                strcat(outstr, temp);

    } /* end of for loop */
}

/* -------------------------------------------------------------------- */
/*  pause()         busy-waits N/100 seconds                            */
/* -------------------------------------------------------------------- */
void pause(int ptime)
{
    union REGS in, out;
    register int i, j=0;
    in.h.ah=0x2C;
    intdos(&in, &out);
    i = out.h.dl;
    while(j < ptime)
    {
        in.h.ah=0x2C;
        intdos(&in, &out);
        if(out.h.dl < (uchar)i)
            j += (100+out.h.dl)-i;
        else
            j += out.h.dl-i;
        i = out.h.dl;
    }
}

/* -------------------------------------------------------------------- */
/*  systimeout()    returns 1 if time has been exceeded                 */
/* -------------------------------------------------------------------- */
int systimeout(long timer)
{
    long currentime;

    time(&currentime);

    if (( 
          (
            (loggedIn && (((currentime - timer) / 60 ) >= cfg.timeout))
          ||(!loggedIn && (((currentime - timer) / 60 ) >= cfg.unlogtimeout))
          )
        &&(whichIO == CONSOLE || (whichIO == MODEM  &&  haveCarrier))
        ) || sleepkey)
    {
        sleepkey = FALSE;
        return(TRUE);
    }
    return(FALSE);
}
