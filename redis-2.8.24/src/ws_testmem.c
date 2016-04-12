#include "memtest.c"

void main(int argc, char **argv){
	if(argc != 3)
		printf("error ,usage ws_testmem [size] [pass].");
	else{
		int size = atoi(argv[1]);
		int pass = atoi(argv[2]);
		memtest(size, pass);
	}
}