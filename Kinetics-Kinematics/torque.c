/* Torque Program.
   ------------------
   Program calculates the torque around the ankle.
   Program prompts for datafile name.
   Currently the datafile is the same name as the "BEDAS"
   output file, except is has an extension (*.dta).
   Program opens both "BEDAS" output file and the "*.dta" file
   (from "ankle.c" ) and combines them to develop a torque file
   ( "*.tor" ).
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <alloc.h>

#define PI 3.14159265

void open_files( void );
void input_data( void );
void convert( void );
void calculate( void );
void save_data( void );
void all_done( void );

int n, end, fin;
double fy, fz, gamma, rel_gamma, zero_angle, torque;



struct jttype
{
   double y, z;
};

typedef struct frametype
{
   struct jttype joint[6];
} FRAMETYPE;

typedef struct frametype FRAME;

FRAME *f;

FILE *fpSESS, *fpNEW, *fpFORCE;

main()
{
   open_files();
   for( n = 0; n < end; n++)
    {
/*      printf("\n%d ", n ); */
      input_data();
      convert();
      calculate();
      save_data();
/*      printf("%d", n );        */
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
       printf( "Error:  Can't allocate memory.\n" );
       exit( 1 );
     }
   else
     {
       for( i = 0; i < 6; i++)
         {
            f->joint[i].y = 0;
            f->joint[i].z = 0;
          }
     }
   return f;
}


void open_files( void )
{
   int i, length;
   char datafile[9], name[9], string[40],
           forcefile[28], sessfile[28], newdatafile[28];

   printf("Enter the name of the datafile (no extension): ");
   scanf( "%8s", &datafile);
   length = strlen( datafile );
   strcpy (sessfile, "F:\\FP_DIG.DAT\\");
   strncat (sessfile, datafile, length + 1 );
   strcat (sessfile, ".DTA");
   if ((fpSESS = fopen( sessfile, "r")) == NULL )
      printf("\nDatafile not found.");
   f = create_frame();
   strcpy ( newdatafile, "F:\\TORQUE.DAT\\");
   strncat ( newdatafile, datafile, length + 1 );
   strcat ( newdatafile, ".TOR");
   if ((fpNEW = fopen( newdatafile, "w+")) == NULL)
      printf("\nCannot create new datafile");
   strcpy ( forcefile, "F:\\FORCE.DAT\\" );
   strcat ( forcefile, datafile );
   if ((fpFORCE = fopen( forcefile, "r" )) == NULL )
      printf( "\nForce datafile not opened." );
   fscanf( fpSESS, "%s\n%d\n%d\n", &name, &end, &fin );
   for ( i = 0; i < 74; i++ )
      fscanf( fpFORCE, "\n%s", string );

}


void input_data( void )
{
   int i;
   double temp, fx, mx, my, mz;

   if ( n == 0 )
   {
    fscanf( fpFORCE, "%lf%lf%lf%lf%lf%lf",
      &fx, &fy, &fz, &mx, &my, &mz );
    for ( i = 0; i < 6; i++ )
      fscanf( fpSESS, "%lf%lf", &f->joint[i].y, &f->joint[i].z);
   }
   else if ( n > 0 )
   {
    for ( i = 0; i < 24; i++ )
       fscanf( fpFORCE, "%lf", &temp );
    fscanf( fpFORCE, "%lf%lf%lf%lf%lf%lf",
               &fx, &fy, &fz, &mx, &my, &mz );
    i = 0;
    fscanf( fpSESS, "\n%lf%lf", &f->joint[i].y, &f->joint[i].z);
    for ( i = 1; i < 6; i++ )
       fscanf( fpSESS, "%lf%lf", &f->joint[i].y, &f->joint[i].z);
   }
/*
     printf( "\nForce - Moment values:\n" );
     printf( "%5.2lf%7.2lf%7.2lf%7.2lf%7.2lf%7.2lf",
        fx, fy, fz, mx, my, mz );
     printf( "\nData Points \(1 - 6\) \n" );
     for ( i = 0; i < 6; i++)
        printf( "%3.0lf %4.0lf   ", f->joint[i].y, f->joint[i].z );
     getchar();
*/
}


void convert( void )
{
   int i;
   double cfactor = 0.00318;

                   /* Converting data from pixels to centimeters */
   for ( i = 0; i < 6; i++ )
     {
        f->joint[i].y = ( f->joint[i].y * cfactor * 100);
        f->joint[i].z = ( f->joint[i].z * cfactor * 100);
     }

           /* Moving origin from lower left to lower right */
   for ( i = 0; i < 6; i++ )
     f->joint[i].y = ( 640 - f->joint[i].y );

}


void calculate( void)
{
   double y1, z1, y2, z2, y3, z3, alpha, beta;


   /* First, finding the angle between ankle, toe, and fin */

        /* Finding the coordinate change from heel to toe */

   y1 = ( f->joint[2].y - f->joint[0].y );
   z1 = ( f->joint[2].z - f->joint[0].z );

   if (( y1 < 0 ) && ( z1 > 0 ))
      {
        printf( "\n\nERROR 1 IN FIN ORIENTATION !! ( Line : %d", n );
        printf( "\n\nPress <Enter>");
        getchar();
        exit( 1 );
      }

       /* Finding the coordinate change from toe to fin */

   y2 = ( f->joint[4].y - f->joint[2].y );
   z2 = ( f->joint[4].z - f->joint[2].z );
   if (( y2 < 0 ) || ( z2 > 0 ))
      {
        printf( "\n\nERROR 2 IN FIN ORIENTATION !! ( Line : %d", n );
        printf( "\n\nPress <Enter>");
        getchar();
        exit( 2 );
      }

                /* Testing for the fin orientation */

   if ( y1 == 0 )
     alpha = 0;

   if ( z1 == 0 )
     alpha = 0;

   if ( y2 == 0 )
     beta = ( PI / 2 );

   if ( z2 == 0 )
      beta = 0;

   if (( y1 != 0 ) && ( z1 != 0 ))
      alpha = atan2( z1, y1 );

   if (( y2 != 0 ) && ( z2 != 0 ))
      beta = atan2( z2, y2 );

   if ((( y1 > 0 ) && ( z1 > 0 )) || (( y1 > 0 ) && ( z1 < 0 )))
      gamma = ( 360 - (( PI + beta - alpha ) * 180 / PI ));
   else if (( y1 < 0 ) && ( z1 < 0 ))
      gamma = (( 180 - ( beta - alpha ) * 180 / PI ));

   if ( n == 0 )
      zero_angle = gamma;
   else if ( n > 0 )
      rel_gamma = ( zero_angle - gamma );




                               /* Finally, finding the torque */

   y3 = ( f->joint[4].y - f->joint[0].y );
   z3 = ( f->joint[4].z - f->joint[0].z );

                               /* Torque in NEWTON METERS */

   torque =  ((( fz * y3 ) - ( fy * z3 )) / 100);

   printf( "\ngamma: %5.3lf  rel_gamma: %5.3lf  torque: %5.3lf",
                             gamma, rel_gamma, torque );
/*   if (( n % 15 ) == 0 )
       getchar();
*/
}


void save_data( void )
{
   int i;

   fprintf( fpNEW, "%d%10.3lf%10.3lf%10.3lf%10.3lf\n", 
                             fin, fy, fz, gamma, rel_gamma );       
}       

void all_done( void )
{
   fclose( fpNEW );
   fclose( fpSESS );
   fclose( fpFORCE );
   printf("\n\nAll Done !\n");
}

