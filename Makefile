CC = g++
PROG = 3dm_2_g2

GOTLIB	= -lGoToolsCore -L/usr/local/lib/GoTools -lGoTrivariate
ONOPT	= -I../opennurbs
ONLIB	= -L../opennurbs -lopenNURBS

CFLAGS = -Wall $(GOTOPT) $(ONOPT) -g

SRCS = 3dm_2_g2.cpp

LIBS = $(GOTLIB) $(ONLIB)

all: $(PROG)

$(PROG):	$(SRCS)
	$(CC) $(CFLAGS) -o $(PROG) $(SRCS) $(LIBS)

clean:
	rm -f $(PROG)
