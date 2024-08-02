#include "disk.h"
#include "diskput.h"
#include "disk.c"

int power_of_two (int n) {
    // check that n has only one bit set
    return n > 0 && !(n & (n - 1));
}

int add_to_data_area(char * p, char * sector_contents) {
    int logical_cluster = 2;
    // find empty sector
    while (logical_cluster < 2880) {
        if(read_FAT_entry(p, logical_cluster) == 0) {
            break;
        } else {
            logical_cluster ++;
        }
    }

    // copy sector contents to it
    memcpy(p + (31 + logical_cluster) * byt_per_sec, sector_contents, byt_per_sec);
    return logical_cluster;
}

void add_first_cluster_to_dir(char * p, int logical_cluster, char ** path, int path_len, long file_size_in_bytes) {
    int free_location;
    // find empty space in directory
    if (path_len == 1) {
        for (int offset = 19*byt_per_sec; offset < 33*byt_per_sec; offset += 32) {
            if (*(int *) (p + offset) == 0) {
                free_location = offset;
                break;
            }
        }
    } else {
        // loop through files in root directory and look for subdirectory
        for (int offset = 19*byt_per_sec; offset < 33*byt_per_sec; offset += 32) {
            char * subdir_name = alloc_uppercase_string(path[0], strlen(path[0]));
            if (filename_compare(subdir_name, p + offset, 8) == 0) {
                int next_location = *(unsigned short *) (p + offset + 26);
                free_location = find_free_location_in_subdir(p, path+1, path_len-1, next_location);
                break;
            }
            free(subdir_name);
        }
    }

    // add directory entry at free location
    // calculate date and time
    struct stat attr;
    if (stat(path[path_len-1], &attr) != 0) {
        perror("stat error");
    }
    struct tm * mod_time = localtime(&(attr.st_mtime));
    unsigned short formatted_mod_time = (mod_time->tm_sec & 0x1f) + ((mod_time->tm_min & 0x3f) << 5) + ((mod_time->tm_hour & 0x1f) << 11);
    unsigned short formatted_mod_date = (mod_time->tm_mday & 0x1f) + (((mod_time->tm_mon + 1) & 0xf) << 5) + (((mod_time->tm_year + 1900 - 1980) & 0x7f) << 9);
    struct tm * access_time = localtime(&(attr.st_atime));
    unsigned short formatted_access_date = (access_time->tm_mday & 0x1f) + (((access_time->tm_mon + 1) & 0xf) << 5) + (((access_time->tm_year + 1900 - 1980) & 0x7f) << 9);
    // convert filename to uppercase
    char * name = alloc_uppercase_string(strtok(path[path_len-1], "."), 8);
    char * extension = alloc_uppercase_string(strtok(NULL, "."), 3);
    // combine values into char array
    char entry[32];
    memcpy(entry, name, 8);
    memcpy(entry + 8, extension, 3);
    entry[11] = 0;
    entry[14] = (formatted_mod_time & 0xff);
    entry[15] = (formatted_mod_time & 0xff00) >> 8;
    entry[16] = (formatted_mod_date & 0xff);
    entry[17] = (formatted_mod_date & 0xff00) >> 8;
    entry[18] = (formatted_access_date & 0xff);
    entry[19] = (formatted_access_date & 0xff00) >> 8;
    entry[22] = (formatted_mod_time & 0xff);
    entry[23] = (formatted_mod_time & 0xff00) >> 8;
    entry[24] = (formatted_mod_date & 0xff);
    entry[25] = (formatted_mod_date & 0xff00) >> 8;
    entry[26] = (logical_cluster & 0xff);
    entry[27] = (logical_cluster & 0xff00) >> 8;
    entry[28] = (file_size_in_bytes & 0xff);
    entry[29] = (file_size_in_bytes & 0xff00) >> 8;
    entry[30] = (file_size_in_bytes & 0xff0000) >> 16;
    entry[31] = (file_size_in_bytes & 0xff000000) >> 24;
    // add to memory
    memcpy(p + free_location, entry, 32);
    free(name);
    free(extension);
}

int find_free_location_in_subdir (char * p, char ** path, int path_len, int subdir_location) {
    if (path_len == 1) {
        // find empty space in subdirectory
        for (int current_sector = subdir_location; ; current_sector = read_FAT_entry(p, current_sector)) {
            if (current_sector >= 0xff8) {
                break;
            } else if (current_sector == 0 || current_sector >= 0xff0) {
                printf("Warning: find_free_location_in_subdir accessed FAT entry of %d.\n", current_sector);
                break;
            }
            for (int offset = (current_sector + 31) * byt_per_sec; offset < (current_sector + 32) * byt_per_sec; offset += 32) {
                if (*(int *) (p + offset) == 0) {
                    return offset;
                }
            }
        }
        printf("Error: current sector of subdirectory is full.\n");
        return -1;
    } else {
        // find subdirectory
        for (int current_sector = subdir_location; ; current_sector = read_FAT_entry(p, current_sector)) {
            if (current_sector >= 0xff8) {
                break;
            } else if (current_sector == 0 || current_sector >= 0xff0) {
                printf("Warning: find_free_location_in_subdir accessed FAT entry of %d.\n", current_sector);
                break;
            }

            // in each sector, loop through files and look for subdirectory
            for (int offset = (current_sector + 31) * byt_per_sec; offset < (current_sector + 32) * byt_per_sec; offset += 32) {
                char * subdir_name = alloc_uppercase_string(path[0], strlen(path[0]));
                if (filename_compare(subdir_name, p + offset, 8) == 0) {
                    int next_location = *(unsigned short *) (p + offset + 26);
                    return find_free_location_in_subdir(p, path+1, path_len-1, next_location);
                }
                free(subdir_name);
            }
        }
        printf("Usage: Couldn't find subdirectory %s.\n", path[0]);
        return -1;
    }
}

int find_subsubdir (char * p, char ** path, int path_len, int subdir_location) {
    if (path_len == 1) {
        // found it
        return 1;
    } else {
        // look deeper
        for (int current_sector = subdir_location; ; current_sector = read_FAT_entry(p, current_sector)) {
            if (current_sector >= 0xff8) {
                break;
            } else if (current_sector == 0 || current_sector >= 0xff0) {
                printf("Warning: find_subsubdir accessed FAT entry of %d.\n", current_sector);
                break;
            }

            // in each sector, loop through files and look for subdirectory
            for (int offset = (current_sector + 31) * byt_per_sec; offset < (current_sector + 32) * byt_per_sec; offset += 32) {
                char * subdir_name = alloc_uppercase_string(path[0], strlen(path[0]));
                if (filename_compare(subdir_name, p + offset, 8) == 0) {
                    int next_location = *(unsigned short *) (p + offset + 26);
                    return find_subsubdir(p, path+1, path_len-1, next_location);
                }
                free(subdir_name);
            }
        }
        return 0;
    }
}

void add_entry_to_FAT(char * p, int next_sector, int sector) {
    char * first_byte = p + byt_per_sec + (3 * sector) / 2;
    char * second_byte = first_byte + 1;
    if (sector % 2 == 0) {
        // for even entries, concatenate low nibble of second byte with first byte
        int second_byte_val = (*(unsigned short*) second_byte) & 0xf0;
        second_byte_val |= (next_sector & 0xf00) >> 8;
        *second_byte = second_byte_val;
        *first_byte = next_sector & 0xff;
        *(second_byte + 9*byt_per_sec) = second_byte_val;
        *(first_byte + 9*byt_per_sec) = next_sector & 0xff;
    } else {
        // for odd entries, concatenate second byte with high nibble of first byte
        int first_byte_val = (*(unsigned short*) first_byte) & 0x0f;
        first_byte_val |= ((next_sector & 0xf) << 4);
        *second_byte = ((next_sector & 0xff0) >> 4);
        *first_byte = first_byte_val;
        *(second_byte + 9*byt_per_sec) = ((next_sector & 0xff0) >> 4);
        *(first_byte + 9*byt_per_sec) = first_byte_val;
    }
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

    // open input file
    char ** path = malloc(2 * sizeof(char *));
    char * token = strtok(argv[2], "/");
    int path_len = 0;
    while (token != NULL) {
        // extend path if necessary
        if (power_of_two(path_len)) {
            char ** temp = malloc(2 * path_len * sizeof(char *));
            for (int i = 0; i < path_len; i ++) {
                temp[i] = path[i];
            }
            free(path);
            path = temp;
        }
        // add token to path
        path[path_len] = malloc(strlen(token) + 1);
        strncpy(path[path_len], token, strlen(token));
        token = strtok(NULL, "/");
        path_len ++;
    }

    if (path_len != 1) {
        // check if subdirectory exists
        // loop through files in root directory and look for subdirectory
        int found_subdir = 0;
        for (int offset = 19*byt_per_sec; offset < 33*byt_per_sec; offset += 32) {
            char * subdir_name = alloc_uppercase_string(path[0], strlen(path[0]));
            if (filename_compare(subdir_name, p + offset, 8) == 0) {
                int next_location = *(unsigned short *) (p + offset + 26);
                found_subdir = find_subsubdir(p, path+1, path_len-1, next_location);
                free(subdir_name);
                break;
            }
            free(subdir_name);
        }
        if (found_subdir == 0) {
            printf("Usage: Couldn't find directory.\n");
            exit(1);
        } 
    }

    // open file
    FILE * f = fopen(path[path_len-1], "rb");
    if (f == NULL) {
        printf("Usage: File %s not found.\n", path[path_len-1]);
        exit(1);
    }
    char * buffer = malloc(byt_per_sec);

    // check that data area has room for file
    fseek(f, 0L, SEEK_END);
    long file_size_in_bytes = ftell(f);
    rewind(f);
    if (file_size_in_bytes > calculate_free_space(p) * byt_per_sec) {
        printf("Error: Not enough free space in the disk image.\n");
        exit(1);
    }

    // loop through sectors
    int is_first_cluster = 1;
    int prev_logical_cluster = -1;
    while (!feof(f)) {
        fread(buffer, byt_per_sec, 1, f);
        
        // write file contents to data area
        int logical_cluster = add_to_data_area(p, buffer);

        // add location in data area to FAT or directory
        if (is_first_cluster) {
            is_first_cluster = 0;
            add_first_cluster_to_dir(p, logical_cluster, path, path_len, file_size_in_bytes);
        } else {
            add_entry_to_FAT(p, logical_cluster, prev_logical_cluster);
        }
        add_entry_to_FAT(p, 0xff0, logical_cluster);
        prev_logical_cluster = logical_cluster;
    }
    add_entry_to_FAT(p, 0xfff, prev_logical_cluster);

    for (int i = 0; i < path_len; i ++) {
        free(path[i]);
    }
    free(path);
    free(buffer);
    fclose(f);
    munmap(p, sb.st_size); // map p back to the disk
}