CFLAGS=-g -O2 -Wall

CC = mpicc

PROGRAM_NAME= filter
OBJS = main.o rwppm.o processing.o 

$(PROGRAM_NAME): $(OBJS)
	$(CC) -o $@ $? 

clean:
	rm  *.o $(PROGRAM_NAME) *~ 