/*included headers*/
#include "dm_img_proc_lib_basics.h"
#include "dm_egyptian_pyramid_lib.h"
#include "opencv2/opencv.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/stitching/detail/blenders.hpp"
#include <thread>
#include <mutex>
#include <chrono>
using namespace cv::detail;
/*constant definition*/
/*local var definition*/
std::mutex work_queue_mtx;
std::mutex lru_mtx;
std::mutex test_mtx;
/*local function definition*/
bool findRectIntersect(cv::Rect& rect1, cv::Rect& rect2, \
						cv::Rect& out_rect){
	int min_x, min_y, max_x, max_y;
	if ( (rect1.tl().x < rect2.br().x) && \
			(rect2.tl().x < rect1.br().x) && \
			(rect1.tl().y < rect2.br().y) && \
			(rect2.tl().y < rect1.br().y) ) {
		min_x = (rect1.tl().x<rect2.tl().x) ? \
				rect2.tl().x : rect1.tl().x;
		min_y = (rect1.tl().y<rect2.tl().y) ? \
				rect2.tl().y : rect1.tl().y;
		max_x = (rect1.br().x<rect2.br().x) ? \
				rect1.br().x : rect2.br().x;
		max_y = (rect1.br().y<rect2.br().y) ? \
				rect1.br().y : rect2.br().y;

		out_rect = cv::Rect(min_x, min_y, max_x-min_x, max_y-min_y);
		return true;
	}
	return false;
}
/*
 * class definition
 * */
dm_egyptian_pyramid_lib::dm_egyptian_pyramid_lib(const char* input_file_dir, \
												const char* output_file_dir, \
												const int mat_id, \
												const unsigned int tile_width, \
												const unsigned int tile_height, \
												const float max_cache_size_in_megabytes){
	this->input_file_dir_str = std::string(input_file_dir);
	this->output_file_dir_str = std::string(output_file_dir);
	this->mat_id = mat_id;
	this->tile_width = tile_width;
	this->tile_height = tile_height;
	this->lru_cache_ptr = new dm_img_lrucache_lib(max_cache_size_in_megabytes);
}
dm_egyptian_pyramid_lib::~dm_egyptian_pyramid_lib(){
	this->clearAll();
	delete this->lru_cache_ptr;
}
/*reset*/
void dm_egyptian_pyramid_lib::clearAll(){
	while(false==this->work_queue.empty()){
		this->work_queue.pop();
	}
	this->base_tile_layout.clear();
	this->pyramid_tile_layout.clear();
	this->layer_sizes.clear();
	this->lru_cache_ptr->clearAll();
}
/*register tile info*/
int dm_egyptian_pyramid_lib::registerBaseTile(int x_index, int y_index, \
										float tl_x_pos, float tl_y_pos, \
										unsigned int width, unsigned int height, \
										int x_offset, int y_offset){
	pyramid_tile_index new_tile_index = pyramid_tile_index(x_index, y_index, 0);
	pyramid_tile_obj  new_tile_obj = pyramid_tile_obj(tl_x_pos, tl_y_pos, \
														width, height, \
														x_offset, y_offset);
	this->base_tile_layout[new_tile_index] = new_tile_obj;
	return DM_IMG_PROC_RETURN_OK;
}
/*start building*/
int dm_egyptian_pyramid_lib::build(int tile_range_x, int tile_range_y, \
									int num_of_threads, unsigned int use_blender){
	/*check input*/
	if( (1>tile_range_x)||(1>tile_range_y) ){
		return DM_IMG_PROC_RETURN_FAIL;
	}
	else if(1>num_of_threads){
		return DM_IMG_PROC_RETURN_FAIL;
	}
	/*local var*/
	int r;
	std::vector<std::thread> workers;
	bool useBlender;
	/*init*/
	useBlender = (0==use_blender)?false:true;
	/*build*/
	{
		//calculate layout
		r = this->calculateLayout();
		if(DM_IMG_PROC_RETURN_OK!=r) return r;
		//generate work schedule
		r = this->generateWorkSchedule(tile_range_x, tile_range_y);
		if(DM_IMG_PROC_RETURN_OK!=r) return r;
		//start work thread
		for (int i = 0; i < num_of_threads; ++i) {
			workers.push_back(std::thread( \
						&dm_egyptian_pyramid_lib::workerThread, this, i, useBlender \
						));
		}
		//join threads
		for (int i = 0; i < num_of_threads; ++i) {
			workers[i].join();
		}
		//post process(output info)
	}
	/*return ok*/
	return DM_IMG_PROC_RETURN_OK;
}
/*generate pyramid layout*/
int dm_egyptian_pyramid_lib::getFirstLayerFromBase(){
	/*check input*/
	/*local var*/
	/*init*/
	this->pyramid_tile_layout.clear();
	this->layer_sizes.clear();
	/*generate first layer from base*/
	{
		//get the range of base layer
		float ul_limit_x, ul_limit_y, br_limit_x, br_limit_y;
		ul_limit_x = -1.f;
		ul_limit_y = -1.f;
		br_limit_x = -1.f;
		br_limit_y = -1.f;
		std::map<pyramid_tile_index, pyramid_tile_obj>::iterator tile_iter;
		for (tile_iter = this->base_tile_layout.begin(); \
				tile_iter != this->base_tile_layout.end(); \
				++tile_iter) {
			if(this->base_tile_layout.begin()== \
					tile_iter){
				ul_limit_x = tile_iter->second.tl_x_pos + \
							 (float)tile_iter->second.x_offset;
				ul_limit_y = tile_iter->second.tl_y_pos + \
							 (float)tile_iter->second.y_offset;
				br_limit_x = tile_iter->second.tl_x_pos + \
							 (float)tile_iter->second.x_offset + \
							 (float)tile_iter->second.width;
				br_limit_y = tile_iter->second.tl_y_pos + \
							 (float)tile_iter->second.y_offset + \
							 (float)tile_iter->second.height;
			}	   
			else{
				float temp_x, temp_y;

				temp_x = tile_iter->second.tl_x_pos + \
						 (float)tile_iter->second.x_offset;
				temp_y = tile_iter->second.tl_y_pos + \
						 (float)tile_iter->second.y_offset;
				ul_limit_x = (ul_limit_x < temp_x) ? \
							 ul_limit_x : temp_x;
				ul_limit_y = (ul_limit_y < temp_y) ? \
							 ul_limit_y : temp_y;

				temp_x = tile_iter->second.tl_x_pos + \
							(float)tile_iter->second.x_offset + \
							(float)tile_iter->second.width;
				temp_y = tile_iter->second.tl_y_pos + \
							(float)tile_iter->second.y_offset + \
							(float)tile_iter->second.height;
				br_limit_x = (br_limit_x < temp_x) ? \
							 temp_x : br_limit_x;
				br_limit_y = (br_limit_y < temp_y) ? \
							 temp_y : br_limit_y;
			}
		}
		if( (0>ul_limit_x)||(0>ul_limit_y)||(0>br_limit_x)||(0>br_limit_y) ){
			return DM_IMG_PROC_RETURN_FAIL;
		}
		//create first layer layout
		int tile_num_x = 1 + (int)( (br_limit_x-ul_limit_x)/(this->tile_width) );
		int tile_num_y = 1 + (int)( (br_limit_y-ul_limit_y)/(this->tile_height) );
		if( (1>=tile_num_x)||(1>=tile_num_y) ){
			return DM_IMG_PROC_RETURN_FAIL;
		}
		for (int i = 0; i < tile_num_x; ++i) {
			for (int j = 0; j < tile_num_y; ++j) {
				//tile index
				pyramid_tile_index new_tile_index(i+1, j+1, 1);
				//tile obj
				long long tl_x_pos = (long long)ul_limit_x + \
								 (i*this->tile_width);
				long long tl_y_pos = (long long)ul_limit_y + \
								 (j*this->tile_height);
				pyramid_tile_obj new_tile_obj((float)tl_x_pos, (float)tl_y_pos, \
											this->tile_width, \
											this->tile_height, \
											0, 0);
				this->pyramid_tile_layout[new_tile_index] = new_tile_obj;
			}
		}
		//update layer sizes
		this->layer_sizes.push_back( \
				pyramid_layer_size(tile_num_x, tile_num_y));
		printf("first layer size %d %d\n", tile_num_x, tile_num_y);
	}
	/*return*/
	return DM_IMG_PROC_RETURN_OK;
}
int dm_egyptian_pyramid_lib::calculateLayout(){
	/*check input*/
	/*local var*/
	int r;
	bool exitFlag;
	int prev_tile_num_x, prev_tile_num_y;
	std::map<pyramid_tile_index, pyramid_tile_obj>::iterator tile_iter;
	/*init*/
	exitFlag = false;
	/*generate pyramid layout*/
	{
		/*generate first layer from base*/
		r = this->getFirstLayerFromBase();
		if(DM_IMG_PROC_RETURN_OK!=r){
			return r;
		}
		/*generate higher layers iteratively or recursively*/
		int current_layer = 2;
		while(!exitFlag){
			//get prev_tile_num from vector
			prev_tile_num_x = this->layer_sizes[current_layer-2].tile_num_x;
			prev_tile_num_y = this->layer_sizes[current_layer-2].tile_num_y;
			//update tile_num_x and tile_num_y for current layer
			int cur_tile_num_x = (int)( (prev_tile_num_x+1)/2 );
			int cur_tile_num_y = (int)( (prev_tile_num_y+1)/2 );
			this->layer_sizes.push_back( \
					pyramid_layer_size(cur_tile_num_x, cur_tile_num_y) );
			printf("layer %d size %d %d\n", current_layer, \
					cur_tile_num_x, cur_tile_num_y);
			//generate layout for current layer
			for (int i = 0; i < cur_tile_num_x; ++i) {
				for (int j = 0; j < cur_tile_num_y; ++j) {
					tile_iter = this->pyramid_tile_layout.find( \
							pyramid_tile_index(2*i+1, 2*j+1, current_layer-1) \
							);
					if(this->pyramid_tile_layout.end() != \
							tile_iter){
						pyramid_tile_index new_tile_index(i+1, j+1, current_layer);
						pyramid_tile_obj new_tile_obj( \
													tile_iter->second.tl_x_pos, \
													tile_iter->second.tl_y_pos, \
													this->tile_width, \
													this->tile_height, \
													0, 0);
						this->pyramid_tile_layout[new_tile_index] = new_tile_obj;
					}
				}
			}
			//update exit flag and else
			current_layer++;
			prev_tile_num_x = cur_tile_num_x;
			prev_tile_num_y = cur_tile_num_y;
			if( (1==prev_tile_num_x)&&(1==prev_tile_num_y) ){
				exitFlag = true;
			}
		}
	}
	/*return*/
	return DM_IMG_PROC_RETURN_OK;
}
/*generate work schedules*/
int dm_egyptian_pyramid_lib::generateWorkSchedule( \
								int tile_range_x, int tile_range_y){
	/*check input*/
	if( (1>tile_range_x)||(1>tile_range_y) ){
		return DM_IMG_PROC_RETURN_FAIL;
	}
	/*local var*/
	int current_layer;
	int cur_tile_num_x, cur_tile_num_y;
	int max_block_num_x, max_block_num_y;
	std::vector<pyramid_tile_index> tile_list;
	int min_x, min_y, max_x, max_y;
	/*init*/
	current_layer = 1;
	while(false==this->work_queue.empty()){
		this->work_queue.pop();
	}
	/*generate work schedules*/
	while(current_layer<=(int)this->layer_sizes.size()){
		//get tile num for current layer
		cur_tile_num_x = this->layer_sizes[current_layer-1].tile_num_x;
		cur_tile_num_y = this->layer_sizes[current_layer-1].tile_num_y;
		//get block num for next work
		max_block_num_x = (cur_tile_num_x%tile_range_x) ? \
						  (cur_tile_num_x/tile_range_x + 1) : \
						  (cur_tile_num_x/tile_range_x);
		max_block_num_y = (cur_tile_num_y%tile_range_y) ? \
						  (cur_tile_num_y/tile_range_y + 1) : \
						  (cur_tile_num_y/tile_range_y);
		//create next work order
		for (int i = 0; i < max_block_num_x; ++i) {
			for (int j = 0; j < max_block_num_y; ++j) {
				min_x = tile_range_x*i+1;
				min_y = tile_range_y*j+1;
				max_x = (i==(max_block_num_x-1)) ? \
							cur_tile_num_x : ( tile_range_x*(i+1) );
				max_y = (j==(max_block_num_y-1)) ? \
							cur_tile_num_y : ( tile_range_y*(j+1) );
				tile_list.clear();
				for (int x_index = min_x; x_index <= max_x; ++x_index) {
					for (int y_index = min_y; y_index <= max_y; ++y_index) {
						tile_list.push_back( \
								pyramid_tile_index( \
									x_index, y_index, current_layer) );
					}
				}
				this->work_queue.push( tile_list );
			}
		}
		//update current_layer
		current_layer++;
	}	
	/*return*/
	return DM_IMG_PROC_RETURN_OK;
}
/*worker thread definition*/
void dm_egyptian_pyramid_lib::workerThread(const int thread_id, const bool useBlender){
	/*check input*/
	/*local var*/
	std::vector<pyramid_tile_index> tile_list;
	int current_layer;
	std::vector<int> comp_params;
	/*init*/
	{
		comp_params.push_back(cv::IMWRITE_JPEG_QUALITY);
		comp_params.push_back(75);
	}
	/*start worker thread*/
	while(1){
		//check work queue
		{
			std::lock_guard<std::mutex> lck(work_queue_mtx);
			if(this->work_queue.empty()){
				break;
			}
			tile_list = this->work_queue.front();
			this->work_queue.pop();
		}
		printf("thread[%d] got work of size %d at layer %d with useBlender[%d]!\n", \
				thread_id, (int)tile_list.size(), tile_list.front().pyramid_level, useBlender);
		//start processing work
		if(false == tile_list.empty()){
			//get current layer number
			current_layer = tile_list.front().pyramid_level;
			if (1==current_layer) {
				//find rectangle enclosing all tiles in this work
				//find tiles in base layer intersecting with the rect
				std::vector<pyramid_tile_index> base_tiles;
				this->getInterSectingBaseTiles(tile_list, base_tiles);
				cv::Rect block_rect;
				float min_x, min_y, max_x, max_y;
				float temp_x, temp_y;
				bool dataExists;
				//generate tile images for 1st layer
				if(false == base_tiles.empty()){
					//find the range of base layer images
					for(std::vector<pyramid_tile_index>::iterator \
							iter=base_tiles.begin(); \
						   iter!=base_tiles.end(); ++iter){
						std::map<pyramid_tile_index, pyramid_tile_obj>::iterator \
										layout_iter = this->base_tile_layout.find(*iter); 
						//update min_x, min_y, max_x, max_y
						if( iter==base_tiles.begin() ){
							min_x = layout_iter->second.tl_x_pos + \
									(float)(layout_iter->second.x_offset);
							min_y = layout_iter->second.tl_y_pos + \
									(float)(layout_iter->second.y_offset);
							max_x = min_x + (float)(layout_iter->second.width);
							max_y = min_y + (float)(layout_iter->second.height);
						}
						else{
							temp_x = layout_iter->second.tl_x_pos + \
										   (float)(layout_iter->second.x_offset);
							temp_y = layout_iter->second.tl_y_pos + \
										   (float)(layout_iter->second.y_offset);
							min_x = ( min_x < temp_x ) ? \
									min_x :  temp_x ;
							min_y = ( min_y < temp_y ) ? \
									min_y :  temp_y ;

							temp_x += (float)(layout_iter->second.width);
							temp_y += (float)(layout_iter->second.height);
							max_x = ( max_x > temp_x) ? \
									max_x : temp_x ;
							max_y = ( max_y > temp_y ) ? \
									max_y : temp_y ;
						}
					}	
					block_rect = cv::Rect((int)min_x, (int)min_y, \
											(int)(max_x-min_x), (int)(max_y-min_y));
					//find image data in cache or disk storage
					//perform image blending if possible
					cv::Mat block_image;
					cv::Ptr<Blender> blender;
					std::vector<cv::Point> tile_corners;
					std::vector<cv::Size> tile_sizes;
					bool boolUseGPU = false;
					unsigned int blend_width = 8;
					if(false==useBlender){
						block_image = cv::Mat::zeros((int)(max_y-min_y), \
													(int)(max_x-min_x), \
													CV_8UC3);
					}
					else{
						//create blender
						blender = Blender::createDefault(Blender::MULTI_BAND, boolUseGPU);
						MultiBandBlender *mb_ptr = \
						   dynamic_cast<MultiBandBlender*>(static_cast<Blender*>(blender));
						mb_ptr->setNumBands( \
							static_cast<int>(ceil(log((double)blend_width)/log(2.f))-1.f) );
						//prepare blender
						for(std::vector<pyramid_tile_index>::iterator \
								iter=base_tiles.begin(); \
								iter!=base_tiles.end(); ++iter){
							std::map<pyramid_tile_index, pyramid_tile_obj>::iterator \
											layout_iter = this->base_tile_layout.find(*iter);
							cv::Point tl_pos = \
										cv::Point( \
												layout_iter->second.tl_x_pos+ \
													(float)layout_iter->second.x_offset- \
													min_x, \
												layout_iter->second.tl_y_pos+ \
													(float)layout_iter->second.y_offset- \
													min_y
											);
							cv::Size tile_size = cv::Size(layout_iter->second.width, \
															layout_iter->second.height);
							tile_corners.push_back( tl_pos );
							tile_sizes.push_back( tile_size );
						}
						blender->prepare(tile_corners, tile_sizes);
					}
					for(std::vector<pyramid_tile_index>::iterator \
							iter=base_tiles.begin(); \
							iter!=base_tiles.end(); ++iter){
						std::map<pyramid_tile_index, pyramid_tile_obj>::iterator \
										layout_iter = this->base_tile_layout.find(*iter); 
						//get new_img either from disk or cache
						cv::Mat new_img;
						{
							{
								std::lock_guard<std::mutex> lck(lru_mtx);
								dataExists = this->lru_cache_ptr->get(*iter, new_img);
							}
							if( false == dataExists ){
								//test_mtx.lock();
								cv::Mat raw_img = cv::imread( \
										this->input_file_dir_str+"/tile_"+ \
										NumberToString(this->mat_id)+ \
										"_"+NumberToString(iter->x)+ \
										"_"+NumberToString(iter->y)+".jpg", \
										cv::IMREAD_UNCHANGED);
								//test_mtx.unlock();
								if(false==raw_img.empty()){
									cv::Point img_offset = cv::Point( \
											layout_iter->second.x_offset, \
											layout_iter->second.y_offset );
									cv::Size img_size = cv::Size( \
											layout_iter->second.width, \
											layout_iter->second.height );
									new_img = cv::Mat(raw_img, \
												cv::Rect(img_offset, img_size));
									{
										std::lock_guard<std::mutex> lck(lru_mtx);
										this->lru_cache_ptr->put(*iter, new_img);
									}
								}
								
								////test_mtx.lock();
								//std::string image_path_str = \
										//this->input_file_dir_str+"/tile_"+ \
										//NumberToString(this->mat_id)+ \
										//"_"+NumberToString(iter->x)+ \
										//"_"+NumberToString(iter->y)+".jpg";
								//FILE* pFile = fopen(image_path_str.c_str(), "rb");
								//fseek(pFile, 0, SEEK_END);
								//size_t jpg_size = ftell(pFile);
								//fseek(pFile, 0, SEEK_SET);
								//char* raw_buffer = new char[jpg_size];
								//fread(raw_buffer, sizeof(char), jpg_size, pFile);
								//cv::Mat raw_img = cv::imdecode( \
										//cv::Mat(1, jpg_size, CV_8UC1, raw_buffer), \
										//CV_LOAD_IMAGE_UNCHANGED );
								////test_mtx.unlock();
								//if(false==raw_img.empty()){
									//cv::Point img_offset = cv::Point( \
											//layout_iter->second.x_offset, \
											//layout_iter->second.y_offset );
									//cv::Size img_size = cv::Size( \
											//layout_iter->second.width, \
											//layout_iter->second.height );
									//new_img = cv::Mat(raw_img, \
												//cv::Rect(img_offset, img_size)).clone();
									//{
										//std::lock_guard<std::mutex> lck(lru_mtx);
										//this->lru_cache_ptr->put(*iter, new_img);
									//}
								//}
								//raw_img.release();
								//delete[] raw_buffer;
							}
						}
						if(false==useBlender){
							//copy it to block_image
							new_img.copyTo( block_image(cv::Rect( \
								layout_iter->second.tl_x_pos+(float)layout_iter->second.x_offset- \
									min_x, \
								layout_iter->second.tl_y_pos+(float)layout_iter->second.y_offset- \
									min_y, \
								layout_iter->second.width, layout_iter->second.height)) );
						}
						else{
							//add image to blender
							cv::Mat tile_mask = cv::Mat::zeros(new_img.size(), CV_8U);
							tile_mask( \
										cv::Rect(0, 0, \
											layout_iter->second.width, \
											layout_iter->second.height) \
									).setTo(cv::Scalar::all(255));
							cv::Point tl_pos = \
										cv::Point( \
												layout_iter->second.tl_x_pos+ \
													(float)layout_iter->second.x_offset- \
													min_x, \
												layout_iter->second.tl_y_pos+ \
													(float)layout_iter->second.y_offset- \
													min_y
											);
							cv::Mat image_s;
							new_img.convertTo(image_s, CV_16S);
							blender->feed( \
									image_s, tile_mask, tl_pos \
									);
						}
					}
					if(useBlender){
						//perform blending
						cv::Mat out_image_s, out_mask;
						blender->blend(out_image_s, out_mask);
						out_image_s.convertTo(block_image, CV_8U);
					}
					//output to tile with tile_width and tile_height
					for(std::vector<pyramid_tile_index>::iterator \
							iter=tile_list.begin(); \
							iter!=tile_list.end(); ++iter){
						std::map<pyramid_tile_index, pyramid_tile_obj>::iterator \
										layout_iter = this->pyramid_tile_layout.find(*iter); 
						cv::Mat new_img = cv::Mat::zeros(layout_iter->second.height, \
														layout_iter->second.width, 
														CV_8UC3);
						cv::Rect tile_rect = cv::Rect( \
								(int)(layout_iter->second.tl_x_pos+layout_iter->second.x_offset), \
								(int)(layout_iter->second.tl_y_pos+layout_iter->second.y_offset), \
								layout_iter->second.width, layout_iter->second.height);
						//find intersection of tile_rect and block_rect
						cv::Rect out_rect;
						if( findRectIntersect(block_rect, tile_rect, out_rect) ){
							cv::Mat copy_image = block_image(cv::Rect(out_rect.tl().x-min_x, \
												out_rect.tl().y-min_y, \
												out_rect.width, out_rect.height));
							copy_image.copyTo( new_img( cv::Rect( \
											out_rect.tl().x-(int)layout_iter->second.tl_x_pos, \
											out_rect.tl().y-(int)layout_iter->second.tl_y_pos, \
											out_rect.width, out_rect.height) ) );
						}
						//save to disk and cache
						{
							std::lock_guard<std::mutex> lck(lru_mtx);
							this->lru_cache_ptr->put(*iter, new_img);
						}
						{
							std::string out_string = output_file_dir_str+"/pr_"+ \
													 NumberToString(this->mat_id)+"_"+ \
													 NumberToString(iter->x)+"_"+ \
													 NumberToString(iter->y)+"_"+ \
													 NumberToString(iter->pyramid_level)+".jpg";
							std::vector<uchar> jpg_image;
							cv::imencode(".jpg", new_img, jpg_image, comp_params);
							FILE *pFile = std::fopen(out_string.c_str(), \
														"wb");
							std::fwrite(&jpg_image[0], sizeof(char), \
											jpg_image.size(), pFile);
							fclose(pFile);
						}
					}
				}
			}
			else{
				for(std::vector<pyramid_tile_index>::iterator \
						iter=tile_list.begin(); \
						iter!=tile_list.end(); ++iter){
					//find tiles in the previous layer
					pyramid_tile_index tl_index, tr_index, bl_index, br_index;
					tl_index = pyramid_tile_index( \
										2*(iter->x)-1, 2*(iter->y)-1, \
										iter->pyramid_level-1 \
										);
					tr_index = pyramid_tile_index( \
										2*(iter->x), 2*(iter->y)-1, \
										iter->pyramid_level-1 \
										);
					bl_index = pyramid_tile_index( \
										2*(iter->x)-1, 2*(iter->y), \
										iter->pyramid_level-1 \
										);
					br_index = pyramid_tile_index( \
										2*(iter->x), 2*(iter->y), \
										iter->pyramid_level-1 \
										);
					//find image data in cache or disk storage
					//or wait until data is available
					cv::Mat block_image = cv::Mat::zeros((int)(2*(this->tile_height)), \
														(int)(2*(this->tile_width)), \
														CV_8UC3);
					{
						//tl
						this->copyToBlock(tl_index, block_image, \
								cv::Rect(0, 0, this->tile_width, this->tile_height) );
						//tr
						this->copyToBlock(tr_index, block_image, \
								cv::Rect(this->tile_width, 0, \
									this->tile_width, this->tile_height) );
						//bl
						this->copyToBlock(bl_index, block_image, \
								cv::Rect(0, this->tile_height, \
									this->tile_width, this->tile_height) );
						//br
						this->copyToBlock(br_index, block_image, \
								cv::Rect(this->tile_width, this->tile_height, \
									this->tile_width, this->tile_height) );
					}
					//generate tile images for 2nd layer and above
					cv::Mat new_img;
					cv::resize(block_image, new_img, cv::Size(), 0.5, 0.5, cv::INTER_AREA);
					//save to disk and cache
					{
						std::lock_guard<std::mutex> lck(lru_mtx);
						this->lru_cache_ptr->put(*iter, new_img);
					}
					{
						std::string out_string = output_file_dir_str+"/pr_"+ \
												 NumberToString(this->mat_id)+"_"+ \
												 NumberToString(iter->x)+"_"+ \
												 NumberToString(iter->y)+"_"+ \
												 NumberToString(iter->pyramid_level)+".jpg";
						std::vector<uchar> jpg_image;
						cv::imencode(".jpg", new_img, jpg_image, comp_params);
						FILE *pFile = std::fopen(out_string.c_str(), \
													"wb");
						std::fwrite(&jpg_image[0], sizeof(char), \
										jpg_image.size(), pFile);
						fclose(pFile);
					}
				}
			}
		}
	}
	/*return*/
	return;
}
/*get intersecting base tiles*/
void dm_egyptian_pyramid_lib::getInterSectingBaseTiles( \
							std::vector<pyramid_tile_index> &input_tiles, \
							std::vector<pyramid_tile_index> &output_base_tiles){
	/*check input*/
	if(input_tiles.empty()){
		return;
	}
	/*local var*/
	int min_index_x, min_index_y, max_index_x, max_index_y;
	float ul_limit_x, ul_limit_y, br_limit_x, br_limit_y;
	std::map<pyramid_tile_index, pyramid_tile_obj>::iterator layout_iter;
	/*init*/
	output_base_tiles.clear();
	/*get intersecting base tiles*/
	{
		//get tile index range
		for (std::vector<pyramid_tile_index>::iterator \
				iter = input_tiles.begin(); \
				iter != input_tiles.end(); ++iter) {
			if ( iter==input_tiles.begin() ) {
				min_index_x = iter->x;
				min_index_y = iter->y;
				max_index_x = min_index_x;
				max_index_y = min_index_y;
			}
			else{
				min_index_x = (min_index_x<(iter->x)) ? \
							  min_index_x : (iter->x) ;
				min_index_y = (min_index_y<(iter->y)) ? \
							  min_index_y : (iter->y) ;
				max_index_x = (max_index_x>=(iter->x)) ? \
							  max_index_x : (iter->x) ;
				max_index_y = (max_index_y>=(iter->y)) ? \
							  max_index_y : (iter->y) ;
			}
		}
		//get pixel range
		layout_iter = this->pyramid_tile_layout.find( \
								pyramid_tile_index(min_index_x, min_index_y, 1) );
		if(layout_iter==this->pyramid_tile_layout.end()){
			return;
		}
		ul_limit_x = layout_iter->second.tl_x_pos;
		ul_limit_y = layout_iter->second.tl_y_pos;
		
		layout_iter = this->pyramid_tile_layout.find( \
								pyramid_tile_index(max_index_x, max_index_y, 1) );
		if(layout_iter==this->pyramid_tile_layout.end()){
			return;
		}
		br_limit_x = layout_iter->second.tl_x_pos + \
					 (float)(this->tile_width);
		br_limit_y = layout_iter->second.tl_y_pos + \
					 (float)(this->tile_height);
		//find all intersecting base tiles
		for (layout_iter = this->base_tile_layout.begin(); \
				layout_iter != this->base_tile_layout.end(); \
				++layout_iter) {
			float min_pos_x = layout_iter->second.tl_x_pos + \
							  (float)(layout_iter->second.x_offset);
			float min_pos_y = layout_iter->second.tl_y_pos + \
							  (float)(layout_iter->second.y_offset);
			float max_pos_x = min_pos_x + \
							  (float)(layout_iter->second.width);
			float max_pos_y = min_pos_y + \
							  (float)(layout_iter->second.height);

			if( \
					(br_limit_x>min_pos_x)&&(max_pos_x>ul_limit_x)&& \
					(br_limit_y>min_pos_y)&&(max_pos_y>ul_limit_y)
					){//intersecting
				output_base_tiles.push_back( layout_iter->first );
			}
		}
	}
	/*return*/
	return;
}
/*copy image data to image block*/
void dm_egyptian_pyramid_lib::copyToBlock(pyramid_tile_index &target_index, \
											cv::Mat &block_image, \
											cv::Rect copyArea){
	/*local var*/
	bool exitFlag, dataExists;
	std::map<pyramid_tile_index, pyramid_tile_obj>::iterator \
					layout_iter;
	/*init*/
	exitFlag = false;
	layout_iter = this->pyramid_tile_layout.find(target_index);
	/*copy to block*/
	if(this->pyramid_tile_layout.end()!=layout_iter){
		cv::Mat raw_img;
		while(false==exitFlag){
			{
				std::lock_guard<std::mutex> lck(lru_mtx);
				dataExists = \
							 this->lru_cache_ptr->get(target_index, raw_img);
			}
			if(false==dataExists){
				//test_mtx.lock();
				raw_img = cv::imread( \
						this->output_file_dir_str+"/pr_"+ \
						NumberToString(this->mat_id)+ \
						"_"+NumberToString(target_index.x)+ \
						"_"+NumberToString(target_index.y)+ \
						"_"+NumberToString(target_index.pyramid_level)+".jpg", \
						cv::IMREAD_UNCHANGED);
				//test_mtx.unlock();
				if(false==raw_img.empty()){
					exitFlag = true;
				}
				else std::this_thread::sleep_for( \
						std::chrono::milliseconds(10));
			}
			else exitFlag = true;
		}
		raw_img.copyTo( block_image( copyArea ) );
		
		//cv::Mat raw_img;
		//while(false==exitFlag){
			//{
				//std::lock_guard<std::mutex> lck(lru_mtx);
				//dataExists = \
							 //this->lru_cache_ptr->get(target_index, raw_img);
			//}
			//if(false==dataExists){
				////test_mtx.lock();
				//std::string image_path_str = \
						//this->output_file_dir_str+"/pr_"+ \
						//NumberToString(this->mat_id)+ \
						//"_"+NumberToString(target_index.x)+ \
						//"_"+NumberToString(target_index.y)+ \
						//"_"+NumberToString(target_index.pyramid_level)+".jpg";
				//FILE* pFile = fopen(image_path_str.c_str(), "rb");
				//fseek(pFile, 0, SEEK_END);
				//size_t jpg_size = ftell(pFile);
				//fseek(pFile, 0, SEEK_SET);
				//char* raw_buffer = new char[jpg_size];
				//fread(raw_buffer, sizeof(char), jpg_size, pFile);
				//raw_img = cv::imdecode( \
						//cv::Mat(1, jpg_size, CV_8UC1, raw_buffer), \
						//CV_LOAD_IMAGE_UNCHANGED );
				////delete[] raw_buffer;
				////test_mtx.unlock();
				//if(false==raw_img.empty()){
					//exitFlag = true;
				//}
				//else std::this_thread::sleep_for( \
						//std::chrono::milliseconds(10));
			//}
			//else exitFlag = true;
		//}
		//raw_img.copyTo( block_image( copyArea ) );
	}
	/*return*/
	return;
}
