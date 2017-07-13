// *********************************************************
//  PUMA2.H                                             
//       - Header file for the DT3852 board
//                                                         
//     syntax for use:                                     
//            #include "puma2.h"                        
// *********************************************************


#include <graph.h>
#include <memory.h>
#include <process.h>

#define COM1      0x3F8
#define LSR       5
#define THR       0
#define RDR       0
#define THRRDY    0x20
#define RS232     0X14
#define SETUP     0x83
#define B1200     0x80
#define NOPARITY  0x00
#define WORD8     0x03
#define STOP1     0x00
#define DTARDY    0X01
#define MASK      0x7F
#define STOP      0x00
#define PLAY      0x01
#define RW        0x03
#define PAUSE     0x04
#define FADV      0x05
#define SLOWF     0x06
#define SLOWR     0x07
#define FRAMENUM  0x0D
#define STATUS    0x0E
#define SET_VCR   0x14
#define VCR_TYPE  0x34
#define TERMINATE 0xFE
#define ENTER     13
#define ESC       27
#define F1        59
#define LENGTH    25
#define MAXJTS    20
#define NO        0
#define YES       !NO

union  _REGS regs;
CONFIG config;

struct nametype
{
    char name[15];
} jtnames[MAXJTS];

struct jttype
{
   double x, y;
};

typedef struct frametype
{
    struct jttype joint[MAXJTS];
} FRAMETYPE;

typedef struct frametype FRAME;

struct
  {
    short x, y;           // coordinates
    short left, right;    // buttons
    short x1, y1, x2, y2; // boundaries
  } mouse;

FRAME *frame, *prevframe;
char filename[LENGTH], edtalk[15], array[16][1024];
int skip, numframes, numjoints, framestop, Warr[5];
double ctrlength, cfactor;
static u_short vgar[256], vgag[256], vgab[256];

FRAME *create_frame( void );
int   xmit( char ch );
int   churdy( void );
int   rch( void );
void  init_editor( void );
void  stop( void );
void  play( void );
void  adv( void );
int   response( void ); 
void  status( void );
void  reg_rewind( void );
void  slow_rewind( void );
void  slow_play( void );
void  get_frame_num( void );
void  install_cursor( void );
void  move_mouse( void );
short check_mouse( void );
void  init_dt3852( void );
void  format_setup(void);
void  acquire_setup(u_short *, u_short *, u_short *);
void  view_video( void );
void  find_cfactor( void );
void  DigitizeFrame( void );
void  Digitizeit(int frmcnt,int totjoints,struct nametype jtnames[MAXJTS]);
void  conversions( void );   
void  intro_screen( void );      
void  menu_main( void );
void  setup_menu( void );
void  session_setup( void );
void  namejoints( int njts );
void  save_session( char new_session[8] );
void  save_data( int frmcnt, char type[4] );
