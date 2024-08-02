#include "disk.h"
#include "stack.h"
#include "disklist.h"
#include "disk.c"
#include "stack.c"

// Adapted from https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
// by Adam Rosenfield
void trimwhitespace(char *str)
{
  char *end;

  if(*str == 0)  // All spaces?
    return;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';
}

void print_file_info(char * p, int offset) {
    // check if printing the file makes sense
    int attributes = ((*(unsigned short*) (p + offset + 11)) & 0xff);
    if (attributes == 0x0f || attributes == 0x08) {
        // long names and volume labels can be ignored
        return;
    }
    if (*(int *) (p + offset) == 0 || *(unsigned short *) (p + offset) == 0x202e || *(unsigned short *) (p + offset) == 0x2e2e) {
        // empty files, self (.), and parent (..), can be ignored
        return;
    }

    // get and print the file info
    char * file_name;
    char * extension;
    char file_type;
    int creation_date;
    int year;
    int month;
    int day;
    int creation_time;
    int hours;
    int minutes;
    int file_size;

    // determine the file's name and extension
    file_name = malloc(8 * sizeof(char));
    strncpy(file_name, p + offset, 8);
    extension = malloc(3 * sizeof(char));
    strncpy(extension, p + offset + 8, 3);

    // determine the file type
    if ((attributes & 0x10) == 0) {
        // file is not a directory
        file_type = 'F';
    } else {
        // file is a directory
        add_dir_to_stack(offset);
        file_type = 'D';
    }

    // determine the creation_date and creation_time the file was created
    creation_time = *(unsigned short *) (p + offset + 14);
    creation_date = *(unsigned short *) (p + offset + 16);
    //the year is stored as a value since 1980
    //the year is stored in the high seven bits
    year = ((creation_date & 0xFE00) >> 9) + 1980;
    //the month is stored in the middle four bits
    month = (creation_date & 0x1E0) >> 5;
    //the day is stored in the low five bits
    day = (creation_date & 0x1F);
    //the hours are stored in the high five bits
    hours = (creation_time & 0xF800) >> 11;
    //the minutes are stored in the middle 6 bits
    minutes = (creation_time & 0x7E0) >> 5;

    // determine the size of the file
    file_size = *(int *) (p + offset + 28);

    // print all information
    trimwhitespace(file_name);
    trimwhitespace(extension);
    printf("%c %10d %17s.%s %4d-%2d-%2d, %2d:%2d\n", file_type, file_size, file_name, extension, year, month, day, hours, minutes);
    free(extension);
    free(file_name);
}

void print_directory(char * p, char * dir_name, int starting_sector) {
    printf("\n%s\n==============\n", dir_name);
    // loop through sectors
    for (int current_sector = starting_sector; ; current_sector = read_FAT_entry(p, current_sector)) {
        if (current_sector >= 0xff8) {
            break;
        } else if (current_sector == 0 || current_sector >= 0xff0) {
            printf("Warning: print_directory accessed FAT entry of %d.\n", current_sector);
            break;
        }

        // in each sector, loop through files and call print_file_info
        for (int offset = (current_sector + 31) * byt_per_sec; offset < (current_sector + 32) * byt_per_sec; offset += 32) {
            print_file_info(p, offset);
        }
    }
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
    assert(sizeof(int) == 4);

    // initialize global variables
    byt_per_sec = get_byt_per_sec(p);
    dir_list = malloc(sizeof(stack_node));
    dir_list->val = -1;
    dir_list->next = NULL;

    // print root directory
    printf("Root Directory\n==============\n");
    for (int offset = 19*byt_per_sec; offset < 33*byt_per_sec; offset += 32) {
        print_file_info(p, offset);
    }

    // print all subdirectories
    for (int root_offset = get_dir_from_stack(); root_offset != -1; root_offset = get_dir_from_stack()) {
        char * dir_name = malloc(8 * sizeof(char));
        strncpy(dir_name, p + root_offset, 8);
        int sub_sector = *(unsigned short *) (p + root_offset + 26);
        print_directory(p, dir_name, sub_sector);
        free(dir_name);
    }
}