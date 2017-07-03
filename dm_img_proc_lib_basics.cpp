/*included files*/
#include "dm_img_proc_lib_basics.h"
/*define variable*/
const std::string DM_IMG_SCAN_TILE_NAME_PREFIX = "/tile_";
const std::string DM_IMG_SCAN_TILE_NAME_POSFIX = ".bmp";
/*defined functions*/
#ifdef _WIN32
int getDirFreeSpace(const char *file_dir, float *free_bytes_ptr){
    /*check input*/
    if( (NULL==file_dir)||(NULL==free_bytes_ptr) ){
        return DM_IMG_PROC_RETURN_FAIL;
    }
    /*local var*/
    BOOL isOK;
    ULARGE_INTEGER freeBytes;
    /*init*/
    /*get number of bytes of free space in directory*/
    isOK = GetDiskFreeSpaceEx((LPCSTR)file_dir, &freeBytes, NULL, NULL);
    if(TRUE!=isOK){
        return DM_IMG_PROC_RETURN_FAIL;
    }
    *free_bytes_ptr = (float)(freeBytes.QuadPart);
    /*return*/
    return DM_IMG_PROC_RETURN_OK;
}
#elif __linux__
int getDirFreeSpace(const char *file_dir, float *free_bytes_ptr){
    /*check input*/
    if( (NULL==file_dir)||(NULL==free_bytes_ptr) ){
        return DM_IMG_PROC_RETURN_FAIL;
    }
    /*local var*/
    int r;
    struct statvfs stat;
    /*init*/
    /*get number of bytes of free space in directory*/
    r = statvfs(file_dir, &stat);
    if(0!=r){
        return DM_IMG_PROC_RETURN_FAIL;
    }
    *free_bytes_ptr = (float)(stat.f_bsize) * (float)(stat.f_bavail);
    /*return*/
    return DM_IMG_PROC_RETURN_OK;
}
#else
    #error "Unknow OS"
#endif // _WIN32
bool checkIfFileExists(std::string file_path){
    FILE *file = fopen(file_path.c_str(), "r");
    if(NULL==file){
        return false;
    }
    fclose(file);
    return true;
}
template <typename T>
std::string NumberToString( T Number ){
    std::ostringstream ss;
    ss << Number;
    return ss.str();
}
std::string getTilePath(const char* tile_dir, int mat_id, int x, int y){
    std::string out_str = tile_dir + DM_IMG_SCAN_TILE_NAME_PREFIX + \
                            NumberToString(mat_id) + "_" + NumberToString(x) + "_" + NumberToString(y) + \
                            DM_IMG_SCAN_TILE_NAME_POSFIX;
    return out_str;
}
std::string GetScanInfoFileName(int mat_id){
    return "/scan_info_"+NumberToString(mat_id)+".txt";
}
std::string GetAlignInfoFileName(int mat_id){
    return "/alignment_info_"+NumberToString(mat_id)+".txt";
}
