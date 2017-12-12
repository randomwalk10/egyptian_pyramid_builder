#ifndef DM_EGYPTIAN_PYRAMID_LIB_H
#define DM_EGYPTIAN_PYRAMID_LIB_H
#include <vector>
#include <queue>
#include <map>
#include "dm_img_lrucache_lib.h"
#include "dm_egyptian_pyramid_basics.h"
/*type definition*/
/*class declaration*/
class dm_egyptian_pyramid_lib{
	public:
		dm_egyptian_pyramid_lib(const char* input_file_dir, \
								const char* output_file_dir, \
								const int mat_id, \
								const unsigned int tile_width, \
								const unsigned int tile_height, \
								const float max_cache_size_in_megabytes);
		virtual ~dm_egyptian_pyramid_lib();
		/*reset*/
		void clearAll();
		/*register tile info*/
		int registerBaseTile(int x_index, int y_index, \
						float tl_x_pos, float tl_y_pos, \
						unsigned int width, unsigned int height, \
						int x_offset, int y_offset, \
						long long byte_pos, int byte_size);
		/*start building*/
		int build(int tile_range_x, int tile_range_y, \
					int num_of_threads, unsigned int use_blender);
	protected:

	private:
		std::queue<std::vector<pyramid_tile_index>> work_queue;
		std::map<pyramid_tile_index, base_tile_obj> base_tile_layout;
		std::map<pyramid_tile_index, pyramid_tile_obj> pyramid_tile_layout;
		std::vector<pyramid_layer_size> layer_sizes;
		std::string input_file_dir_str, output_file_dir_str;
#ifndef SINGLE_OUTPUT
		std::vector<pyramid_tile_index> processed_tiles;
#else
		std::map<pyramid_tile_index, std::vector<long long>> processed_tiles;
#endif
		int mat_id;
		unsigned int tile_width;
		unsigned int tile_height;
		int getFirstLayerFromBase();
		int calculateLayout();
		int generateWorkSchedule(int tile_range_x, int tile_range_y);
		void workerThread(const int thread_id, const bool useBlender);
		void getInterSectingBaseTiles( \
				std::vector<pyramid_tile_index> &input_tiles, \
				std::vector<pyramid_tile_index> &output_base_tiles);
		void copyToBlock(pyramid_tile_index &target_index, \
				cv::Mat &block_image, \
				cv::Rect copyArea);
		int outputPyramidInfo();
		dm_img_lrucache_lib *lru_cache_ptr;
};
#endif
