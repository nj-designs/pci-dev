all: libcrc-xcel.so crc-xcel-test

libcrc-xcel.so: crc-xcel.o
	$(CC) -shared -o $@ $<

crc-xcel.o: crc-xcel.c
	$(CC) $(CFLAGS) -Wall -Werror -fPIC -c $<

crc-xcel-test: crc-xcel-test.c libcrc-xcel.so
	$(CC) $(CFLAGS) -Wall -Werror -o $@ -L $(@D) -lcrc-xcel $<
