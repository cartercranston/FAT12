#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

int get_byt_per_sec(char * p);

int read_FAT_entry(char * p, int offset);

int calculate_free_space(char * p);

int filename_compare(char * s1, char * s2, int len);

char * alloc_uppercase_string(char * str, int len);

int byt_per_sec;