default:
	gcc -c l2cache.c
	gcc -o test l2cache_main.c l2cache.o