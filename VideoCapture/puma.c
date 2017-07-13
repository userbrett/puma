
/* ----------------------------------------------------------------------

 PUMA.C
 Purdue University Motion Analysis
 Written in Microsoft C version 7
  (should compile under earlier versions)

 This program was written to move images from a VCR through a frame-
 grabber and display them on a second multisync (VGA) monitor.  This is
 currently done in passthru mode.  Within is also the code to mark
 desired locations on the images and then save the coordinates to disk.

 ** In order to provide code that does not infringe upon any existing
    copyrights, some functions that pertain to the frame grabber are
    incomplete or empty.  The remaining code is in tact and can be used
    as a starting point from which to build a system.

 We currently perform the steps the following order.

      1.  Dub SMPTE time code on the audio track and also put
          video time codes on tape
                - to control for any tape slippage
      2.  Establish communications with and initialize the editor & VCR
      3.  Configure the frame grabber
      4.  Create the parameters for the digitization session
      5.  Calculate the conversion factor between image and actual
      6.  Acquire & digitize the video images

 When "marking" points on the image, the ENTER key (or the mouse left
 button) cause the furrent coordinates to be loaded into an array.  To
 do a point over, the F1 key will back up one marker each time it is
 pressed.  To quit, press the ESC key before digitizing the first point
 of an image.  The F4 key is used if the point is hidden
 (x=999, y=999 is inserted).

 This program currently requires the presence of two directories.  Those
 directories are:

                  D:\PUMA\SESSIONS   for SESSION PARAMETERS
                  D:\PUMA\DATA       for DATA FILE OUTPUT

 This program runs in conjunction with the Microsoft "C" public
 domain menu programs:

               menu.c   &&   menu.h

 The fonts (again provide by Microsoft "C") need to be located in the
 directory in which PUMA.EXE is located.

 To use this program, it should be compiled using:

         cl /c /AL puma.c > errors

 To link:

         link /NOD /NOE puma menu,,,llibc7+graphics;


 The major limitations of the code are:

     It is not a turn-key system.  Some of the source code is
     dependant on both the VCR and the frame-grabber used.

     The source code does not include any analytical routines (we
     currently use FORTRAN on a mainframe, therefore the data files
     may look strang.  An additional data file containing converted
     coordinate representations is also created simultaneously).


 Known problems:

     After 5 minutes in pause or slow motion, the VCR we have will
     automatically shut off.  To override this, this program has been
     written so that the VCR will leave PAUSE mode periodically to
     reset the 5 minute counter.  This is done by regulation of the
     amount of frames digitized.  Users are prompted from the setup
     menu for the number of frames they can digitize in less than 5
     minutes.  This number is then used to regulate the intervals.
     (It's kind of hokey, but with our current inability to save images
     onto the frame grabbers memory [no thanks to their technical
     support :{)] it's the best we can do right now.)

     An array has been created to store the coordinates of the
     joints position in the previously digitized image.  The
     placement of the cursor in the current image to that position
     does not work on our system, possibly due to incompatible "cast
     operators".

     Checking the presence of a mouse at the beginning of the program
     causes the mouse to fail on its' future use.



 Final notes:

     Using the VCR in PAUSE mode may damage the VHS tape or the VCR
     heads.  Instead, images should be digitized and stored into memory
     from play mode (if possible) or even from slow motion play (if
     available).

     As a last resort, store the images into memory from PAUSE mode
     (1 field is about 256K, so 80 images is only about 20M). This
     will greatly reduce the time the VCR is in pause mode.


 In distributing the code we have created, we would be very grateful
 for any advice and/or suggestions on solutions to the current problems
 of this system.  We would be especially interested in "C" programs that
 graphically display the "stick-figures" or those that perform any
 kinematic calculations with the data.

Best of Luck in All You Do,

Available contacts:

Dr. Carol Widule     / Hardware
Director - Biomechanics
Purdue University
Bitnet:   Widule@purccvm.bitnet
Internet: Widule@vm.cc.purdue.edu

Krisanne Bothner    / Software
Graduate Student
University of Oregon
Internet: KBothner@oregon.uoregon.edu

Brett Lee           / Software
Graduate Student
Purdue University
Bitnet:   BrettLee@purccvm.bitnet
Internet: BrettLee@vm.cc.purdue.edu


----------------------------------------------------------------- */

#if (_MSC_VER <= 600)
#define _getch     getch
#define _kbhit     kbhit
#endif

#include <malloc.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <graph.h>
#include <memory.h>
#include <process.h>

#include "menu.h"     // Microsoft C public header file

                      // Many of the definitions, which are specific to
                      // the editor we use, were left in to eliminate
                      // confusion in the program
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

                                       // Data Structures
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

                                       // Global Variables
FRAME *frame, *prevframe;
char filename[LENGTH], edtalk[15];
int skip, numframes, numjoints, framestop;
double ctrlength, cfactor;

                                      // Function prototypes
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


//********************************************************
//* Memory allocation....
//********************************************************

FRAME *create_frame( void )
{
   int i;
   FRAME *f;

   f = (FRAME *) malloc( sizeof(FRAME) );
   if( f == NULL )
     {
       printf( "Error:  create_frame()  malloc failed.\n" );
       exit( 1 );
     }
   else
     {
       for( i = 0; i < numjoints; i++)
         {
            f->joint[i].x = 0;
            f->joint[i].y = 0;
          }
     }
   return f;
}


//*******************************************************
//* Mouse control on overlay buffer
//*******************************************************


short check_mouse( void )
{
    regs.x.ax = 0;
    _int86( 0x33, &regs, &regs );
    return( regs.x.ax );
}

                            // Function for image monitor
                            // Dependant on frame grabber
void install_cursor( void )
{
     // get/set config for cursor on image monitor
     // initialize mouse boundary (mouse.x1, .x2, .y1, .y2 )
     // turn cursor off
}


void move_mouse( void )
{
                                    // get mouse coordinates
    regs.x.ax = 11;
    _int86( 0x33, &regs, &regs );
    mouse.x += regs.x.cx;
    mouse.y += regs.x.dx;

              // ensure mouse stays withing screen boundary
    if ( mouse.x < mouse.x1 )
       mouse.x = mouse.x1;
    if ( mouse.x > mouse.x2 )
       mouse.x = mouse.x2;
    if ( mouse.y < mouse.y1 )
       mouse.y = mouse.y1;
    if ( mouse.y > mouse.y2 )
       mouse.y = mouse.y2 ;

                               // move cursor to mouse.x , .y
    (specific to frame grabber)
                               // get the mouse buttons
    regs.x.ax = 3;
    _int86( 0x33, &regs, &regs );
    mouse.left = regs.h.bl & 1;
    mouse.right = (regs.h.bl & 2) >> 1;
}


//*******************************************************
//* Serial port input / output
//*******************************************************


                       //  Sends character over serial port
int xmit( char ch )
{
    register int cnt = 0;

    while (!(_inp(COM1+LSR) & THRRDY) && cnt < 10000)
       cnt++;
    if ( cnt >= 10000)
       return (-1);
    _outp (COM1+THR, ch);
    return(0);
}

/* The editor we use returns a 15 byte response for each command
   it receives.  Before another command can be issues, the response
   must be read.  Even though we directly access the UART and thus
   eliminate the overhead of using the BIOS DOS provides, we are
   still using a "polling" method which causes some characters to
   be lost at 1200 baud.  If you have a problem like this and it is
   important that you read each character, or if you are using a
   baud rate faster than 1200, you will need to use an interrupt
   driven serial port routine (these read up to 19,000 baud easily).
   For information on how to write one, Borlands Turbo C provides
   a public domain program (serial.c && serial.h) which demonstrate
   how to do this.
*/

                         // Read responses from the editor
int response( void )
{
    int i;
    long int cnt = 0;

    for ( i = 0; i < 15; i++ )
     {
        while (!(_inp(COM1+LSR) & DTARDY) && cnt < 150000)
           cnt++;
        if ( cnt < 150000 )
         {
          cnt = 0;
          while (!( edtalk[i] = (_inp(COM1+RDR) & MASK )) && cnt < 10000)
            cnt++;
         }
        else
           {
/*          _clearscreen( _GCLEARSCREEN );
            _settextposition( 6, 0 );
            printf("Editor problem...press ENTER to continue.");
            while (( i = _getch()) != 13);
            _clearscreen( _GCLEARSCREEN );
*/          return -1;
           }
      }

    if (edtalk[0] == '?')
      {
        _clearscreen( _GCLEARSCREEN );
        _settextposition( 6, 0 );
        printf("Editor problem...press ENTER to continue.");
        while (( i = _getch()) != 13);
      }

/*  _settextposition( 6, 0);        // Display on primary VGA
    for ( i = 0; i < 15; i++)
      printf( "%c", edtalk[i]);
*/
    return(0);
}



//*******************************************************
//* Editor control coding....
//*******************************************************

                                      // Initialize the editor
void init_editor( void )
{
    int i, j;
    register int cnt = 0;
                                   // Configure the serial port
    regs.h.ah = 0;
    regs.x.dx = COM1;
    regs.h.al = SETUP;
    _int86( RS232, &regs, &regs);

    for ( i = 0; i < 30000; i++ )
       for ( j = 0; j < 50; j++ );
                                       // Configure the VCR
    xmit( SET_VCR );
    xmit( VCR_TYPE );
    xmit( (char) TERMINATE );
    i = response();

    _clearscreen( _GCLEARSCREEN );
    if (edtalk[0] == '?' )
      {
        printf( "Editor problem, command not understood.");
        printf( "\n\nPress ENTER to continue... ");
        while ((i = _getch()) != 13 );
        _clearscreen( _GCLEARSCREEN );
      }
    else
      {
        printf( "Editor and VCR configuration complete.");
        printf( "\n\nEditor response from configuration: \n" );
        for ( i = 0; i < 15; i++)
            printf("%c", edtalk[i]);

        xmit( STATUS );
        xmit( (char) TERMINATE );
        i = response();

        printf( "\n\nCurrent VCR status: \n");
        for ( i = 0; i < 15; i++)
            printf("%c", edtalk[i]);
        printf( "\n\nProceed to the Frame Grabber configuration.");
        printf( "\n\nPress ENTER to continue...\n\n");
        while ((i = _getch()) !=13 );
        _clearscreen( _GCLEARSCREEN );
      }
}

                         // Find current status of the VCR
void status( void )
{
    int i;

    xmit( STATUS );
    xmit( (char) TERMINATE );
    i = response();
}


                         //  Tell VCR to STOP
void stop( void )
{
    int i;

    xmit( STOP );
    xmit( (char) TERMINATE );
    i = response();
}

                         //   Start VCR
void play( void )
{
    int i;

    xmit( PLAY );
    xmit( (char) TERMINATE );
    i = response();
}
                            // Play forward slowly
void slow_play( void )
{
    int i;

    xmit( SLOWF );
    xmit( (char) TERMINATE );
    i = response();
}

                         // Try to pause the VCR
void pause( void )
{
    int i;

    xmit( PAUSE );
    xmit( (char) TERMINATE );
    i = response();
}

                        //  Advance VCR one field
void adv( void )
{
    int i;

    xmit( FADV );
    xmit( (char) TERMINATE );
    i = response();
}

                      // Rewind tape slowly
void slow_rewind( void )
{
   int i;

   xmit( SLOWR );
   xmit( (char) TERMINATE );
   i = response();
}


                      // Rewind tape at normal speed
void reg_rewind( void )
{
    int i;

    xmit( RW );
    xmit( (char) TERMINATE );
    i = response();
}

                     // Find the current frame number
void get_frame_num( void )
{
   int i;

   xmit( FRAMENUM );
   xmit( (char) TERMINATE );
   i = response();
}



// ***********************************************************
// **************** Frame Grabber Routines ****************
// ***********************************************************

                      //  Initialize the Frame Grabber

void init_dt3852( void )
{
  int c;

  _clearscreen( _GCLEARSCREEN );

                   //  Frame Grabber Initialization (of course)
                   //  specifics removed

  _clearscreen( _GCLEARSCREEN );
  printf( "\nFrame Grabber successfully configured.");
  printf( "\n\nProceed to Session Setup.");
  printf( "\n\nHit ENTER to continue...");
  while (( c = _getch()) != 13);
  _clearscreen( _GCLEARSCREEN );
}


                    // Sets correct inserts and resets for this VCR

void format_setup( void )
{
                // Again removed
}


                  // Set up the LUT's, acquire and display buffers

void acquire_setup(vgared, vgagreen, vgablue)
{
                  // Yep, once again
}


                          // Plays VHS thru image monitor
void view_video( void )
{
    int i;

    _clearscreen( _GCLEARSCREEN );
    _settextposition( 10, 5 );
    _outtext( " Press ESC to stop.");
    play();

                    // Here is where we turn the passthru mode on

    while( _getch() != 27);
    stop();

                    // And now off

    _clearscreen( _GCLEARSCREEN );
}


// ***********************************************************
// ***************** Digitization of Images ******************
// ***********************************************************


                            // Meat of the PUMA program

void DigitizeFrame( void )
{
    int i, j, n, c, dig_choice, done, frmcnt = 0;
    short v;
    char frame_num[15];
    char ftn[] = { ".FTN" };
    char dat[] = { ".DAT" };

    play();

               // Passthru mode on again

    _displaycursor( _GCURSOROFF );
    _setvideomode( _VRES16COLOR );
    _clearscreen( _GCLEARSCREEN );
    _registerfonts( "*.FON" );
    _setfont( "t'helv'h125w80b" );
    _moveto( 70, 130 );
    _settextcolor( 14 );
    _setcolor( 14 );
    _outgtext( "PUMA DIGITIZATION" );
    _moveto( 95, 250 );
    _setfont( "t'tms rmn'h20w12" );
    _outgtext( "Position tape at desired location." );
    _moveto( 95, 285 );
    _outgtext( "VCR must be in PAUSE mode during acquisition." );
    _setfont( "t'tms rmn'h15w8" );
    _moveto( 350, 455 );
    _outgtext( "Press ENTER when ready..." );
    while ((c = _getch()) != 13);
    _displaycursor( _GCURSORON );
    _setvideomode( _DEFAULTMODE );
    _clearscreen( _GCLEARSCREEN );
    _unregisterfonts();
    frame = create_frame();
    prevframe = create_frame();
   do
     {
          // This is where we stop the VCR before 5 minutes expires
      if ((frmcnt > 0 ) && ((frmcnt % framestop ) == 0 ))
        {
          status();     // Gets current time code of tape
          for ( i = 0; i < 15; i ++ )
             frame_num[i] = edtalk[i];
          _clearscreen( _GCLEARSCREEN );
          printf("STOPPING THE TAPE.");
          printf("\n\nCHECK TAPE FOR EXACT POSITION.\n\n");
          for ( i = 4; i < 11; i++ )
            {
              if ((i % 2) == 1 )
                printf(":");
              printf( "%c", frame_num[i] );
            }
          printf("\n\nPress <F1> to continue.");
          while ( _getch() != 59 );
          _clearscreen( _GCLEARSCREEN );
          for ( i = 0; i < 30000; i++ )
             for ( j = 0; j < 50; j++ );
          stop();
          for ( i = 0; i < 30000; i++ )
            for ( j = 0; j < 75; j++ );
          reg_rewind();
          for ( i = 0; i < 30000; i++ )
            for ( j = 0; j < 250; j++ );
          stop();
          _clearscreen( _GCLEARSCREEN );
          printf( "Position the tape at frame\n\n");
          for ( i = 4; i < 11; i++ )
            {
              if ((i % 2) == 1 )
                printf(":");
              printf( "%c", frame_num[i] );
            }
          printf( "\n\nin pause mode.");
          printf( "\n\nHit <ENTER> to continue.");
          play();
          for ( i = 0; i < 30000; i++ )
            for ( j = 0; j < 650; j++ );
          pause();
          while ( _getch() != 13 );
        }
                      // Now, digitization continues
      adv();
      _clearscreen( _GCLEARSCREEN );
      _settextposition( 20, 16 );
      _settextcolor( 14 );
      printf( "Displaying field %d", frmcnt );
      _settextposition( 22, 16 );
      printf( "ENTER = Digitize   <or>   ESCAPE = Quit ");
      while(((c = _getch()) != 27) && (c != 13));
      if ( c == 27 )
        {
          done = YES;
          break;
        }
      if ( c == 13 )
        {
          _clearscreen (_GCLEARSCREEN );
          Digitizeit( frmcnt, numjoints, jtnames );
          save_data( frmcnt, ftn );
          conversions();
          save_data( frmcnt, dat );
          frmcnt++;
          _clearscreen( _GCLEARSCREEN );
          printf( "CURRENTLY ADVANCING VIDEO TAPE...");
          for ( n = 0; n < skip; n++ )
            {                     // FIELD SKIPS BETWEEN ACQUIRES
              _settextposition( 3, 0 );
              printf( "\rField skip count is : %02d", n + 1 );
              adv();
              for ( i = 0; i < 30000; i++ )
                for ( j = 0; j < 60; j++ );
            }
          done = NO;
         }
      } while (done == NO );

      frmcnt--;
      free( prevframe );
      free( frame );

          // Turning passthru mode off again

      _displaycursor( _GCURSOROFF );
      _setvideomode( _VRES16COLOR );
      _clearscreen( _GCLEARSCREEN );
      _settextcolor( 14 );
      _setcolor( 14 );
      _registerfonts( "*.FON" );
      _moveto( 70, 120 );
      _setfont( "t'helv'h100w80b" );
      _outgtext( "DIGITIZATION COMPLETE.");
      _moveto( 100, 200 );
      _setfont( "t'tms rmn'h30w15");
      _outgtext( "Press ENTER to return to ");
      _moveto( 100, 225 );
      _outgtext(" the main menu.");
      while (( c = _getch()) != 13);
      stop();
      _unregisterfonts();
      _displaycursor( _GCURSORON );
      _setvideomode( _DEFAULTMODE );
      _clearscreen( _GCLEARSCREEN );
}

           // This function handles the digitization of the
           // images on the image monitor

void Digitizeit( int framecnt, int totjoints, struct nametype jtnames[MAXJTS] )
{
    int c, pointdone;
    unsigned short jtcnt = 0;

           // Turn cursor off for display of current x/y coord's
    _settextcursor( 0x2000 );

                            // Passthru mode on again

                          // move mouse to center of the screen

    do
    {
     pointdone = NO;
     if( framecnt >= 1 )         // move mouse to previous location
                                 // this doesn't seem to work
       set_curs_xy(((short)((float)prevframe->joint[jtcnt].x)),
                   ((short)((float)prevframe->joint[jtcnt].y)));
     _settextposition( 20, 40 );
     _outtext( "Next joint: ");
     _settextposition( 20, 53 );
     printf("%-12s", jtnames[jtcnt].name );
     _settextposition( 22, 40 );
     _outtext( "ENTER to Digitize        F1 to Backup");
     _settextposition( 24, 40 );
     _outtext( "F4 if point Hidden");
     _settextposition( 0, 54 );
     _outtext( "  X       Y   ");
     while (!_kbhit())
       {
        move_mouse();
        _settextposition( 2, 55 );
        printf( "%03d     %03d", mouse.x, mouse.y);
       }
     c = _getch();
     if (c == 13)
       {
         if ( framecnt >= 1 )
           {
             prevframe->joint[jtcnt].x = frame->joint[jtcnt].x;
             prevframe->joint[jtcnt].y = frame->joint[jtcnt].y;
           }
         frame->joint[jtcnt].x = (double) mouse.x;
         frame->joint[jtcnt].y = (double) mouse.y;
         jtcnt++;
         continue;
       }
     if ( c == 59 )
       {
         if (jtcnt == 0 )
           continue;
         else
           {
            jtcnt--;       // REDIGITIZE LAST POINT
            continue;
           }
       }
     if (c == 62 )
       {
         frame->joint[jtcnt].x = 999;
         frame->joint[jtcnt].y = 999;
         jtcnt++;
         continue;
       }
   } while ( jtcnt < totjoints );

                          // Last time, passthru mode off



   _settextcursor( 0x0707 );    // Turn cursor back on
   _clearscreen( _GCLEARSCREEN );
}


// ***********************************************************
// ****************** DATA MANIPULATIONS *********************
// ***********************************************************

                           // Find the conversion factor

void find_cfactor( void )
{
    int i, c, j, v;
    double n = 0.0;
    char ch;
    struct nametype sides[] = { "LEFT SIDE", "RIGHT SIDE" };
    long int xdist, ydist;
    double pixdist, sqdist, sum;
    sum = i = j = 0;

    play();

                         // I lied, passthru mode on again

    _displaycursor( _GCURSOROFF );
    _setvideomode( _VRES16COLOR );
    _setcolor( 14 );
    _clearscreen( _GCLEARSCREEN );
    _registerfonts( "*.FON" );
    _setfont( "t'helv'h125w80b" );
    _moveto( 0, 85 );
    _outgtext( "CONVERSION FACTOR DETERMINATION" );
    _moveto( 100, 230 );
    _setfont( "t'tms rmn'h25w10" );
    _outgtext( "Position the tape at the");
    _moveto( 100, 260 );
    _outgtext( "measurement standard.  The VCR" );
    _moveto( 100, 290 );
    _outgtext( "must be in PAUSE mode to begin");
    _moveto( 100, 320 );
    _outgtext( "acquisition.");
    _setfont( "t'tms rmn'h15w8" );
    _moveto( 325, 420 );
    _outgtext( "Press ENTER when ready..." );
    while (( c = _getch()) != 13);

    adv();
    _clearscreen( _GCLEARSCREEN );
    _setfont( "t'tms rmn'h125w80b" );
    _moveto( 70, 200 );
    _outgtext( "ACQUISITION COMPLETE." );
    _setfont( "t'tms rmn'h15w8" );
    _moveto( 350, 420 );
    _outgtext( "Press ENTER to continue..." );
    while (( c = _getch()) != 13);
    _clearscreen( _GCLEARSCREEN );
    _settextcursor( _GCURSORON );
    _unregisterfonts();
    _setvideomode( _DEFAULTMODE );
    _settextcolor( 14 );

    frame = create_frame();
    while( n < 49 || n > 57 )
     {
       _settextposition( 12, 5 );
       _outtext( "Number of times to digitize control rod (1-9): ");
       n = _getch();
     }
    n = n - 48;               // Converts from ASCII to decimal.
    _clearscreen( _GCLEARSCREEN );

    for ( i = 0; i < n; )
       {
         _clearscreen( _GCLEARSCREEN );
         Digitizeit( -1, 2, sides );
         xdist = (long int) ((frame->joint[1].x) - (frame->joint[0].x));
         ydist = (long int) ((frame->joint[1].y) - (frame->joint[0].y));
         sqdist = (double) ((xdist*xdist) + (ydist*ydist));
         if ( sqdist == 0 )
           {
            if ( j > 0 )
               break;
            j++;
            _clearscreen( _GCLEARSCREEN );
            _settextposition( 12, 10 );
            _outtext( "Control rod digitized to ZERO length.");
            _settextposition( 14, 10 );
            _outtext( "Redigitize the control rod.");
            _settextposition( 30, 40 );
            _outtext( "Press ENTER to continue...");
            while ((c = _getch()) != 13);
            _clearscreen( _GCLEARSCREEN );
           }
         else
           {
              sum += sqdist;
              i++;
           }
        }

               // Guess what, passthru mode off again

    sqdist = (sum / n);
    stop();

    if (sqdist > 0 )
       {
        _clearscreen( _GCLEARSCREEN );
        pixdist = sqrt( sqdist );
        cfactor = (ctrlength/pixdist);
        _settextposition( 10, 2 );
        _outtext( "Conversion factor is: ");
        _settextposition( 10, 25 );
        printf( "%01.5lf", cfactor );
        _settextposition( 12, 2 );
        _outtext( "You may want to note this for future sessions...");
        _settextposition( 30, 40 );
        _outtext( "Press ENTER to continue ...");
        while (( c = _getch()) != 13);
       }
    else
       {
        cfactor = sqdist = 1.0;
        _clearscreen( _GCLEARSCREEN );
        _settextposition( 12, 12 );
        _outtext( " No conversion factor calculated.");
        _settextposition( 30, 40 );
        _outtext( "Press ENTER to continue...");
        while (( c = _getch()) != 13);
       }
    free( frame );
    _clearscreen( _GCLEARSCREEN );
}


// *******************************************
// CONVERT TO REAL WORLD COORDINATES
// *******************************************

void conversions( void )
{
    int i;
    for( i = 0; i < numjoints; i++ )
      {
         frame->joint[i].x = ( cfactor )*( frame->joint[i].x );
         frame->joint[i].y = ( cfactor )*( frame->joint[i].y );
      }
}


// *******************************************
// SAVE DATA / SESSIONS TO HARD DISK
// *******************************************


void save_data( int frmcnt, char type[4] )
{
    int hold, length, i = 0;
    double speed;
    char datafile[LENGTH], temp[20];
    FILE *fpDATA;

    _setcolor( 0 );
    _settextcolor( 14 );
    strcpy( datafile, "D:\\PUMA\\DATA\\");
    length = strlen( filename );
    strncat( datafile, filename, length + 1);
    strncat( datafile, type, 4 );
    speed = ((skip + 1) / 60.0);

    if (type[1] == 'D')
      {
       if ( frmcnt == 0)
         {
          while ((fpDATA = fopen( datafile, "w" )) == NULL)
           {
              _clearscreen( _GCLEARSCREEN );
              _settextposition( 12, 12);
              _outtext( "No filename exists.");
              _settextposition( 14, 12);
              _outtext( "Run Session Setup before digitizing.");
              _settextposition( 24, 23);
              _outtext( "Press ENTER to start over..." );
              while (( i = _getch()) != 13);
              exit( 0 );
           }
          fprintf( fpDATA, "%d\n", numjoints );
          while ( i < numjoints )
            {

              fprintf( fpDATA, "%06.3lf %06.3lf   ", frame->joint[i].x,
                                                     frame->joint[i].y );
              i++;
            }
          fprintf( fpDATA, "\n");
         }
       else
         {
           fpDATA = fopen( datafile, "a" );
           while ( i < numjoints )
            {
              fprintf( fpDATA, "%06.3lf %06.3lf   ", frame->joint[i].x,
                                                     frame->joint[i].y );
              i++;
            }
           fprintf( fpDATA, "\n");
         }
       }
    else if ( type[1] == 'F')
     {
       if ( frmcnt == 0 )
          {
           fpDATA = fopen( datafile, "w" );
           fprintf( fpDATA, "%s", filename );        // NAME
           fprintf( fpDATA, "\n\n8");                // NUMFRA & NX
           fprintf( fpDATA, "\n%01.5lf", cfactor );  // CONVERSION
           speed = ((skip + 1) / 60.0);
           fprintf( fpDATA, "\n%01.5lf\n\n", speed); // FILM SPEED
           fprintf( fpDATA, "\n%03d", frmcnt );
           while ( i < numjoints )
             {
               if ( (i % 4) == 0)
                  fprintf( fpDATA, "\n");
               fprintf( fpDATA, "%03.0lf %03.0lf   ", frame->joint[i].x,
                                                      frame->joint[i].y);
               i++;
              }
          }
        else
          {
            fpDATA = fopen( datafile, "a");
            fprintf( fpDATA, "\n%03d", frmcnt );
            while ( i < numjoints )
              {
                if ( (i % 4) == 0)
                   fprintf( fpDATA, "\n");
                fprintf( fpDATA, "%03.0lf %03.0lf   ", frame->joint[i].x,
                                                   frame->joint[i].y );
                i++;
              }
            }
         }
     fclose( fpDATA );
}



void save_session( char new_session[8] )
{
    char savesess[LENGTH], temp[20];
    FILE *fpSESS;
    int i, length;

    _setcolor( 0 );
    _settextcolor( 14 );
    strcpy( savesess, "D:\\PUMA\\SESSIONS\\" );
    length = strlen( new_session );
    strncat( savesess, new_session, length + 1 );
    strncat( savesess, ".SES", 4 );

    while ((fpSESS = fopen( savesess, "w" )) == NULL)
       {                    // Only run if path non-existant
         _clearscreen( _GCLEARSCREEN );
         _settextposition( 12, 12);
         _outtext( "INVALID DATA PATH / FILENAME EXISTS.");
         _settextposition( 13, 12);
         _outtext( "Enter new path AND datafile (w/o an extension). ");
         _settextposition( 15, 12 );
         _outtext( "Use TWO backslashes '\\' for each backslash '\'.");
         _settextposition( 17, 12 );
         gets( temp );
         strcpy( savesess, temp );
         strncat( savesess, filename, length + 1 );
         strncat( savesess, ".SES", 4 );
       }
       fprintf( fpSESS, "%02.5lf\n%d\n%d\n%02.4lf\n%d",
                  cfactor, framestop, skip, ctrlength, numjoints );
       for( i = 0; i < numjoints; i++ )
             fprintf( fpSESS, "\n%s", jtnames[i].name );
       fclose( fpSESS );
}

// ********************************************
// * SETUP THE DIGITIZATION SESSIONS
// ********************************************


void namejoints( int njts )
{
  char ch;
  int i, j, done;

  for( done = NO; done != YES; )
    {
    _setcolor( 0 );
    _settextcolor( 14 );
    _clearscreen( _GCLEARSCREEN );
    _settextposition( 3, 18 );
    _outtext( "NAME JOINTS IN ORDER OF DIGITIZATION (MAX = 20)" );
    _settextposition( 4, 22 );
    _outtext( "(Maximum length of 12 characters)" );

    j = 0;
    for( i = 0; i < njts; i++)
      {
       if( i >= 10 )
         {
           _settextposition( 8+j, 40 );
           j++;
         }
       else
       _settextposition( 8+i, 10 );
       printf( "Name of joint %d:  ", i + 1 );
       scanf( "%12s", &jtnames[i].name );
       _strupr( jtnames[i].name );
      }

    _settextposition( 22, 23 );
    _outtext( "Are these joints correct ? <Y or N> ");
    do
     {
       ch = (__toascii(toupper(_getch())));
     } while ( ch != 'Y' && ch != 'N');
    if( ch == 'Y' )
         done = YES;
    else
         done = NO;
    }
    _settextposition( 24, 23);
    _outtext( "Press ENTER to continue...." );
    while (( i = _getch()) != 13);
    _clearscreen( _GCLEARSCREEN );
}


void session_setup()
{
    int i, c, frmcnt, length, done = NO;
    char reply, sessresp, ch, units;
    char infile[8];
    char newsession[8];
    char sessfile[LENGTH];
    FILE *fpSESS;

    while ( !done )
    {
      _setcolor( 0 );
      _settextcolor( 14 );
      _clearscreen ( _GCLEARSCREEN );
      _settextposition( 7, 20 );
      _outtext( "SETUP PARAMETERS FOR THIS SESSION" );
      _settextposition( 9, 12 );
      _outtext( "Create or Load a session file? <c/l> " );
      sessresp = toupper(_getche());
      if( sessresp  == 'L')
        {
         _settextposition( 11, 12);
         _outtext( "Load which session file: " );
         scanf( "%s", &infile );
         length = strlen( infile );
         strcpy( sessfile, "D:\\PUMA\\SESSIONS\\");
         strncat( sessfile, infile, length + 1 );
         strcat( sessfile, ".SES" );
         if ((fpSESS = fopen( sessfile, "r" )) == NULL)
            {
              _clearscreen( _GCLEARSCREEN );
              _settextposition( 11, 12 );
              _outtext("File not found.");
              _settextposition( 30, 40 );
              _outtext("Press ENTER to continue.");
              while(( c = _getch()) != 13);
              done = NO;
              continue;
            }
         else
            {
              _settextposition( 12, 12 );
              _outtext( "Name of datafile: ");
              scanf( "%s", &filename);
              fscanf( fpSESS, "%lf\n%d\n%d\n%lf\n%d",
                  &cfactor, &framestop, &skip, &ctrlength, &numjoints );
              for( i = 0; i < numjoints; i++ )
                  fscanf( fpSESS, "\n%s", &jtnames[i].name );
              fclose( fpSESS );
              done = YES;
              break;
            }
         }
       else if ( sessresp == 'C' )
         {
             _settextposition( 11, 12);
             _outtext( "Name of new session file: ");
             scanf( "%8s", &newsession );
             _settextposition( 12, 12);
             _outtext( "Name of datafile for this session: " );
             scanf( "%8s", &filename );
             _settextposition( 13, 12);
             _outtext( "Number of fields to skip: ");
             scanf( "%d", &skip);
             _settextposition( 14, 12 );
             _outtext( "TOTAL number of points to digitize: " );
             scanf( "%d", &numjoints );
             _settextposition( 15, 12 );
             _outtext( "Length of the control rod: " );
             scanf( "%lf", &ctrlength );
             _settextposition( 16, 12 );
             _outtext( "Units of measure: " );
             _settextposition( 17, 14 );
             _outtext( " m = meters   c = centimeters ");
             _settextposition( 18, 14 );
             _outtext( " y = yards    f = feet   i = inches ");
             _settextposition( 16, 30 );
             units = toupper(_getche());
             _settextposition( 20, 12 );
             _outtext( "Are the above parameters correct? <Y or N>" );
             do
               {
                 reply = (__toascii(toupper(_getch())));
               } while( reply != 'Y' && reply != 'N');
             if ( reply == ('N'))
                {
                 done = NO;
                 continue;
                }
             else
                 done = YES;
         _settextposition( 22, 12 );
         _outtext( "Press ENTER to continue. ");
         while (( c = toupper(_getch())) != 13 );
         if ( units == 'C' )
               ctrlength = (ctrlength / 100);
         else if ( units == 'F' )
               ctrlength = (ctrlength * .3048);
         else if ( units == 'Y' )
               ctrlength = (ctrlength * .9144);
         else if ( units == 'I' )
               ctrlength = (ctrlength / 39.37);
         namejoints( numjoints );
       }
    }
    _clearscreen( _GCLEARSCREEN );
    _settextposition( 5, 12 );
    _outtext( "Conversion factor choices: ");
    _settextposition( 7, 15 );
    _outtext( "N:  Calculate NEW conversion factor.");
    _settextposition( 8, 15 );
    _outtext( "K:  Enter KNOWN conversion factor.");
    _settextposition( 9, 15 );
    _outtext( "C:  Keep CURRENT conversion factor.");
    _settextposition( 11, 20 );
    _outtext( "Selection < N K C > : ");
    do
     {
       reply = (__toascii(toupper(_getch())));
     } while( reply != 'N' && reply != 'K' && reply != 'C');
    if ( reply == 'N' )
      find_cfactor();
    if ( reply == 'K' )
      {
       _settextposition( 13, 20 );
       _outtext( "Enter conversion factor: ");
       scanf( "%lf", &cfactor );
      }
    _clearscreen( _GCLEARSCREEN );
    _settextposition( 5, 12 );
    _outtext( "Will you ENTER or CHANGE the number of images");
    _settextposition( 7, 12 );
    _outtext( "to be digitized in each 5 minute interval ? ");
    _settextposition( 11, 12 );
    _outtext( "You MUST answer YES if creating NEW Session. ");
    _settextposition( 14, 12 );
    _outtext( "Selection < Y or N > : ");
    do
      {
        reply = (__toascii(toupper(_getch())));
      } while( reply != 'N' && reply != 'Y');
    if ( reply == 'Y')
      {
        _settextposition( 17, 12 );
        _outtext( "Enter the number to digitize: ");
        scanf( "%d", &framestop );
      }
   if (sessresp == 'C')
      save_session( newsession );
   else
      save_session( infile );
   _clearscreen( _GCLEARSCREEN );
}


void setup_menu( void )
{
  int setup_choice;
  int done;

     ITEM SetupMenu[] =               // Menu() DATA STRUCTURE
    {
     { 0, "VCR & EDITOR" },
     { 0, "FRAME GRABBER" },
     { 0, "MAIN MENU" },
     { 0, "" }
    };

   enum SChoice               // ENUMERATE ACQUISITION OPTIONS
    {
       EDITOR,
       FRAME,
       RETURN
     };

    do
    {
      setup_choice = Menu( 12, 54, SetupMenu, 0 );
      switch( setup_choice )
      {
       case EDITOR:
         init_editor();
         done = NO;
         continue;
       case FRAME:
         init_dt3852();
         done = NO;
         continue;
       case RETURN:
         _clearscreen( _GCLEARSCREEN );
         done = YES;
         break;
       }
     } while (!done);
}


void menu_main( void )
{
    int main_choice = 0;
    int done;

    ITEM MainMenu[] =              // Menu() DATA STRUCTURE
    {
     {  5, "INIT HARDWARE"},
     {  6, "SETUP SESSION"},
     {  0, "CONVERSION FACTOR"},
     {  5, "VIEW TAPE" },
     {  0, "DIGITIZE VIDEO" },
     {  0, "QUIT" },
     {  0, "" }
     };

     enum CHOICES               // ENUMERATE MAIN MENU OPTIONS
     {
     HARDWARE,
     SESSION,
     CONVERSION,
     VIEW,
     DIGITIZE,
     QUIT
     };

    _clearscreen( _GCLEARSCREEN );
    do
    {
      main_choice = Menu( 10, 40, MainMenu, 0 );
      switch( main_choice )
        {
         case HARDWARE:
            setup_menu();
            done = NO;
            continue;
         case SESSION:
            session_setup();
            done = NO;
            continue;
         case CONVERSION:
            find_cfactor();
            done = NO;
            continue;
         case VIEW:
            view_video();
            done = NO;
            continue;
         case DIGITIZE:
            DigitizeFrame();
            done = NO;
            continue;
         case QUIT:
            stop();
            term_tiga;
            _clearscreen( _GCLEARSCREEN );
            done = YES;
        }
    }
    while (!done);
}


void intro_screen( void )
{
    int c;
    _displaycursor( _GCURSOROFF );
    _setvideomode( _VRES16COLOR );
    _setbkcolor( 0 );
    _settextcolor( 14 );
    _clearscreen( _GCLEARSCREEN );
    if( _registerfonts( "*.FON" ) < 0)
     _outtext( "Can't find the fonts..." );
    else
     _setfont( "t'roman'h200w100" );
    _setcolor( 14 );
    _moveto( 70, 100 );
    _outgtext( "PUMA" );
    _setfont( "t'helv'h100w80b" );
    _moveto( 130, 300);
    _outgtext( "PURDUE UNIVERSITY" );
    _moveto( 132, 320 );
    _outgtext( " MOTION ANALYSIS " );
    _setfont( "t'tms rmn'h15w8" );
    _moveto( 15, 380 );
    _outgtext( "Written by:");
    _moveto( 45, 400 );
    _outgtext( "Krisanne Bothner &");
    _moveto( 45, 420 );
    _outgtext( "Brett Lee");
    _moveto( 350, 420 );
    _outgtext( "Press ENTER (or Mouse Left) to continue..." );
    while ((c = _getch()) != 13 );
    _displaycursor( _GCURSORON );
    _unregisterfonts();
    _setvideomode( _DEFAULTMODE );
    _clearscreen( _GCLEARSCREEN );
}


// *************************************************************
// *************************************************************
// *  MAIN PROGRAM
// *
// *  Control of menus and session parameters, as well as saving
// *  and retrieving of both data and session settings
// *************************************************************
// *************************************************************

void main(void)
{
    int c;
    intro_screen();
                               // causes mouse crash
/*
    if(!check_mouse())
    {
        _clearscreen(_GCLEARSCREEN );
        printf("\nMouse driver not installed.");
        printf("\nPress enter to exit.");
        while((c = _getch()) != 13 );
        exit( 0 );
      }
    else
*/
    menu_main();
}
