#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "nvme-addon-defs.h"
#include <linux/fs.h>

#include "kv_object.h"

/* Globals */
unsigned long 	lbas;

/**
 * A minimalistic hash that works nicely for now.
 * Replace with any advanced hash that can rule out
 * any collision later. Maybe Davies-Meyer hash.
 *
 * The following hash has been based on the examples in:
 * http://www.cse.yorku.ca/~oz/hash.html
 */ 
static inline unsigned long
hash( char *str)
{
    unsigned long hash = 5381;
    int c;

    while ( (c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

static inline char
*allocBuf( size_t len )
{
	char *p = NULL;

	p = (char *)calloc( len, sizeof(char));
	assert( p != NULL );

	return p;
}	

int
nvme_open_device( void )
{
	int 	_fd 		= -1;
	int	saved_errno 	= 0;
	char	*buf 		= NULL;

	if (( _fd = open( NVME_DEV_PATH, O_RDWR )) == -1 ) {
		saved_errno = errno;

		buf = allocBuf( MAX_BUF_LEN );
		if ( buf )
			strerror_r( saved_errno, buf, MAX_BUF_LEN );
		else
			printf( "Could not allocate buffer of size %d\n",
				MAX_BUF_LEN );
		printf( "Cannot open device file : %s, error : %s",
			NVME_DEV_PATH,
			buf );

	}

	if ( buf ) {
		free( buf );
		buf = NULL;

		return ERR_DEVICE_OPEN;
	}

	return _fd;
}

int
get_phys_sector_size( int fd, int *phys_sector_sz )
{
	int saved_errno = 0;
	
	if ( ioctl( fd, BLKPBSZGET, phys_sector_sz ) < 0 ) {
		saved_errno = errno;

		printf( "Error during ioctl(), cannot get physical \
				sector sz, errno = %d\n",
			saved_errno );

		return ERR_IOCTL_PHYSECSZ;
	}

	return ERR_OK;
}


int
nvme_numlbas( int fd )
{
	int blocksz = 0;
	off_t sz = -1;

	ioctl( fd, BLKSSZGET, &blocksz );
	printf( "logical block size : %d\n", blocksz );

	sz = lseek( fd, 0, SEEK_SET );
	sz = lseek( fd, 0, SEEK_END );

#ifdef _NVME_DEBUG_
	printf(" Method 2 : size of NVMe block dev is : %ld\n", sz );
	printf( "Number of LBAs : %ld\n", (long int)sz/blocksz );
#endif

	lbas = (long int)sz/(blocksz * 2 * 4);
	return 0;
}


/**
 * Assumes that the https/ReST request has been stripped of the http://
 * keyword etc. and that only the object-relevant portion of the path
 * is being passed into this function
 */
static inline long int
obj2lba( char *obj_url )
{
	unsigned long  hashval = 0;
	unsigned long hashlba = 0;

#ifdef _NVME_DEBUG_
	printf( "%s : obj_url = %s\n", __FUNCTION__, obj_url );
#endif
	assert( obj_url != NULL );

	hashval = hash( obj_url );

#ifdef _NVME_DEBUG_
	printf( "%s : hashval is %lu, lbas is %lu\n", __FUNCTION__, hashval, lbas );
#endif

	assert( hashval > 0 );
	hashlba = hashval % lbas;	
	assert( hashlba > 0 );

	return hashlba;
}

void
print_usage( char *objurl )
{
	if ( objurl == NULL )
		return;

	printf( "\nUsage :  %s  <path_to_object>\n", objurl );
	printf( "	 : path_to_object e.g. /v2/account/container/object_id_or_name\n\n" );
}

int 
driver_init( void )
{
	int 		rc 		= 0;
	int 		fd 		= -1;
	int 		phys_sec_sz 	= 0;


	fd = nvme_open_device();
	if ( fd <= 0 ) {
		printf( "Error initializing device(%s), error(%d)",
			NVME_DEV_PATH,
			ERR_DEVICE_OPEN );

		goto err_exit;
	} else {
#ifdef _NVME_DEBUG_
		printf( "Successfully initialized device file %s\n",
			NVME_DEV_PATH );
#endif
	}

	/**
	 *  Device opened successfully
	 *  We will try a bunch of operations on the NVMe block device now
	 */

	/* 1. Get physical sector or blk sz */
	rc = get_phys_sector_size( fd, &phys_sec_sz );
	if ( rc ) {
		printf( "Error getting phys sector size, ret sz %d\n",
			phys_sec_sz );
		goto err_exit;
	} else {
#ifdef _NVME_DEBUG_
		printf( "Physical sector size for device is : %d\n",
			phys_sec_sz );
#endif
	}

	nvme_numlbas( fd );
close_and_exit:
	close( fd );

#ifdef _NVME_DEBUG_
	printf( "Closed device file %s\n", NVME_DEV_PATH );
#endif

	return 0;

err_exit:
	printf( "Error running driver\n" );
	goto close_and_exit;
}

int
nvme_write( int fd, unsigned long lba, char *data )
{
	int 	rc 		= 0;
	size_t 	count 		= 0;
	off_t	off		= 0;
	int	saved_errno 	= 0;

	assert( fd > 0 );

	count = strlen( data );
	off = lba * LBA_SIZE_BYTES;
	rc = pwrite( fd, &count, sizeof( int ), off );
	if ( rc == -1 ) {
		saved_errno = errno;
		printf( "%s : error during write of object MD count, errno = %d\n",
			__FUNCTION__, saved_errno );
	}

	rc = pwrite( fd, data, count, off + sizeof( int ) );
	if ( rc == -1 ) {
		saved_errno = errno;
		printf( "%s : error during write of object data, errno = %d\n",
			__FUNCTION__, saved_errno );
	}

	return rc;
}

int
nvme_read( int fd, unsigned long lba, void **buf  )
{
	int	rc 		= 0;
	off_t 	off 		= 0;
	size_t	count 		= 0;
	int	saved_errno	= 0;

	assert( fd > 0 );
	assert( lba > 0 );

	off = lba * LBA_SIZE_BYTES;
	rc = pread( fd, &count, sizeof( int ), off );
	if ( rc == -1 ) {
		saved_errno = errno;
		printf( "%s : error during pread of MD count, errno = %d\n",
			__FUNCTION__, saved_errno );
	}

	*buf = ( char *)malloc( (sizeof( char )) * (count + 1) );
	memset( *buf, '\0', count + 1 );
	rc = pread( fd, *buf, count, off + sizeof( int ));
	if ( rc == -1 ) {
		saved_errno = errno;
		printf( "%s : error during pread of object data, errno = %d\n",
			__FUNCTION__, saved_errno );
	}

	return count;
}

/**
 * Function/API Name : kv_get()
 *
 * [In]  : char * obj_url  : path to object
 * [Out] : char * data     : object data that will be written
 *         to persistent object location by the API
 * [Out] : Returns an integer count of bytes written for object data
 *         ie. on successful completion it should return the number
 *         of bytes which should be equal to strlen of data
 */  
int
kv_put( char *obj_url, char *data )
{
	int 	fd	= -1;
	int	rc 	= 0;
	unsigned long lba = 0;

#ifdef _NVME_DEBUG_
	printf( "%s : obj_url = %s , data = %s\n", __FUNCTION__,
		obj_url, data );
#endif
	fd = nvme_open_device();
	if ( fd <= 0 ) {
		rc = ERR_DEVICE_OPEN;
		printf( "Error initializing device(%s), error(%d)",
			NVME_DEV_PATH,
			ERR_DEVICE_OPEN );
		goto err_exit;
	}

	lba = obj2lba( obj_url );

	rc = nvme_write( fd, lba, data );
	
exit:
	if ( fd > 0 )
		close( fd );
	return rc;

err_exit:
	printf( "%s : %s : Error during object put operation\n",
		__FILE__, __FUNCTION__ );
	goto exit;
}

/**
 * Function/API Name : kv_get()
 *
 * [In]  : char * obj_url  : path to object
 * [Out] : char **obj_data : *data will be allocated by the API and
 *         the object data will be copied to the buffer pointed to
 *         by *obj_data on successful completion of the API function
 * [Out] : Returns an integer containing number of bytes got for the
 *         object data
 */
int
kv_get( char *obj_url, char **obj_data )
{
	int 	fd 	= -1;
	int	rc 	= 0;
	unsigned long lba = 0;
	char	*data	= NULL;

#ifdef _NVME_DEBUG_
	printf("%s : obj_url : %s\n",__FUNCTION__, obj_url );
#endif
	fd = nvme_open_device();
	if ( fd <= 0 ) {
		rc = ERR_DEVICE_OPEN;
		printf( "Error initializing device(%s), error(%d)",
			NVME_DEV_PATH,
			ERR_DEVICE_OPEN );
		goto err_exit;
	}

	lba = obj2lba( obj_url );

	rc = nvme_read( fd, lba, (void **)&data );
	if ( rc > 0 ) {
		/* non zero bytes read into data, success? */
#ifdef _NVME_DEBUG_
		printf( "%s : read %d bytes corresponding to object (%s), data : %s\n",
			__FUNCTION__, rc, obj_url, data );
#endif
		*obj_data = data;
	} else {
		printf( "Error reading object (%s), bytes read = %d, data = %s\n",
			obj_url, rc, data );
		obj_data = NULL;
	}
		
exit:
	if ( fd > 0 )
		close( fd );
	return rc;

err_exit:
	printf( "%s : %s : Error during object get operation\n",
		__FILE__, __FUNCTION__ );
	goto exit;
}
