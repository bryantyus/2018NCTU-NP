CC = c++
CFLAG = -g -Wall -O0
OBJ = main.o shell.o socket.o
INC = shell.h socket.h

np_multi_proc: $(OBJ) $(INC)
	$(CC) -o np_multi_proc $(OBJ) -I $(INC) -lrt

%.o: %.cpp
	$(CC) $(CFLAG) -c $^

.PHONY: clean

clean:
	@rm np_multi_proc *.o *.txt
