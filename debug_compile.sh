if [[ "$1" == "s" ]]; then
	echo "single mode(debug)"
	g++ -D SINGLE_OUTPUT -std=c++11 -O0 -g -c ./dm_img_proc_lib_basics.cpp
	g++ -D SINGLE_OUTPUT -std=c++11 -O0 -g -c ./dm_egyptian_pyramid_lib.cpp
	g++ -D SINGLE_OUTPUT -std=c++11 -O0 -g -c ./dm_img_lrucache_lib.cpp
	g++ -D SINGLE_OUTPUT -std=c++11 -O0 -g -c ./main.cpp
else
	echo "multi mode(debug)"
	g++ -std=c++11 -O0 -g -c ./dm_img_proc_lib_basics.cpp
	g++ -std=c++11 -O0 -g -c ./dm_egyptian_pyramid_lib.cpp
	g++ -std=c++11 -O0 -g -c ./dm_img_lrucache_lib.cpp
	g++ -std=c++11 -O0 -g -c ./main.cpp
fi

g++ -std=c++11 -O0 -g ./dm_img_proc_lib_basics.o ./dm_egyptian_pyramid_lib.o ./dm_img_lrucache_lib.o ./main.o -o ./debug_egyptian_pyramid_tester -L/usr/local/lib -L/usr/lib/x86_64-linux-gnu -lpthread -lopencv_imgcodecs -lopencv_core -lopencv_imgproc -lopencv_stitching

