#include <time.h>

int byt_per_sec;

int power_of_two (int n);

int add_to_data_area(char * p, char * sector_contents);

void add_first_cluster_to_dir(char * p, int logical_cluster, char ** path, int path_len, long file_size_in_bytes);

int find_free_location_in_subdir (char * p, char ** path, int path_len, int subdir_location);

void add_entry_to_FAT(char * p, int sector, int prev_sector);

int find_subsubdir (char * p, char ** path, int path_len, int subdir_location);