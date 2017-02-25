#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void
printUsage( char *prog )
{
	printf( "\nUsage : %s <string or path to object>\n\n",
		prog );
}

unsigned long
hash(unsigned char *str)
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

int main( int argc, char **argv )
{
	unsigned long lba = 0;

	if ( !argv[1] ) {
		printUsage( argv[0] );
		exit(1);
	}

	lba = hash( argv[1] );
	printf( "Hashed lba for input string ( %s ) is  :  %lu\n", argv[1], lba );	

	return 0;
}
