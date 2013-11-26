GCC_OPTIONS=-Wall -pedantic -I../../AngelCode_F2013/include
GL_OPTIONS=-framework OpenGL -framework GLUT
COPTIONS=$(GCC_OPTIONS) $(GL_OPTIONS)

all: prog

prog: initShader.o simpleProgram.o
	g++ $(GL_OPTIONS) -g -o prog initShader.o simpleProgram.o

initShader.o: initShader.cpp
	g++ $(GCC_OPTIONS) -g -c initShader.cpp

simpleProgram.o: simpleProgram.cpp
	g++ $(GCC_OPTIONS) -g -c simpleProgram.cpp

clean:
	rm initShader.o
	rm simpleProgram.o
	rm prog
