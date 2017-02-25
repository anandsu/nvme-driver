#ifndef __KV_DRIVER_H__
#define __KV_DRIVER_H__

// Local defines
#define NVME_DEV_PATH   "/dev/nvme1n1"
#define MAX_BUF_LEN     1024
#define LBA_SIZE_BYTES  (4 * 1024)

#define MAX_OBJECT_DATA_SZ	4092

// Error defines
#define ERR_OK                  0
#define ERR_DEVICE_OPEN         -1
#define ERR_MEM_ALLOC           -2
#define ERR_IOCTL_PHYSECSZ      -3



/**
 * KV Interface APIs
 *
 * NOTE : These will likely be merged into a common API
 * with an option to pass in the Op type as Put/Get
 * For later.
 */
int
driver_init( void );

int
kv_put( char *obj_url, char *obj_data );

int
kv_get( char *obj_url, char **obj_data );


#endif
