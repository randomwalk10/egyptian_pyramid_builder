#ifndef _DM_IMG_LRUCACHE_LIB_H_
#define _DM_IMG_LRUCACHE_LIB_H_
/*included headers*/
#include "dm_egyptian_pyramid_lib.h"
#include "opencv2/opencv.hpp"
#include <unordered_map>
#include <list>
#include <utility>
/*type definition*/
typedef std::pair<pyramid_tile_index, cv::Mat> key_value_pair_t;
typedef std::list<key_value_pair_t>::iterator list_iterator_t;
/*class declaration*/
class dm_img_lrucache_lib{
	public:
		dm_img_lrucache_lib(float max_size_in_megabytes);
		virtual ~dm_img_lrucache_lib();
		void clearAll();
		void put(const pyramid_tile_index& index, \
					const cv::Mat& img);
		bool get(const pyramid_tile_index& index, \
					cv::Mat& out_img);

	private:
		std::list<key_value_pair_t> _cache_items_list;
		std::unordered_map<pyramid_tile_index, list_iterator_t> _cache_items_map;
		float _max_size;
		float _cur_size;
};

#endif /*_DM_IMG_LRUCACHE_LIB_H_*/
