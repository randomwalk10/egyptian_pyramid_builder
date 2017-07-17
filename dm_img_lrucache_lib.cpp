/*included headers*/
#include "dm_img_lrucache_lib.h"
/*constant definition*/
#define NUM_OF_MEGA 1000000.f
/*local function definition*/
/*class definition*/
dm_img_lrucache_lib::dm_img_lrucache_lib(float max_size_in_megabytes){
	this->_max_size = max_size_in_megabytes;
	this->_cur_size = 0.f;
}
dm_img_lrucache_lib::~dm_img_lrucache_lib(){
	this->clearAll();
}
//clear all
void dm_img_lrucache_lib::clearAll(){
	this->_cache_items_list.clear();
	this->_cache_items_map.clear();
	this->_cur_size = 0.f;
}
//insert
void dm_img_lrucache_lib::put(const pyramid_tile_index& index, \
								const cv::Mat& img){
	auto it = this->_cache_items_map.find(index);
	this->_cache_items_list.push_front(key_value_pair_t(index, img));
	if (it != this->_cache_items_map.end()) {
		this->_cur_size-=( it->second->second.total() ) * \
						 ( it->second->second.elemSize() ) / \
						 NUM_OF_MEGA;
		this->_cache_items_list.erase(it->second);
		this->_cache_items_map.erase(it);
	}
	this->_cache_items_map[index] = this->_cache_items_list.begin();
	this->_cur_size+=( this->_cache_items_list.begin()->second.total() ) * \
					  ( this->_cache_items_list.begin()->second.elemSize() ) / \
					  NUM_OF_MEGA;

	while(this->_cur_size>this->_max_size){
		auto last = this->_cache_items_list.end();
		last--;
		this->_cur_size-=( last->second.total() ) * \
							( last->second.elemSize() ) / \
							NUM_OF_MEGA;
		this->_cache_items_map.erase(last->first);
		this->_cache_items_list.pop_back();
	}
}
//get
bool dm_img_lrucache_lib::get(const pyramid_tile_index& index, \
								cv::Mat& out_img){
	auto it = this->_cache_items_map.find(index);
	if (it == this->_cache_items_map.end() ){
		return false;
	}
	this->_cache_items_list.splice(this->_cache_items_list.begin(), \
									this->_cache_items_list, it->second);
	out_img = it->second->second;

	return true;
}
