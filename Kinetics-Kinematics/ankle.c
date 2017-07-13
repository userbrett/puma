/* Ankle program.  
   ------------------
   Program prompts for datafile name, angle and distance to ankle.
   Calculates x,y coordinates of non-existant ankle.

   ** Should error check:
      USE:   ( distance2 = xdist^2 * ydist^2 )
             ( error     = distance2 - distance )
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <alloc.h>

#define PI    3.1415927

void open_files( void );
void input_data( void );
void convert( void );
void calculate( void );
void save_data( void );
void all_done( void );

int n, numframes, end;
char datafile[8];
double deg_alpha, distance, anklex, ankley;

struct jttype
{
   double x, y;
};

typedef struct frametype
{
   struct jttype joint[5];
} FRAMETYPE;

typedef struct frametype FRAME;

FRAME *f;

FILE *fpSESS, *fpNEW;

main()
{
   open_files();
   for( n = 0; n < end; n++)
    {
      input_data();
      convert();
      calculate();
      save_data();
    }
   all_done();
}


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
       for( i = 0; i < 5; i++)
         {
            f->joint[i].x = 0;
            f->joint[i].y = 0;
          }
     }
   return f;
}


void open_files( void )
{
   int i, length, fin;
   char sessfile[28], newdatafile[28];

   printf("Enter the name of the datafile (no extension): ");
   scanf( "%8s", &datafile);
   length = strlen( datafile );
   strcpy (sessfile, "F:\\FP_DIG.DAT\\");
   strncat (sessfile, datafile, length + 1 );
   strcat (sessfile, ".DAT");
   if ((fpSESS = fopen( sessfile, "r")) == NULL )
      printf("\nDatafile not found.");
   f = create_frame();
   fscanf( fpSESS, "%d\n", &numframes);
   strcpy ( newdatafile, "F:\\FP_DIG.DAT\\");
   strncat ( newdatafile, datafile, length + 1 );
   strcat ( newdatafile, ".DTA");
   if ((fpNEW = fopen( newdatafile, "a")) == NULL)
      printf("\nCannot create new datafile");
   printf("\nEnter the angle (in degrees): ");
   scanf( "%lf", &deg_alpha );
   printf("\nEnter the distance (in centimeters): ");
   scanf( "%lf", &distance );
   printf( "\nEnter the number of records: ");
   scanf( "%d", &end );
   printf( "\nEnter the fin number: ");
   scanf( "%d", &fin );

            /* Converting distance from cm to pixels. */
   distance = (( distance / 100 ) / 0.00318 );

                       /* Saving initial info to file */
   fprintf( fpNEW, "%s\n%d\n%d\n", datafile, end, fin );

}

void input_data( void )
{
   int i = 0;
   fscanf( fpSESS, "\n%lf%lf", &f->joint[i].x, &f->joint[i].y);
   for ( i = 1; i < numframes; i++ )
      {
      if ( (i % 4) == 0)
         fscanf( fpSESS, "\n%lf%lf", &f->joint[i].x, &f->joint[i].y);
      else
         fscanf( fpSESS, "%lf%lf", &f->joint[i].x, &f->joint[i].y);
      }
/*   printf( "\n" );
   for ( i = 0; i < numframes; i++)
      printf( "%5.3lf %6.3lf  ", f->joint[i].x, f->joint[i].y );
   printf( "\n" ); */
}


void convert( void )
{
   int i;
   double cfactor = 0.00318;

/* Loop will convert from real world to pixel (640*480) coord's. */

   for ( i = 0; i < numframes; i++ )
     {
       f->joint[i].x = floor(( f->joint[i].x / cfactor) + 0.5);
       f->joint[i].y = floor(( f->joint[i].y / cfactor) + 0.5);
     }

/* Loop will move origin from upper left to lower left. */

   for ( i = 0; i < numframes; i++)
      f->joint[i].y = ( 480 - f->joint[i].y );
}


void calculate( void)
{
   int temp;
   double x, y, xdist, ydist, pi, deg_gamma,
             rad_gamma, rad_alpha, rad_beta, deg_beta;

   anklex = ankley = xdist = ydist = 0.0;


/* Finding the change in x and y */

   x = ( f->joint[0].x - f->joint[1].x );
   y = ( f->joint[0].y - f->joint[1].y );

/* Converting (from degrees to radians) the orientation
                       of the ankle with respect to the fin */
   rad_alpha = (( deg_alpha / 180.0 ) * PI );

/* Finding the orientation of the fin */

   if ( x == 0.0 )
      rad_beta = ( PI / 2 );
   else
      rad_beta = atan2 ( y, x );

/* Finding the absolute orientation of the ankle */

   if ( x == 0.0 )     /* Fin is perpendicular */
     {
      rad_gamma = ( rad_beta - rad_alpha );
      xdist = floor(( distance * cos( rad_gamma )) + 0.5 );
      ydist = floor(( distance * sin( rad_gamma )) + 0.5 );
      anklex = ( f->joint[0].x + xdist );
      ankley = ( f->joint[0].y + ydist );
     }
   else if (( x > 0 ) && ( y < 0 ))       /* Quadrant 2 */
     {
      rad_gamma = rad_alpha - rad_beta;
      xdist = floor(( distance * cos( rad_gamma )) + 0.5 );
      ydist = floor(( distance * sin( rad_gamma )) + 0.5 );
      anklex = ( f->joint[0].x + xdist );
      ankley = ( f->joint[0].y - ydist );
     }
   else if (( x > 0 ) && ( y > 0 ))  /* Quadrant 3 */
     {
      rad_gamma = rad_beta - rad_alpha;
      if ( rad_gamma == 0 )
        {
         xdist = floor(( distance * cos( rad_alpha)) + 0.5 );
         anklex = ( f->joint[0].x + xdist );
         ankley = ( f->joint[0].y );
        }
      else
        {
         xdist = floor(( distance * cos( rad_gamma )) + 0.5 );
         ydist = ( distance * sin( rad_gamma ));
         anklex = ( f->joint[0].x + xdist );
         ankley = floor(( f->joint[0].y + ydist ) + 0.5);
        }
     }
   else if (( x < 0 ) && ( y > 0 ))  /* Quadrant 4 */
     {
      rad_gamma = ( PI + rad_beta - rad_alpha );
      xdist = floor(( distance * cos( rad_gamma )) + 0.5 );
      ydist = floor(( distance * sin( rad_gamma )) + 0.5 );
      anklex = ( f->joint[0].x - xdist );
      ankley = ( f->joint[0].y - ydist );
     }
   else if (( x < 0) && ( y < 0 ))  /* Quadrant 1 */
     {
      printf( "\n\nERROR IN FIN ORIENTATION !! (Line = %d)\n\n", n);
      getchar();
      exit( 1 );
     }
/*
   deg_beta = ( rad_beta * 180 / PI );
   deg_gamma = ( rad_gamma / PI * 180 );
   printf("\nx & y = %7.1lf, %7.1lf", x, y);
   printf("\nAlpha = %6.4lf radians, or %5.2lf degrees.",
                                       rad_alpha, deg_alpha);
   printf("\nBeta = %6.4lf radians, or %5.2lf degrees.",
                                       rad_beta, deg_beta);
   printf( "\nGamma = %6.4lf radians, or %5.2lf degrees.",
                                   rad_gamma, deg_gamma );
   printf("\nxdist = %lf, \nydist = %lf", xdist, ydist );
*/

}


void save_data( void )
{
   int i;

   fprintf( fpNEW, "%4.0lf%5.0lf", anklex, ankley );
   for ( i = 0; i < numframes; i++ )
      fprintf( fpNEW, "%6.0lf%5.0lf", f->joint[i].x, f->joint[i].y );
   fprintf( fpNEW, "\n" );
}

void all_done( void )
{
   fclose( fpSESS);
   fclose( fpNEW);
   free ( f );
   printf("\nALL DONE.!\n\n");
}
