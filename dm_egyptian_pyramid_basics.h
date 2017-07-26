#ifndef DM_EGYPTIAN_PYRAMID_BASICS_H
#define DM_EGYPTIAN_PYRAMID_BASICS_H
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
#endif
