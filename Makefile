NVCC := /usr/local/cuda/bin/nvcc --ptxas-options=-v
LIBS := -L/usr/local/cuda/sdk/lib64 -L/usr/local/cuda/lib64
INCS := -I/usr/local/cuda/include -I/usr/include/cuda -I./ -I/usr/local/cuda/sdk/common/inc
CFLAGS := $(INCS) -c# -D_DEBUG
LDFLAGS := $(LIBS) -lcuda
SHA1OBJS := hash.o

all: hash

# SHA-1 benchmark test
hash: $(SHA1OBJS)
	$(NVCC) $(LDFLAGS) $(SHA1OBJS) -o hash
hash.o: hash.cu sha1.h
	$(NVCC) $(CFLAGS) hash.cu -o hash.o


clean:
	rm -rf *~
	rm -rf *.o
	rm hash
