/* Velocity.c
   -----------
   Program calculates velocity given a number of different
   pixel locations.  Currently uses a datafile.

*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

void open_files( void );
void input_data( void );
void calculate( void );
void save_data( void );
void all_done( void );

char rawdatafile[15], newdatafile[15];
int subject, fin, rpe, lane, fields,
    oldrpe, trial, end, x1, x2, pixy, y2;
double pixels, displacement, seconds, velocity,
       cfactor;

FILE *fpRAW, *fpNEW;

main()
{
   int i;

   open_files();
   for( i = 0; i < end; i++)
    {
      input_data();
      calculate();
      save_data();
    }
   all_done();
}

void open_files( void )
{
   printf("Enter the name of the datafile: ");
   scanf( "%12s", &rawdatafile);
   fpRAW = fopen( rawdatafile, "r");
   strcpy( newdatafile, "velocity.dat");
   fpNEW = fopen( newdatafile, "a");
   printf("\nEnter the lane number: ");
   scanf( "%d", &lane );
   if (lane == 5 )
      cfactor = 0.00850;
   else if ( lane == 4 )
      cfactor = 0.00695;
   else if ( lane == 2 )
      cfactor = 0.00577;
   printf("\nEnter the number of records: ");
   scanf( "%d", &end );
   oldrpe = 1;
}

void input_data( void )
{
   fscanf( fpRAW, "%d%d%d%d%d%d%d%d\n", &subject, &fin, &rpe,
                                        &x1, &pixy, &x2, &y2, &fields);
}

void calculate( void )
{
   double xdist, ydist;

   pixels = 0.0;
/*   printf(" values: %d %d %d %d\n", x1, pixy, x2, y2); */
   xdist = (double) ( x2 - x1 );
   ydist = (double) ( y2 - pixy );
/*   printf("xdist & ydist = %d %d\n", xdist, ydist );   */
   pixels += ( xdist*xdist );
/*   printf("Pixels with xdist = %lf\n", pixels );       */
   pixels += ( ydist*ydist );
/*   printf("Pixels with ydist = %lf\n", pixels );       */
   pixels = sqrt( pixels );
/*   printf("Pixels after sqrt = %lf\n", pixels );       */
   pixels = floor( pixels + .5 );
   displacement = (pixels * cfactor);
   seconds = ( fields / 60.0000 );
   velocity = ( displacement / seconds );
   if ( rpe != oldrpe)
      trial = 1;
   else
      trial += 1;
}


void save_data( void )
{
   fprintf( fpNEW, "%02d %02d %02d %02d %01d %03d %03d %03d %03d %03.0lf %01.4lf %01.4lf %02.4lf\n",
                    subject, fin, rpe, trial, lane, x1, pixy, x2, y2,
                    pixels, displacement, seconds, velocity );
   oldrpe = rpe;
}


void all_done( void )
{
   fclose( fpRAW);
   fclose( fpNEW);
   printf("\n ALL DONE.!");
}

