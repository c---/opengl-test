main: main.cpp
	g++ -Wall -O2 -g $< -o $@ -lGL -lSDL2
