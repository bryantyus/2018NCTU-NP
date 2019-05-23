CC = c++
CFLAG = -g -Wall -O2
OBJ = main.o shell.o socket.o
INC = shell.h socket.h

np_simple: $(OBJ) $(INC)
	$(CC) -o np_simple $(OBJ) -I $(INC)

%.o: %.cpp
	$(CC) $(CFLAG) -c $^

.PHONY: clean

clean:
	@rm np_simple *.o *.txt
