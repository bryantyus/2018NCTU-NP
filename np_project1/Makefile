CC = c++
CFLAG = -g -Wall -O2
OBJ = main.o shell.o
INC = shell.h

npshell: $(OBJ) $(INC)
	$(CC) -o npshell $(OBJ) -I $(INC)

%.o: %.cpp
	$(CC) $(CFLAG) -c $^

.PHONY: clean

clean:
	@rm npshell *.o
