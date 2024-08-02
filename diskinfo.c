#include "diskinfo.h"
#include "disk.h"
#include "stack.h"
#include "disk.c"
#include "stack.c"

char * alloc_OS_name(char * p) {
    char * name = malloc(8 * sizeof(char));
    strncpy(name, p + 3, 8);
    return name;
}

char * alloc_disk_label(char * p) {
    char * label;
    for (int offset = 19*byt_per_sec; offset < 33*byt_per_sec; offset += 32) {
        int attributes = ((*(unsigned short*) (p + offset + 11)) & 0xff);
        if (attributes == 0x08) {
            label = malloc(11 * sizeof(char));
            strncpy(label, p + offset, 11);
            return label;
        }
    }
    printf("Warning: Label not found.\n");
    label = malloc(1);
    label[0] = 0;
    return label;
}

int get_disk_size(char * p) {
    // returns the number of bytes that the FAT can map to (not the available space)
    int total_sectors = *(unsigned short*) (p + 19);

    return total_sectors;
}

/*int calculate_num_files(char * p) {
    int num_files = 0;
    for (int offset = 2; offset < 2880; offset ++) {
        int value = read_FAT_entry(p, offset);
        
        if (value >= 0xff8) {
            num_files ++;
        }
    }
    return num_files;
}*/

int check_file(char * p, int offset) {
    // check file properties
    int attributes = ((*(unsigned short*) (p + offset + 11)) & 0xff);
    if (attributes == 0x0f || attributes == 0x08) {
        // long names and volume labels aren't counted
        return 0;
    } else if (*(p + offset) == 0xe5 || *(p + offset) == 0) {
        // free files aren't counted
        return 0;
    } else if (*(unsigned short *) (p + offset + 26) <= 1) {
        // empty files aren't counted
        return 0;
    } else if ((attributes & 0x10) == 0) {
        // file is not a directory
        return 1;
    } else {
        // file is a directory
        add_dir_to_stack(offset);
        return 0;
    }
}

int files_in_subdirectory(char * p, int starting_offset) {
    int num_files = 0;
    // loop through sectors
    for (int current_sector = starting_offset; ; current_sector = read_FAT_entry(p, current_sector)) {
        if (current_sector >= 0xff8) {
            break;
        } else if (current_sector == 0 || current_sector >= 0xff0) {
            printf("Warning: files_in_subdirectory accessed FAT entry of %d.\n", current_sector);
            break;
        }

        // in each sector, loop through files
        for (int offset = current_sector; offset < current_sector + 16; offset += 32) {
            // check if there is a file or subdirectory at this offset
            num_files += check_file(p, offset);
        }
    }
    return num_files;
}

int calculate_num_files(char * p) {
    int num_files = 0;
    // count files in root directory
    for (int offset = 19*byt_per_sec; offset < 33*byt_per_sec; offset += 32) {
        // check if there is a file or subdirectory at this offset
        num_files += check_file(p, offset);
    }

    // count files in subdirectories
    for (int root_offset = get_dir_from_stack(); root_offset != -1; root_offset = get_dir_from_stack()) {
        int sub_offset = *(unsigned short *) (p + root_offset + 26);
        num_files += files_in_subdirectory(p, sub_offset);
    }
    return num_files;
}

int get_num_FAT(char * p) {
    return (*(unsigned short*) (p + 16)) & 0xff;
}

int get_num_sectors(char * p) {
    return *(unsigned short*) (p + 22);
}

int main (int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: must give name of image.\n");
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

    // initialize variables
    byt_per_sec = get_byt_per_sec(p);
    dir_list = malloc(sizeof(stack_node));
    dir_list->val = -1;
    dir_list->next = NULL;
    int disk_size = get_disk_size(p);
    int free_space = calculate_free_space(p);
    char * name = alloc_OS_name(p);
    char * label = alloc_disk_label(p);

    printf("OS Name: %s\n\
Label of the disk: %s\n\
Total size of the disk: %d sectors, i.e. %d bytes\n\
Free size of the disk: %d sectors, i.e. %d bytes\n\
---------------------------------------------\n\
There are %d files in the disk\n\
-------------------------------\n\
There are %d copies of FAT\n\
Sectors per FAT: %d\n", name, label, disk_size, disk_size * byt_per_sec, free_space, free_space * byt_per_sec, calculate_num_files(p), get_num_FAT(p), get_num_sectors(p));
}