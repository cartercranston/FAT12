char * alloc_OS_name(char * p);

char * alloc_disk_label(char * p);

int get_disk_size(char * p);

int check_file(char * p, int offset);

int files_in_subdirectory(char * p, int starting_offset);

int calculate_num_files(char * p);

int get_num_FAT(char * p);

int get_num_sectors(char * p);