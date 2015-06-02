COMPILER = /usr/bin/mpicc
BINDIR = ./bin/
SRCDIR = ./src/
OUTPUTDIR = ./output

all: sequential parallel1 parallel2 parallel3

sequential:
	mkdir -p $(BINDIR)
	mkdir -p  $(OUTPUTDIR) 
	$(COMPILER) $(SRCDIR)sequential.c -o $(BINDIR)sequential

parallel1:
	$(COMPILER) $(SRCDIR)parallel1.c -o $(BINDIR)parallel1

parallel2:
	$(COMPILER) $(SRCDIR)parallel2.c -o $(BINDIR)parallel2

parallel3:
	$(COMPILER) $(SRCDIR)parallel3.c -o $(BINDIR)parallel3

clean:
	rm $(BINDIR)parallel* $(BINDIR)sequential

