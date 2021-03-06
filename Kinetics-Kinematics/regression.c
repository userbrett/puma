/* Regression Program.
   ------------------
   Program calculates the torque around the ankle based on
   the regression parameters provided.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <alloc.h>

void open_files( void );
void read_data( void );
void input_data( void );
void calculate( void );
void save_data( void );
void all_done( void );

int n, term, end, fin, init, rel[40];
double yint, reg, torque[40], angle[40];

FILE *fpDATA, *fpNEW;

main()
{
   open_files();
   for( n = 0; n < term; n++)
    {
     printf("\n%d ", n+1 );
     input_data();
     calculate();
     save_data();
     printf("%d", n+1 );
    }
   all_done();
}


void open_files( void )
{

   char datafile[28], newdatafile[28];

   strcpy (datafile, "F:\\TORQUE.DAT\\FINGRAPH.DAT");
   if ((fpDATA = fopen( datafile, "r")) == NULL )
      printf("\nDatafile not found.");
   strcpy ( newdatafile, "F:\\TORQUE.DAT\\FINGRAPH.OUT");
   if ((fpNEW = fopen( newdatafile, "w+")) == NULL)
      printf("\nCannot create new datafile");
   fscanf( fpDATA, "%d", &term );
}


void input_data( void )
{
    fscanf( fpDATA, "%d%d%d%lf%lf", &fin, &end, &init, &yint, &reg );
}


void calculate( void)
{
   int i;

   for ( i = 0; i < end; i++ )
     {
      angle[i] = ( init - i );
      torque[i] = ( floor ( yint + ( angle[i] * reg ) + 0.5 ));
     }
}


void save_data( void )
{
   int i, count;

   for ( i = 1; i < end; i++ )
      {
        fprintf( fpNEW, "%2d", i );
        for( count = 1; count < fin; count++ )
           fprintf( fpNEW, " . ");
        fprintf( fpNEW, "%8.0lf", torque[i] );
        for( ; count < 9; count++ )
           fprintf( fpNEW, " . ");
        fprintf( fpNEW, "\n");
      }

}



void all_done( void )
{
   fclose( fpDATA );
   fclose( fpNEW );
   printf("\nALL DONE.!\n\n");
   exit(1);
}

