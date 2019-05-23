CC = c++
CFLAG = -g -Wall -O2
OBJ = main.o shell.o socket.o
INC = shell.h socket.h

np_single_proc: $(OBJ) $(INC)
	$(CC) -o np_single_proc $(OBJ) -I $(INC)

%.o: %.cpp
	$(CC) $(CFLAG) -c $^

.PHONY: clean

clean:
	@rm np_single_proc *.o *.txt
