default:
	gcc -c tlb_fns.c
	gcc -c l1_cache_functions.c
	gcc -c l2cache.c
	gcc -c processes.c
	gcc -c pagetable.c
	gcc -c mainmemory.c
	gcc -o driver.out driver.c l1_cache_functions.o mainmemory.o pagetable.o tlb_fns.o l2cache.o processes.o
clean:
	rm -rf *.out
