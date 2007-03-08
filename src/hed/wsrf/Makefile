CFLAGS = -g -I/usr/local/include/libxml2
CPPFLAGS = -g -I/usr/local/include/libxml2
LDFLAGS = -g -L/usr/local/lib -lxml2

OBJECTS = WSResourceProperties.o ../SOAPMessage.o ../XMLNode.o

all: test 

test: $(OBJECTS) test.o
	g++ $(LDFLAGS) $(OBJECTS) test.o -o test

clean:
	rm -rf *.o test