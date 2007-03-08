CFLAGS = -g -I/usr/include/libxml2
CPPFLAGS = -g -I/usr/include/libxml2
LDFLAGS = -g -L/usr/lib -lxml2

OBJECTS = XMLNode.o SOAPMessage.o PayloadRaw.o PayloadSOAP.o \
          PayloadStream.o PayloadTCPSocket.o PayloadHTTP.o

all: test client http_test

test: $(OBJECTS) test.o
	g++ $(LDFLAGS) $(OBJECTS) test.o -o test

client: $(OBJECTS) client.o
	g++ $(LDFLAGS) $(OBJECTS) client.o -o client

http_test: $(OBJECTS) http_test.o
	g++ $(LDFLAGS) $(OBJECTS) http_test.o -o http_test

clean:
	rm -rf *.o test client
