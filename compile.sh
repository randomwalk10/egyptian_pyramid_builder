g++ -std=c++11 -O4 -c ./dm_img_proc_lib_basics.cpp
if [[ "$1" == "s" ]]; then
	echo "single mode"
	g++ -D SINGLE_OUTPUT -std=c++11 -O4 -c ./dm_egyptian_pyramid_lib.cpp
else
	echo "multi mode"
	g++ -std=c++11 -O4 -c ./dm_egyptian_pyramid_lib.cpp
fi
g++ -std=c++11 -O4 -c ./dm_img_lrucache_lib.cpp
g++ -std=c++11 -O4 -c ./main.cpp

g++ -std=c++11 -O4 ./dm_img_proc_lib_basics.o ./dm_egyptian_pyramid_lib.o ./dm_img_lrucache_lib.o ./main.o -o ./egyptian_pyramid_tester -L/usr/local/lib -L/usr/lib/x86_64-linux-gnu -lpthread -lopencv_imgcodecs -lopencv_core -lopencv_imgproc -lopencv_stitching

