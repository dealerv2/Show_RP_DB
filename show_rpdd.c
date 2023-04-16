/* File show_rpdd.c -- RP DataBase Browser for rpdd.zrd files *
 * Date        Version     Author      Comment
 * 2023/03/07  1.0.0       JGM         First Working version.
 * 2023/04/17  1.1.0       JGM         Better Error Msg if DB open fails
 */
#define GNU_SOURCE
#define JGMDBG 1              /* so the dbgprt_macros.h works ok */
#define VERSION "1.1.0"
#define BUILD_DATE "2023/04/18"
#define AUTHORS "JGM"

#include <assert.h>           /* assert.h */
#include <error.h>            /* errno; perror */
#include <fcntl.h>            /* open flags etc. */
#include <stdlib.h>           /* exit(), malloc(), free(), rand48, atoi, */
#include <stdio.h>            /* fprintf etc. */
#include <string.h>           /* memcpy, strlen etc. */

#include <unistd.h>           /* getopt, write, and other low level i/o */
#include <sys/types.h>
#include <sys/stat.h>

#include "./dbgprt_macros.h"    /* JGMDPRT other DBGLOC etc. */

         /* --- types ---*/
typedef unsigned char BYTE_k;
typedef char DEAL_k[52];         /* char instead of unsigned char; easier compatibility and cards never use hi-bit anyway*/
struct rpdd_st {
     BYTE_k card[13] ;
     BYTE_k trick[10];
} ;
union zsep_ut {
   BYTE_k zerobyte[4];
   unsigned int izero;
} ;
struct pbn_st {
   char deal_ch[76];          /* col 76 for the null terminator */
   short int deal_tr[20];     /* tricks never use the hi bit and signed is easier to use than unsigned */
} ;
#define BLOCKSIZE 1000
#define OPTSTR "g:i:o:s:v:D:hVX:"
struct options_st {
   int  options_error;     // 0=OK, 1=Invalid Option, 2=help, 3=Version
   long seed ;             // Offset into file in BLOCKSIZE deal blocks. Default 0 . May cause a wraparound.
   int  opt_g;             // Number of records to retrieve from database. May cause a wraparound.
   char opt_i[256];        // infile name. Default rpdd.zrd
   char opt_o[256];        // ofile for dealer52 fmt. Default none
   int  opt_D ;            // Debug Level.   Default 0
   int  opt_h;             // Show help msg. Default 0
   int  opt_v;             // Show end of run stats (verbose). Default = 1
   int  opt_V;             // Show Version.  Default 0
   int  opt_X;             // Show the first X records of the dl52.bin file at end of run. Default=0
   /* Derived values */
   FILE *rp_file ;
   FILE *dl52_file ;
} ;


enum rp_seat_ek {KIBTZ=-1, WEST=0, NORTH, EAST, SOUTH } ;         /* RP numbering */
enum dl_seat_ek {COMPASS_N=0, COMPASS_E, COMPASS_S, COMPASS_W } ; /* Dealer numbering */
enum dl_suit_ek {NONE=-1, CLUBS, DIAMONDS, HEARTS, SPADES } ; /* Dealer numbering for Makecard stuff */
enum card_rank_ek {spot=-1, Two_rk=0, Three_rk, Four_rk, Five_rk, Six_rk, Seven_rk, Eight_rk, Nine_rk, Ten_rk, Jack_rk, Queen_rk, King_rk, Ace_rk };

         /* --- Defines ---*/
#define MAKECARD(suit,rank) ( (char) (((suit)<<4) | ((rank)&0x0F)) )
#define C_RANK(c) ( (c) & 0x0F )
#define C_SUIT(c) ( ((c)>>4 ) & 0x0F)
#define NT 4
#define BIG_SEED 10485                 // because the rpdd has 10,485,760 deals in it.
#define MAX_RPP_RECS 10485760

         /* --- Globals ---*/
char *pgm ;                            // copy of argv[0] for use in error and usage msgs
int jgmDebug = 0 ;
char rank_name[] = "23456789TJQKA";
short int ddres[4][5] = {0} ; /* results per seat/strain -- dealer numbering North=0, West=3;  Clubs=0, NT=4*/
int pbn_cnt = 0 ;
int rp_cnt = 0 ;
int dl52_cnt = 0 ;
int wrap_cnt = 0 ;
int recnum = 0 ;
union zsep_ut null_sep;
struct rpdd_st rp_rec;
BYTE_k *pbyte = &rp_rec.card[0] ;
DEAL_k curdeal ;
char pbn_rec[136] ; /* 75 for the cards, 60 for dd, x 20, and one for the null terminator. */
char rpdd_dir[]="/home/greg19/Programming/Bridge_SW/RP_DB" ;
char rpdd_path[]="/home/greg19/Programming/Bridge_SW/RP_DB/rpdd.zrd";

struct options_st opts = {
   .options_error=0, .seed=0, .opt_g=100, .opt_i="./rpdd.zrd", .opt_o="", .opt_D=0, .opt_h=0, .opt_v=1, .opt_V=0, .opt_X=0,
};

// n J873.J42.Q65.KT2 e AT652.A976.AJ82. s Q4.85.KT9.A87643 w K9.KQT3.743.QJ95

         /* --- Protos  ---*/
int get_options (int argc, char *argv[], struct options_st *opts) ;
void initprogram( struct options_st *opts ) ;

long int seek_rpdd_pos(FILE *rpdd_file, long int seed ) ;
int get_rprec     (FILE *rpfile, struct rpdd_st *rprec,  struct options_st *opts );
int put_dealer_rec(FILE *ofile, char deal[52]) ;
int put_pbn_rec   (char deal[52] ) ; /* also uses the global ddres[][] */
int get_dl52_rec(DEAL_k deal, short int ddres[][5], FILE *dl52_file) ;

char *Hand52_to_pbnbuff (int p, char *dl, char *buff ) ;
char *fmt_ddres( char *pb ) ;
int   usage(char *pgm );

void show_dl52_file(struct options_st *opts ) ;
void sr_hand_show(int p, char deal[52] ) ;
void show_opts(struct options_st *opts) ;
void show_init(struct options_st *opts) ;

         /* --- Code  ---*/

int main (int argc, char *argv[]) {
   int crdpos, tpos;
   short int tval ;
   int npos, epos, spos, wpos ;
   int pnum,j,k,strain,dealer_pnum;
   int byte_cnt ;
   int rank, suit ;
   char deal[52] ;
   char kard ;
   BYTE_k px4, tx2 ;
   JGMDPRT(1,"JGMDPRT Working. JGMDBG is defined. \n");
         /* --- Process Arglist  ---*/
   pgm = argv[0];
   int opt_err = 0;
   opt_err = get_options(argc, argv, &opts) ;
   if ( opt_err ) { fprintf(stderr, "getopts returns %d. Exiting now. \n",opt_err); return(-1); }
   jgmDebug = opts.opt_D ;
   JGMDPRT(2,"options returned OK. jgmDebug=%d, opt_D=%d\n",jgmDebug, opts.opt_D);
   DBGDO(  3,show_opts(&opts) ) ;
   initprogram(&opts) ;
   DBGDO(  3,show_init(&opts) ) ;

            /* --- Mainloop  ---*/
   byte_cnt = get_rprec(opts.rp_file, &rp_rec, &opts);
   recnum++;
   while ( byte_cnt > 0 && rp_cnt < opts.opt_g && wrap_cnt < 2) {    /* byte_cnt zero at eof -1 on error */
      memcpy(&null_sep.zerobyte, &rp_rec.card[0], 4 ) ;
      if (0 == null_sep.izero  ) {
         fprintf (stderr,"Found a null separator at record number %d \n", recnum );
         continue ; /* skip these records if any */
      }
      rp_cnt++;
      /* An RP dbase record starts with Spade Ace, then Spade King, and so on down to Club Deuce */
      suit = SPADES ;
      rank = Ace_rk ;
      kard = MAKECARD(suit, rank ) ;
      npos = 0; epos=13; spos=26; wpos=39 ; /* The slots in the Deal52 buffer where each hand begins. */

      JGMDPRT(5, "Recnum %d: Card[0]=%02x, card[12]=%02x, trick[0]=%02x, trick[9]=%02x\n",
                  recnum,rp_rec.card[0],rp_rec.card[12],rp_rec.trick[0],rp_rec.trick[9] ) ;
      for (crdpos=0 ; crdpos < 13 ; crdpos++ ) {  /* the whole deal is done in 13 bytes 4 cards each byte*/
         px4 = rp_rec.card[crdpos] ;
         JGMDPRT(6, "Crdpos=%d, cardByte=%02x\n" ,crdpos, px4) ;
         for (j = 0 ; j<4 ; j++ ) {  /* each byte holds 4 cards */
            pnum = (px4 & 0x03 ); /* process the right two bits -- little endian fmt*/
            px4  = px4 >> 2 ;
            kard = MAKECARD(suit, rank ) ; /* set the current card for this crdpos */
            switch (pnum) {                /* put the current card into the designated hand */
               case WEST : deal[wpos++] = kard ; break ;
               case EAST : deal[epos++] = kard ; break ;
               case NORTH: deal[npos++] = kard ; break ;
               case SOUTH: deal[spos++] = kard ; break ;
               default : printf("Cant happen in switch pnum = %d \n", pnum ) ;
            }
           JGMDPRT(6, " Card %c%c at pos %d:%d assigned to %c; seatcounts: N=%d,E=%d,S=%d,W=%d \n",
                     "CDHS"[suit],rank_name[rank],crdpos,j, "WNES"[pnum],npos,(epos-13),(spos-26),(wpos-39) ) ;
            if (--rank < 0 ) { /* have we done all of current suit?  */
               rank = Ace_rk;    /* then start new suit */
               suit-- ;
            }
         } /* end for J 0 .. 3 */
      } /* end for crdpos 0 .. 12 */
      strain = NT ;
      tpos = 0 ;
      memset(ddres, 0, sizeof(ddres) );
      while(tpos < 10 ) { /* Do the tricks:: NT for W,N,E,S, then Spades for W,N,E,S downto Clubs for W,N,E,S*/
         pnum=WEST ;
         for (k = 0 ; k < 2 ; k++ ) {           /* two bytes per strain */
            tx2 = rp_rec.trick[tpos++] ;
            for (j = 0 ; j < 2 ; j++ ) {        /* two players per byte */
               tval = tx2 & 0x0F ;
               dealer_pnum = (pnum+3)&0x00000003 ;  /* convert PAV player to Dealer Player. */
               ddres[dealer_pnum][strain] = tval ;
               JGMDPRT(6, "TPOS=%d,      strain=%c,  pnum=%d, dealer_pnum=%d, tval=%d, tx2=%02x \n",
                        (tpos-1), "CDHSN"[strain],pnum,   dealer_pnum,     tval,     tx2 );
               tx2 = tx2 >> 4 ;
               pnum++ ;
            } /* end for two players */
         } /* end a set of 4 results for a strain */
         strain-- ;
      } /* end while tpos */
      JGMDPRT(5, " ----- Record %d all done. npos=%d, epos=%d, spos=%d, wpos=%d tpos=%d --------\n",recnum, npos, epos, spos, wpos,tpos ) ;
#ifdef JGMDBG
   if (jgmDebug >= 7 ) { /* show the resulting hands */
      for (pnum = 0 ; pnum<4; pnum++ ) { sr_hand_show(pnum, deal) ; }
   }
#endif
      if (opts.dl52_file != NULL) { put_dealer_rec(opts.dl52_file, deal); }
      put_pbn_rec( deal ) ;
      byte_cnt = get_rprec(opts.rp_file, &rp_rec, &opts);
      recnum++;
 } /* end while Mainloop */
   fclose(opts.rp_file);
   if (opts.dl52_file) { fclose(opts.dl52_file) ; }
   if (wrap_cnt > 1 ) {
      fprintf(stderr, "No More Deals Available; Giving up now. \n");
   }
   if (opts.opt_v > 0 ) {
      fprintf(stderr, "% 8d RP DB Records read from file %s start at record number % 8ld \n",rp_cnt, opts.opt_i, 1+opts.seed*BLOCKSIZE );
      fprintf(stderr, "% 8d PBN Records Written to PBN file \n", pbn_cnt);
      fprintf(stderr, "% 8d Records wanted. %4d Wrap arounds occurred \n", pbn_cnt, wrap_cnt);
      if(strlen(opts.opt_o) > 0 ) {
         fprintf(stderr, "% 8d Binary records written to file %s \n", dl52_cnt, opts.opt_o);
      }
   }
   if (opts.opt_X != 0 ) {             // 0: no show ; +ve n: show n -ve n: show all;
      show_dl52_file( &opts ) ;
   }
   return 0 ;
} /* end main */

         /* --- Subroutines  ---*/
int get_dl52_rec(DEAL_k deal, short int ddres[][5], FILE *dl52_file) {
   // Using Globals to return results

   fread(deal, 1 , 52 , dl52_file) ;
   if (feof(dl52_file) ) {return -1 ; }
   fread(&ddres[0][0], 1 , 20*sizeof(short int) , dl52_file ) ;
   if (feof(dl52_file) ) {return -1 ; }
   return 1 ;
}

void  show_dl52_file(struct options_st *opts ) {
   if(strlen(opts->opt_o) == 0 ) {fprintf(stderr, "Cant Show Dealer Binary file. None was created\n"); return ; }
   opts->dl52_file = fopen(opts->opt_o, "r") ;
   if(opts->dl52_file == NULL ) { perror("Re-Open of Dealer Binary file Failed!.\n"); return ; }
   int show_cnt = 0 ;
   int show_max = (opts->opt_X >= 0 ) ? opts->opt_X : dl52_cnt ;
   int rc = get_dl52_rec(curdeal,ddres,opts->dl52_file) ;
   while (show_cnt < show_max && rc > 0 ) {
      put_pbn_rec(curdeal) ;
      show_cnt++ ;
      rc = get_dl52_rec(curdeal,ddres,opts->dl52_file) ;
   }
} /* end show_dl52_file */

// n AKT84.KQ.T43.KT3 e J32.AJ.J5.AQ8742 s Q765.98732.AQ2.J w 9.T654.K9876.965
#define MAXPBNLINE 150    // 76 for deal, 3*20=60 for the DD results
int put_pbn_rec ( char dl[52] ) {
   int h;
   char pbnbuff[MAXPBNLINE] ;
   char *pbuff ;
   pbuff = &pbnbuff[0] ;
   for (h=0; h<4; h++ ) {
      *pbuff++ = "nesw"[h]; *pbuff++ = ' ';
      pbuff = Hand52_to_pbnbuff(h,  dl, pbuff ) ; /* pbuff points to terminating NULL after the space*/
   }
   *(pbuff-1) = ';' ; /* replace last space with semicolon; semicolon is per the PBN std for extraneous info */
   pbuff = fmt_ddres( pbuff ) ;  /* fmt_ddsres will terminate bpbuff with a LF, and leave pbuff pointing at next free loc */
   *pbuff='\0';                  /* add a NULL so that %s works OK. */
   printf( "%s",pbnbuff) ;        /* pbn rec always to stdout; user can re-direct */
   pbn_cnt++ ;
   return pbn_cnt;

} /* end put_pbn-rec */
int rp_read_err(int byte_cnt, int rsize, int recnum ) {
      BYTE_k rpbyte ;
      if (byte_cnt != rsize && 0 != byte_cnt) {
         fprintf(stderr, "Error! %d Bytes READ at recnum=%d: record = ",byte_cnt, recnum);
         for (int x = 0 ; x < byte_cnt ; x++ ) {
            rpbyte = *(pbyte + x ) ;
            fprintf(stderr, "%02x ", rpbyte);
         }
         fprintf(stderr, " \n");
         return -2  ;
      }
   return 0 ;
}

/* read a binary record from the rpdd.zrd database Wrap around if not enough records obtained */
int get_rprec (FILE *rpfile, struct rpdd_st *rprec, struct options_st *opts ) {
   int byte_cnt;

   byte_cnt = fread(rprec, 1, sizeof(struct rpdd_st), rpfile) ; /* read 23 bytes into rprec */
   if (rp_read_err(byte_cnt, 23, recnum) ) {  return -2 ; }
   // byte_cnt = fread(rprec, struct rpdd_st, 1, rpfile) ; /* read one struct rpdd_st into rprec */
   if( feof(rpfile) && rp_cnt < opts->opt_g ) { /* Reached eof before necessary records got */
      fseek(rpfile, 0, SEEK_SET ) ;             /* rewind to beginning */
      wrap_cnt++;
      byte_cnt = fread(rprec, 1, sizeof(struct rpdd_st), rpfile) ;
      if (rp_read_err(byte_cnt, 23, recnum) ) {  return -2 ; }
      recnum = 1 ;                              /* recnum tracks errors; want to know actual one in the file */
      fprintf(stderr, "Wrap Around # %d occurred at rp_cnt=%d, recnum=%d \n",wrap_cnt,rp_cnt,recnum);
   }
   return byte_cnt ;  /* zero at eof; -1 if error ; 23 all other times except maybe separator record.... */
} /* end get_rprec */

   /* Format a Deal52 array into a PBN format record
    * pbn fmt suit sep is dot, hand sep is space. voids are deduced from that.
    * the hands in *dl are sorted. dl[p*13+0] = Highest Spade; dl[p*13+12] = lowest club.
    */

char *Hand52_to_pbnbuff (int p, char *dl, char *buff ) {
   char r_ids[] = "23456789TJQKA";
   int curr_suit, card_rank, card_suit;
   int di, count;
   char *bp ;
   char kard ;
   char suit_sep = '.';
   di = p*13 ;
   bp = buff ;
   count = 0 ;
   curr_suit = 3 ; // spades
   while (count < 13 ) {  // a hand ALWAYS has exactly 13 cards
       kard = dl[di] ; card_suit = C_SUIT(kard); card_rank = C_RANK(kard) ;
       while( curr_suit != card_suit ) { /* write a suit separator for missing suits spades downto first one*/
            *bp++ = suit_sep;
            JGMDPRT(6, "Wrote Void for suit %d \n",curr_suit ) ;
            curr_suit-- ;
        } /* end while curr_suit != card_suit */
        assert(card_suit == curr_suit) ;
        while ( (curr_suit == card_suit) && (count < 13) ) { /* write the cards in this suit */
            kard = dl[di]; card_suit = C_SUIT(kard); card_rank = C_RANK(kard) ;
            if (curr_suit != card_suit ) break;
           *bp++ = r_ids[card_rank];
           count++; di++;
           JGMDPRT(7," Num[%d]=%c%c ", count, "CDHS"[curr_suit], *(bp-1) ) ;
        } // end while curr_suit
        JGMDPRT(7,"\n");
       *bp++ = suit_sep;
        curr_suit-- ; /* Move to next suit */
    } /* end while count < 13*/
    assert(count == 13 ) ;
    // Normal case curr_suit is -1; void clubs curr_suit = 0, void clubs, diamonds, and hearts curr_suit = 2
    // In case there were voids at the end of 13 cards
        while ( curr_suit >= 0 ) { /* write a suit separator for missing suits after the last one downto clubs*/
            *bp++ = suit_sep ;
            curr_suit-- ;
        }
        /* the last char is the suit separator which we don't want after the club suit, so replace it with a space */
        if ( *(bp-1) == suit_sep ) { *(bp-1) = ' ' ; }
        else { fprintf(stderr, "CANT HAPPEN in Hand52_to_Buff, last char is not a suit_separator %c \n", *(bp-1) ); }
        *bp = '\0' ; // terminate the buffer as a string
        return bp  ; /* return pointer to null byte in case we want to append another hand to the buffer */
} /* end Hand52_to_pbnbuff */

/* Fill a text buffer with the dds trix taken. Dealer fmt: North: C,D,H,S,NT, East: C,D,H,S,NT then South then West */
char *fmt_ddres( char *pb ) {
   int seat,strain ;
   int ch_cnt ;
   for (seat=COMPASS_N; seat <= COMPASS_W; seat++ ) {
      for (strain=CLUBS; strain <= NT; strain++ ) {
         ch_cnt = sprintf(pb,"%02d,",ddres[seat][strain]) ;
         pb += ch_cnt ;
      }
   }
   sprintf((pb-1),"\n") ; /* replace the last comma with the LF */
   *(pb)= '\0';
   return pb ;
} /* end fmt_ddsres */


/* dealer_rec is a binary record for easy import into the dealer program without further parsing
 * even though the dealer program does not have a feature to import this file, since it works on the rpdd.zrd file directly.
 */
int put_dealer_rec(FILE *ofile, char deal[52] ) {
   /* Fixed format output; no record separators -- Fut maybe 0D */
   size_t io_count ;
   io_count = fwrite(deal, 1 , 52 , ofile ) ; /* write the 52 cards in the deal */
   if (io_count != 52 ) { fprintf(stderr, "Cards Write Error!! Wrote %ld cards, Expected %d \n", io_count, 52 ) ; }

   /* now append the tricks (no point in having the deal with no results)*/
   io_count = fwrite(ddres , sizeof(short int), 20, ofile ) ;
   if (io_count != 20 ) { printf("Trix Write Error!! Wrote %ld items, Expected %d \n", io_count, 20 ) ; }
   dl52_cnt++ ;
  return dl52_cnt ;
}  /* end put_dealer_rec */

void sr_hand_show(int p, char *dl ) {
    char rns[] = "23456789TJQKA";
    char sns[] ="CDHS";
    char rn, sn ;
    int s, r ,  di ;
    di = p*13 ;
    char sepspc[] = " ";
    // fprintf (stderr," Showing Hand  using CARD_SUIT and CARD_RANK in sr_hand_show\n");
    if (1 == p || 3 == p ) {fprintf(stderr, "%50s",sepspc ) ; }
    fprintf (stderr,"Hand %c=[ ", "NESW"[p]);
    for (di = p*13 ; di < (p+1)*13 ; di++ ) {
             s=C_SUIT(dl[di]); sn=sns[s];
             r=C_RANK(dl[di]); rn=rns[r];
             fprintf (stderr,"%c%c ", sn,rn );
    }
    fprintf (stderr, "]\n");
} /* end sr_hand_show */

#define RPDD_RECSIZE 23
long int seek_rpdd_pos(FILE *rpdd_file, long int seed ) {
   /* 'seed' var in this case is the number of 1000 record blocks to skip from the start of the file.
    * as the biggest the file can be is 10,485,760 deals, seed should be 0 <= seed <= 10,485
    */
   int rc ;
   long int rpdd_pos ;
   rpdd_pos = seed * BLOCKSIZE * RPDD_RECSIZE ;
   rc = fseek(rpdd_file, rpdd_pos, SEEK_SET ) ;
   recnum = seed * BLOCKSIZE +1; /* track the actual record number in case of errors */
   JGMDPRT(2,"Seed=%ld,recnum = %d, rpdd_pos=%ld, fseek_rc=%d\n",seed,recnum,rpdd_pos,rc) ;
   if (rc != 0 ) {
      perror("Seek to pos calculated from seed on rpdd.zrd file FAILED! Reset-ing to start of file (pos=0)\n");
      fprintf(stderr, "Seed=%ld calculated position=%ld \n",seed, rpdd_pos ) ;
      rpdd_pos = 0 ;
      fseek(rpdd_file, rpdd_pos, SEEK_SET) ;
      recnum = 0 ;
   }

   return rpdd_pos ;
} /* end seek_rpdd_pos */

int get_options (int argc, char *argv[], struct options_st *opts) {
    int opt_c;
    extern char *optarg ;
    extern int   optind, opterr, optopt ;
    opterr = 0;
    opts->options_error = 0 ;  /* assume success */
    while ((opt_c = getopt(argc, argv, OPTSTR)) != EOF) {
      switch(opt_c) {
        case 'g':
            opts->opt_g = atoi(optarg) ;
            break ;
        case 'h': {
            usage( pgm );
            opts->options_error = 4 ;
            break;
        }
      case 'i':
        strncpy(opts->opt_i, optarg, 255 ) ;
        break;
      case 'o':
        strncpy(opts->opt_o, optarg, 255 ) ;
        break;
      case 's':
        opts->seed = atol( optarg );
        if (opts->seed < 0 || opts->seed > BIG_SEED ) {
            fprintf (stderr, "Seed Invalid: seed must be bteween %d and %d includsive \n", 0, BIG_SEED );
            fprintf (stderr, "Setting Seed to zero and continuing. \n");
            opts->seed = 0L ;
        }
        break;
      case 'v':
        opts->opt_v = atoi( optarg );
      case 'D':
        opts->opt_D = atoi( optarg );
        jgmDebug = opts->opt_D;
        break;
      case 'V':
        printf ("Version info....\n");
        printf ("Revision: %s \n", VERSION );
        printf ("Build Date:[%s] \n", BUILD_DATE );
        printf ("$Authors: JGM $\n");
        printf ("Default Database: %s\n",rpdd_path ) ;
        #ifdef JGMDBG
          printf("JGMDBG is defined. Debugging printing to stderr is active\n");
        #endif
        opts->options_error = 3 ; // Version Info
        return 3 ;
        break ;
      case 'X':
        opts->opt_X = atoi( optarg );
        break;

      default :
            fprintf(stdout, "DEFAULT CASE FALL THROUGH\n");
      case '?' : // there was an invalid option invalid char is in optopt
            fprintf(stdout, "%s Usage: -[options] [input_filename | stdin] [>output_file]\n", argv[0]);
            fprintf(stdout, "Valid Options == [%s]\n", OPTSTR );
            fprintf(stdout, "\t\t\t[%c] is an invalid option\n", optopt ) ;
            opts->options_error = 1 ;  // invalid option found
            break;
      } /* end switch(opt) */
    } /* end while getopt*/
    if (opts->opt_D >= 4 ) { fprintf(stderr, "Command Line switches done proceeding with main program \n") ; }
        return opts->options_error ;

} /* end getopts */

void initprogram(struct options_st *opts) {
   long rpdd_pos = 0 ;
   jgmDebug = opts->opt_D ;
   if(strcmp(opts->opt_i, "=") == 0 ) {  /* strings match */
      strncpy(opts->opt_i, rpdd_path, sizeof(opts->opt_i) - 1 ) ;
      JGMDPRT(3,"Using Default DB path %s\n",rpdd_path ) ;
      opts->rp_file = fopen(rpdd_path,"r") ;
   }
   else {
      JGMDPRT(3,"Using Requested DB path %s\n",opts->opt_i) ;
      opts->rp_file = fopen(opts->opt_i, "r") ;
   }
   if (opts->rp_file == NULL ) {
      perror("Cant open RP database for read!");
      fprintf(stderr, "\n Read Open Failed for path=[%s] Aborting run. \n", opts->opt_i);
      exit (-1) ;
   }
   JGMDPRT(2,"RP DD File [%s] Opened OK \n",opts->opt_i );
   if (strlen(opts->opt_o ) > 0 ) {
      opts->dl52_file = fopen(opts->opt_o, "w") ;
      if (opts->dl52_file == NULL ) {
         perror("Cant open Dealer Binary File for write");
         fprintf(stderr, "\n Write Open Failed for path [%s] Aborting run. \n",opts->opt_o);
         exit (-1) ;
      }
   }
   JGMDPRT(2,"Dealer Binary File %s Opened OK \n",opts->opt_o );

   if( opts->seed > 0 ) {
      rpdd_pos = seek_rpdd_pos(opts->rp_file, opts->seed) ;
      if( rpdd_pos == 0 ) {fprintf(stderr, "Seeking to position of seed failed. Starting at beginning \n"); }
   }
   JGMDPRT(2,"Seeking to Record # %ld OK \n",opts->seed * BLOCKSIZE + 1 );
   return ;
} /* end initprogram */

#define USAGE_MSG "Usage: %s -[g:i:o:s:v:D:hVX:] [>ofile.pbn] \n" \
"g={Number of records to get from RP Database -- Default 100} \n" \
"i={Name of RP DB file. '=' uses name rpdd.zrd in default directory\n" \
"o={path name of Dealer binary file; if not given no binary file generated}\n" \
"s={seed aka the thousand block number to start from; Default = 0}\n" \
"v={1: print, 0:do not print,  End of run stats} \n" \
"D={Debug Verbosity level} \n" \
"h={show this msg}\n"\
"V={show Version info} \n" \
"X={Show the first N records of the Dealer binary file at end of run. Default=0 0=No show; +ve N Number to show. -ve N show all}\n"

int usage(char *pgm) {
   fprintf(stderr, USAGE_MSG ,pgm) ;
   return -1 ;
}
void show_opts(struct options_st *opts) {
   fprintf(stderr, "------- Showing Run Time Option Settings ----- *\n");
   fprintf(stderr, "Seed = %ld\n",opts->seed ) ;
   fprintf(stderr, "opt_g = %d\n",opts->opt_g ) ;
   fprintf(stderr, "opt_i = %s\n",opts->opt_i ) ;
   fprintf(stderr, "opt_o = %s\n",opts->opt_o ) ;
   fprintf(stderr, "opt_D = %d\n",opts->opt_D ) ;
   fprintf(stderr, "opt_v = %d\n",opts->opt_v ) ;
   fprintf(stderr, "opt_X = %d\n",opts->opt_X ) ;
  return ;
}
void show_init(struct options_st *opts) {
   long rp_pos = ftell(opts->rp_file) ;
   fprintf(stderr, "------- Showing Initialization Status ----- *\n");
   fprintf(stderr, "Starting rpdd recnum = %ld at position %ld \n",1L+rp_pos/RPDD_RECSIZE,rp_pos ) ;
   fprintf(stderr, "Debug Verbosity=%d \n", jgmDebug ) ;

}
