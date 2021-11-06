all: gcc clang

clang:
	 clang++ -std=c++20 -o gor_clang -Wall -Werror -stdlib=libc++ -O2 -g gor.cc

gcc:
	g++ -std=c++20 -o gor_gcc -Wall -Werror -O2 -g gor.cc

clean:
	rm gor_gcc 
	rm gor_clang 

