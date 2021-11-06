all: gcc clang

clang:
	 clang++ -std=c++20 -o readdir_clang -Wall -Werror -stdlib=libc++ -O2 -g main.cc

gcc:
	g++ -std=c++20 -o readdir_gcc -Wall -Werror -O2 -g main.cc

clean:
	rm readdir_clang 
	rm readdir_gcc

