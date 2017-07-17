#include <vector>
#include <queue>
#include <map>
#include "dm_img_lrucache_lib.h"
/*type definition*/
struct pyramid_tile_index{
	int x;
	int y;
	int pyramid_level;
	pyramid_tile_index():x(0),y(0),pyramid_level(0){}
	pyramid_tile_index(const int &x, const int &y, \
						const int &pyramid_level){
		this->x = x;
		this->y = y;
		this->pyramid_level = pyramid_level;
	}
};

inline bool operator<(const pyramid_tile_index &a, const pyramid_tile_index &b){
	if(a.pyramid_level<b.pyramid_level){
		return true;
	}
	else if(a.pyramid_level>b.pyramid_level){
		return false;
	}
	else if(a.y<b.y){
        return true;
    }
    else if(a.y>b.y){
        return false;
    }
    return (a.x<b.x);
}

inline bool operator==(const pyramid_tile_index &a, const pyramid_tile_index &b){
    return (a.x==b.x)&&(a.y==b.y)&&(a.pyramid_level==b.pyramid_level);
}

struct pyramid_tile_obj{
	float tl_x_pos;
	float tl_y_pos;
	unsigned int width;
	unsigned int height;
	int x_offset;
	int y_offset;
	pyramid_tile_obj():tl_x_pos(0),tl_y_pos(0),\
						   width(0),height(0),\
						   x_offset(0),y_offset(0){
	}
	pyramid_tile_obj(const float &tl_x_pos, const float &tl_y_pos, \
						const unsigned int &width, const unsigned int &height, \
						const int &x_offset, const int &y_offset){
		this->tl_x_pos = tl_x_pos;
		this->tl_y_pos = tl_y_pos;
		this->width = width;
		this->height = height;
		this->x_offset = x_offset;
		this->y_offset = y_offset;
	}
};

struct pyramid_layer_size{
	int tile_num_x;
	int tile_num_y;
	pyramid_layer_size():tile_num_x(0),tile_num_y(0){
	}
	pyramid_layer_size(const int &tile_num_x, \
						const int &tile_num_y){
		this->tile_num_x = tile_num_x;
		this->tile_num_y = tile_num_y;
	}
};
/*class declaration*/
class dm_egyptian_pyramid_lib{
	public:
		dm_egyptian_pyramid_lib(const char* file_dir, \
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
						int x_offset, int y_offset);
		/*start building*/
		int build(int tile_range_x, int tile_range_y, \
					int num_of_threads);
	protected:

	private:
		std::queue<std::vector<pyramid_tile_index>> work_queue;
		std::map<pyramid_tile_index, pyramid_tile_obj> base_tile_layout;
		std::map<pyramid_tile_index, pyramid_tile_obj> pyramid_tile_layout;
		std::vector<pyramid_layer_size> layer_sizes;
		std::string file_dir_str;
		int mat_id;
		unsigned int tile_width;
		unsigned int tile_height;
		int getFirstLayerFromBase();
		int calculateLayout();
		int generateWorkSchedule(int tile_range_x, int tile_range_y);
		void workerThread();
		void getInterSectingBaseTiles( \
				std::vector<pyramid_tile_index> &input_tiles, \
				std::vector<pyramid_tile_index> &output_base_tiles);
		dm_img_lrucache_lib lru_cache;
};
