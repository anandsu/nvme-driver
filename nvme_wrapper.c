#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <Python.h>

#include "kv_object.h"

static PyObject *kv_get_wrapper( PyObject *self, PyObject *args )
{
	char 		*obj_url	= NULL;
	char		*obj_data	= NULL;	
	PyObject 	*ret		= NULL;
	int		rc		= 0;

	assert( self != NULL );
	assert( args != NULL );

	/* Parse input args */
	if ( !PyArg_ParseTuple( args, "s", &obj_url, &obj_data ) ) {
		return NULL;
	}

	/* run the actual C function requested */
	rc = kv_get( obj_url, &obj_data );
	
}

/*
static PyObject *kv_put_wrapper( PyObject *self, PyObject *args )
{
}
*/

static PyObject *driver_init_wrapper( PyObject *self, PyObject *args )
{
	assert( self != NULL );

	driver_init();

}
