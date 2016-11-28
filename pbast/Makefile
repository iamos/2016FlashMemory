############## SunOS ########################
CC       = gcc -g
CCOPTS   = -c #-Wall
LINK     = gcc -g

DEPEND  = makedepend

LINKOPTS = -lm -lnsl
#############################################

SOURCES = ftlayer.c device1.c main.c

OBJECTS = ftlayer.o device1.o main.o

.SUFFIXES : .C .c .o

.c.o:
	$(CC) $(CCOPTS) $<

all: msbs

msbs: $(OBJECTS)
	$(LINK) -o msbs $(OBJECTS) $(LINKOPTS)

clean:
	rm *.o msbs database

depend : $(SOURCES)
	$(DEPEND) $(DEFINES) -I/usr/local/lib/g++-include $(SOURCES)

# DO NOT DELETE
