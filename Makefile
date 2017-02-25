CC=gcc
CFLAGS= -Wall -Werror -O2 -fstrict-aliasing
INCLUDE= -I/usr/include -I./ -I./linux -I/usr/include/python2.7/

LDFLAGS= -shared -fPIC
DEBUG_FLAGS= -D_NVME_DEBUG_ -D_KVTEST_DEBUG_ -g

all: exe

debug: CFLAGS += -D_NVME_DEBUG_ -D_KVTEST_DEBUG_ -g
debug: exe

exe:
	$(CC) $(CFLAGS) -fPIC -c nvme_obj.c
	$(CC) $(LDFLAGS) -o libnvme_obj.so nvme_obj.o
	$(CC) -L/root/src/nvme-driver $(INCLUDE) -o objtest kvtest.c -lnvme_obj
	$(CC) $(CFLAGS) $(LDFLAGS) $(INCLUDE) -L./ -o libnvme_wrapper.so nvme_wrapper.c -lnvme_obj -lpython2.7

clean:
	rm -rf  nv *.o a.out *.so *.a objtest libnvme_obj.so
