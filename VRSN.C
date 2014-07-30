/* -------------------------------------------------------------------- */
/*  VRSN.C                                                     91Sep30  */
/* -------------------------------------------------------------------- */

#define VERSION "3.11.00a/405"
#if 1
#define SOFTNAME "ACit"

char softname[] = SOFTNAME;
#endif
char version[] = VERSION;

#ifdef ALPHA_TEST
char testsite[] = "Alpha Test Site";
#else
#  ifdef BETA_TEST
char testsite[] = "Beta Test Site";
#  else
char testsite[] = "";
#  endif
#endif

char cmpDate[] = __DATE__;
char cmpTime[] = __TIME__;

char *welcome[] = {    /* 10 LINES MAX LENGTH!!!!!! */
    "Ah, blow it out your ass!",
    "",
    "--goltar",
    0
};

char *copyright[] = {   /* 2 LINES ONLY!!!! */
    "ACit by Richard Goldfinder",
    "Created using Microsoft C Optimizing Compiler Version 6.00A",
    0
};
