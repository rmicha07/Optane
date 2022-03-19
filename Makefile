all: pmcache

pmcache: pmcache.c
	gcc -std=c99 -Wall -o pmcache pmcache.c -lpmemobj

