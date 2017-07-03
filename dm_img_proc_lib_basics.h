#ifndef DM_IMG_PROC_LIB_BASICS_H
#define DM_IMG_PROC_LIB_BASICS_H
/*included files*/
#ifdef _WIN32
    #include "windows.h"
    #include "winbase.h"
#elif __linux__
    #include <sys/statvfs.h>
#else
    #error "Unknow OS"
#endif // _WIN32
#include <stdio.h>
#include <string>
#include <sstream>
/*enum type*/
enum DM_IMG_PROC_RETURN_CODE {
    DM_IMG_PROC_RETURN_OK = 0,
    DM_IMG_PROC_RETURN_FAIL = 1,
    DM_IMG_PROC_RETURN_EMPTY_ITER = 2,
    DM_IMG_PROC_RETURN_END_OF_ITER = 3,
    DM_IMG_PROC_RETURN_DISK_SPACE_NOT_ENOUGH = 4
};
/*const values*/
extern const std::string DM_IMG_SCAN_TILE_NAME_PREFIX;
extern const std::string DM_IMG_SCAN_TILE_NAME_POSFIX;
/*declared functions*/
int getDirFreeSpace(const char *file_dir, float *free_bytes_ptr);
bool checkIfFileExists(std::string file_path);
template <typename T>
std::string NumberToString( T Number );
std::string getTilePath(const char* tile_dir, int mat_id, int x, int y);
std::string GetScanInfoFileName(int mat_id);
std::string GetAlignInfoFileName(int mat_id);
#endif // DM_IMG_PROC_LIB_BASICS_H
