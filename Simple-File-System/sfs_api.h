#define MAXIMUM_FILE_NAME 20
#define MAXFILENAME MAXIMUM_FILE_NAME //doesn't let me run test 2 if I dont declare this var
#include "disk_emu.h"
#include <stdint.h>


#define FREE(data, bit) \
    data = data | (1 << bit)

#define USE(data, bit) \
    data = data & ~(1 << bit)

//TABLE OF CONTENTS
void mksfs(int fresh);
int sfs_get_next_filename(char *fname);
int sfs_GetFileSize(const char* path);
int sfs_fopen(char *name);
int sfs_fclose(int fileID);
int sfs_fread(int fileID, char *buf, int length);
int sfs_fwrite(int fileID, const char *buf, int length);
int sfs_fwseek(int fileID, int loc);
int sfs_frseek(int fileID, int loc);
int sfs_remove(char *file);
//HELPER FUNCTIONS
void set_index_bit(int index);
int get_index_bit();
void remove_index_bit(int index);
