#include "stdio.h"
#include "time.h"
#include "dm_egyptian_pyramid_lib.h"
#include "dm_img_proc_lib_basics.h"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

using namespace std;
int egyptian_pyramid(const char* input_tile_dir, \
							const int mat_id, \
							dm_egyptian_pyramid_lib *devPtr){
    std::string align_info_path_str = input_tile_dir+GetAlignInfoFileName(mat_id);
	std::ifstream file_in;
	std::string new_line;
    int total_tile_num;
    std::map<pyramid_tile_index, pyramid_tile_obj> pyramid_tile_map;
    const int min_index_x = 0;//1;//
    const int max_index_x = 1000;//20;//
    const int min_index_y = 0;//35;//
    const int max_index_y = 1000;//54;//

#ifdef _WIN32
    clock_t start, finish;
    float elapsed_secs;
#else
	struct timespec start, finish;
	double elapsed_secs;
#endif // _WIN32
    /*perform image pyramiding*/
    {
        /*open txt file*/
        file_in.open(align_info_path_str.c_str());
		if(!file_in.is_open()){
			cout << "failed to open align info["<< mat_id <<"]" << endl;
			return -1;
		}
		cout << "start egyptian pyramid for mat["<< mat_id <<"]" << endl;
        /*read tile num*/
        total_tile_num = 0;
        while(!file_in.eof()){
            getline(file_in, new_line);
#ifdef _WIN32
            if(new_line=="[tile_num]"){
#else
            //if(new_line=="[tile_num]\r"){
            if(new_line=="[tile_num]"){
#endif
                getline(file_in, new_line);
                istringstream ss(new_line);
                string token;
                getline(ss, token);
                total_tile_num = atoi(token.c_str());
                break;
            }
        }
        cout << "total tile number is " << total_tile_num << endl;
        /*read align info*/
        int tile_count = 0;
        while(!file_in.eof()){
            getline(file_in, new_line);
#ifdef _WIN32
            if(new_line=="[align_tile_pos_scene_topleft]"){
#else
            //if(new_line=="[align_tile_pos_scene_topleft]\r"){
            if(new_line=="[align_tile_pos_scene_topleft]"){
#endif
                for(int i=0;i<total_tile_num;i++){
                    getline(file_in, new_line);
                    istringstream ss(new_line);
                    string token;
                    /*x index*/
                    getline(ss, token, ',');
                    int x_index = atoi(token.c_str());
                    /*y index*/
                    getline(ss, token, ',');
                    int y_index = atoi(token.c_str());
                    /*tl x pos*/
                    getline(ss, token, ',');
                    float tl_x = atof(token.c_str());
                    /*tl y pos*/
                    getline(ss, token, ',');
                    float tl_y = atof(token.c_str());
                    /*add to tile map*/
                    if( (x_index>=min_index_x)&& \
                        (x_index<=max_index_x)&& \
                        (y_index>=min_index_y)&& \
                        (y_index<=max_index_y) ){
                        pyramid_tile_map[ \
							pyramid_tile_index(x_index, y_index, 0) ] = \
							pyramid_tile_obj(tl_x, tl_y, 0, 0, 0, 0);
                        tile_count++;
                    }
                }
                break;
            }
        }
        cout << "align tile map" << " of " << tile_count << " is loaded" << endl;
		/*read resize info*/
		tile_count = 0;
        while(!file_in.eof()){
            getline(file_in, new_line);
#ifdef _WIN32
            if(new_line=="[resize_tile_pos_scene_topleft]"){
#else
            //if(new_line=="[resize_tile_pos_scene_topleft]\r"){
            if(new_line=="[resize_tile_pos_scene_topleft]"){
#endif
                for(int i=0;i<total_tile_num;i++){
                    getline(file_in, new_line);
                    istringstream ss(new_line);
                    string token;
                    /*x index*/
                    getline(ss, token, ',');
                    int x_index = atoi(token.c_str());
                    /*y index*/
                    getline(ss, token, ',');
                    int y_index = atoi(token.c_str());
                    /*tl x pos*/
                    getline(ss, token, ',');
                    float tl_x = atof(token.c_str());
                    /*tl y pos*/
                    getline(ss, token, ',');
                    float tl_y = atof(token.c_str());
					/*group id*/
                    getline(ss, token, ',');
					int group_id = atoi(token.c_str());
					(void) group_id;
					/*group size*/
                    getline(ss, token, ',');
					int group_size = atoi(token.c_str());
					(void) group_size;
					/*tile width*/
                    getline(ss, token, ',');
					int tile_width = atoi(token.c_str());
					/*tile height*/
                    getline(ss, token, ',');
					int tile_height = atoi(token.c_str());
                    /*add to tile map*/
                    if( (x_index>=min_index_x)&& \
                        (x_index<=max_index_x)&& \
                        (y_index>=min_index_y)&& \
                        (y_index<=max_index_y) ){
						std::map<pyramid_tile_index, pyramid_tile_obj>::iterator \
							tile_iter = pyramid_tile_map.find( \
									pyramid_tile_index(x_index, y_index, 0) );
						tile_iter->second.width = (unsigned int)tile_width;
						tile_iter->second.height = (unsigned int)tile_height;
						tile_iter->second.x_offset = (int)(tl_x - \
													 tile_iter->second.tl_x_pos);
						tile_iter->second.y_offset = (int)(tl_y - \
													 tile_iter->second.tl_y_pos);
						tile_iter->second.tl_x_pos = tl_x;
						tile_iter->second.tl_y_pos = tl_y;
                        tile_count++;
						cout << "[" << tile_count << "]," << tile_iter->first.x << "," \
							<< tile_iter->first.y << "," << tile_iter->first.pyramid_level << "," \
							<< tile_iter->second.tl_x_pos << "," << tile_iter->second.tl_y_pos << "," \
							<< tile_iter->second.width << "," << tile_iter->second.height << "," \
							<< tile_iter->second.x_offset << "," << tile_iter->second.y_offset << endl;
                    }
                }
                break;
            }
        }
        cout << "resize tile map" << " of " << tile_count << " is loaded" << endl;
        /*close txt file*/
        file_in.close();
        /*start egyptian pyramiding*/
		{
			int r;
			devPtr->clearAll();
			//register tile info
#ifdef _WIN32
			start = clock();
#else
			clock_gettime(CLOCK_MONOTONIC, &start);
#endif
			for(std::map<pyramid_tile_index, pyramid_tile_obj>::iterator \
					iter = pyramid_tile_map.begin(); \
					iter != pyramid_tile_map.end(); ++iter){
				r = devPtr->registerBaseTile(iter->first.x, iter->first.y, \
										iter->second.tl_x_pos, \
										iter->second.tl_y_pos, \
										iter->second.width, \
										iter->second.height, \
										iter->second.x_offset, \
										iter->second.y_offset);
				if(DM_IMG_PROC_RETURN_OK!=r){
					return -1;
				}
				cout << "tile info:" << iter->first.x << "," << iter->first.y << "," << \
						iter->second.tl_x_pos << "," << iter->second.tl_y_pos << "," << \
						iter->second.width << "," << iter->second.height << "," << \
						iter->second.x_offset << "," << iter->second.y_offset << endl;
			}
			cout << "tile registration finished";
#ifdef _WIN32
			finish = clock();
			elapsed_secs = (float)(finish-start)/CLOCKS_PER_SEC;
#else
			clock_gettime(CLOCK_MONOTONIC, &finish);
			elapsed_secs = (finish.tv_sec - start.tv_sec);
			elapsed_secs += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
#endif // _WIN32
			printf("with time elapsed %f s\n", elapsed_secs);

			//start pyramid building
#ifdef _WIN32
			start = clock();
#else
			clock_gettime(CLOCK_MONOTONIC, &start);
#endif

			devPtr->build(20, 20, 7);


			cout << "egyptian pyramiding is finished";
#ifdef _WIN32
			finish = clock();
			elapsed_secs = (float)(finish-start)/CLOCKS_PER_SEC;
#else
			clock_gettime(CLOCK_MONOTONIC, &finish);
			elapsed_secs = (finish.tv_sec - start.tv_sec);
			elapsed_secs += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
#endif // _WIN32
			printf("with time elapsed %f s\n", elapsed_secs);
		}
    }
    /*return*/
    return 0;
}

int main(int argc, char *argv[])
{
	std::string input_file_dir = "./data";
	std::string output_file_dir = "./output";
	dm_egyptian_pyramid_lib dui = dm_egyptian_pyramid_lib( \
													input_file_dir.c_str(), \
													output_file_dir.c_str(), \
													0, 256, 256, 16000.0);

	for (int i = 0; i < 100; ++i) {
		printf("test#%d starts\n", i+1);

		int r = egyptian_pyramid(input_file_dir.c_str(), \
									0, &dui);
		if(0!=r){
			printf("failed test\n");
		}

		printf("test#%d ends\n", i+1);
	}

	printf("hello world\n");
	return 0;
}
