

#include "privateJsonHeader.h"


int  json_squeezer ( char * dd, char * fn ) {
char             * inFileName;
char             * outFileName;
char             * line = NULL;
char             * jsonSqueezed;
char             * p;

FILE             * fIn;
FILE             * fOut;

int                fd;
unsigned long int  n    =    0,
                   l    =    0;

   inFileName = ( char * ) calloc( 1024, 1 );
   strcpy( inFileName, dd );
   strcat( inFileName, fn );

   outFileName = ( char * ) calloc( 1024, 1 );
   strcpy( outFileName, dd );
   strcat( outFileName, fn );
   strcat( outFileName, ".sqzd" );

   fIn = fopen( inFileName, "r" );
   if ( fIn == NULL ) {
      printf("Unable to open file <%s>!\n",inFileName );
      exit(4);
   }

   fOut = fopen( outFileName, "w" );
   if ( fOut == NULL ) {
      printf("Unable to open file <%s>!\n",outFileName );
      exit(4);
   }

   while ( getline( &line, &n, fIn ) != -1 ) {
      while ( ( p = strstr(line,"  ") ) != NULL ) {
         memmove( p+1, p+2, strlen(p) );
      }  
      p = strchr(line,'\n');
     *p = '\0';
      fputs( line, fOut );    
   }
   fclose( fIn  );
   fclose( fOut );
   free( line );   
   free( inFileName );

   fd = open( outFileName, O_RDONLY );
   l  = lseek(fd, 0L, SEEK_END );    // How big is the file
   jsonSqueezed = (char *)calloc(l+1,1);
   lseek(fd,0L,SEEK_SET);            // Back to the beginning 
   read(fd,jsonSqueezed,l);          // Read the data into our buffer
   close(fd);  

   int i = json_valid( jsonSqueezed );
   free( jsonSqueezed );
   free( outFileName );
   return i;
}
