# Compile and link program to play multiple files

SDK=$(xcrun --show-sdk-path)

g++ -Wall -std=c++17 main.cpp paUtils.cpp -g -o transportr \
	-I$SDK/usr/include/c++/v1 \
	-I/usr/local/include -L/usr/local/lib \
	-lsndfile -lncurses -lportaudio
