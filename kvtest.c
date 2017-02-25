#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <string.h>

#include "kv_object.h"

typedef enum objOp {
	KV_OBJ_GET,
	KV_OBJ_PUT
} objOp;

void print_usage( char *progname )
{
	if ( progname == NULL )
		return;

	printf( "Usage:	%s <PUT|put|Put|GET|Get|get> <full_path_to_object|object_url> <data>\n",
		progname );
}

char *
allocBuf( void )
{
	char *p = NULL;
	int saved_errno;

	p = ( char * )malloc( (sizeof( char ) * MAX_OBJECT_DATA_SZ ));
	if ( p == NULL ) {
		saved_errno = errno;
		printf( "%s : %s : Error ( %d )during memory alloc, cannot allocate buffer\n",
			__FUNCTION__,
			__FILE__,
			saved_errno );
	}

	return p;
}

int main( int argc, char **argv )
{
	int 	rc = 0;
	objOp	op;
	char 	*data = NULL;
	char	*obj_url = NULL;

	if ( argc < 3 ) {
		print_usage( argv[0] );
		exit(1);
	}

	driver_init();

	if ((strcmp( argv[1], "GET") == 0 ) ||
	    (strcmp( argv[1], "get") == 0 ) ||
	    (strcmp( argv[1], "Get") == 0 )) {
		obj_url = argv[2];
#ifdef _KVTEST_DEBUG_
		printf( "%s : obj_url = %s\n", __FUNCTION__, obj_url );
#endif
		assert( obj_url != NULL );

		op = KV_OBJ_GET;

		data = allocBuf();

		rc = kv_get( obj_url, &data );

		printf( "kv_get : Got data for object %s : %s\n",
			obj_url, data );
	} else if ((strcmp( argv[1], "PUT") == 0 ) || 
		   (strcmp( argv[1], "put") == 0 ) ||
		   (strcmp( argv[1], "Put") == 0 )) {
		op = KV_OBJ_PUT;

		obj_url = argv[2];
		assert( obj_url != NULL );

#ifdef _KVTEST_DEBUG_
		printf( "%s : Put op : argv[3] = %s\n", argv[0], argv[3] );
#endif
		data = strdup( argv[3] );
		assert( data != NULL );
		rc = kv_put( obj_url, data );

	} else {
		print_usage( argv[0] );
	}

cleanup:
	if ( data ) {
		free( data );
		data = NULL;
	}

	exit( 0 );
}
