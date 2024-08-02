#include "disk.h"
#include "diskget.h"
#include "disk.c"

int get_file_location(char * p, char * file_name) {
    char * name = strtok(file_name, ".");
    char * extension = strtok(NULL, ".");

    // search root directory for file
    int offset = 19*byt_per_sec;
    for (; offset < 33*byt_per_sec; offset += 32) {
        if (filename_compare(name, p + offset, 8) == 0 && filename_compare(extension, p + offset + 8, 3) == 0) {
            return offset;
        }
    }
    printf("Usage: File wasn't in root directory.\n");
    return -1;
}

int main (int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: must give name of image and file to copy.\n");
        return 1;
    }
    int fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        printf("Error: couldn't open %s.\n", argv[1]);
        return 1;
    }
	struct stat sb; 
	fstat(fd, &sb);

	char * p = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); // p points to the starting pos of your mapped memory
	if (p == MAP_FAILED) {
		printf("Error: failed to map memory\n");
		exit(1);
	}

    // ensure that code to read two bytes will work
    assert(sizeof(unsigned short) == 2); 
    assert(sizeof(int) == 4);

    // initialize global variables
    byt_per_sec = get_byt_per_sec(p);

    // open file
    FILE * output = fopen(argv[2], "wb");
    char buffer[512];
    char * file_name = alloc_uppercase_string(argv[2], strlen(argv[2]));

    // loop through sectors
    int file_location = get_file_location(p, file_name);
    if (file_location == -1) {
        exit(1);
    }
    int first_cluster = *(unsigned short *) (p + file_location + 26);
    int file_size_in_bytes = *(int *) (p + file_location + 28);
    int bytes_written = 0;
    for (int current_cluster = first_cluster;; current_cluster = read_FAT_entry(p, current_cluster)) {
        if (current_cluster >= 0xff8) {
            printf("Finished reading.\n");
            break;
        } else if (current_cluster == -1) {
            break;
        } else if (current_cluster == 0 || current_cluster >= 0xff0) {
            printf("Warning: accessed FAT entry of %d.\n", current_cluster);
            break;
        }
        // copy contents of each sector to output file
        memcpy(buffer, p + (current_cluster + 31) * byt_per_sec, byt_per_sec);
        if (bytes_written + byt_per_sec <= file_size_in_bytes) {
            fwrite(buffer, byt_per_sec, 1, output);
            bytes_written += byt_per_sec;
        } else if (bytes_written < file_size_in_bytes) {
            fwrite(buffer, file_size_in_bytes - bytes_written, 1, output);
            bytes_written = file_size_in_bytes;
        } else {
            printf("Warning: File longer than expected.\n");
            break;
        }
    }
    free(file_name);
    fclose(output);
}