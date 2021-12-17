# Compile and link program to play multiple files

g++ -Wall main.cpp paUtils.cpp -o transportr \
	-I/usr/local/include -L/usr/local/lib \
	-lsndfile -lncurses -lportaudio
