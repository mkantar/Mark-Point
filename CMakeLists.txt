cmake_minimum_required(VERSION 2.8)
project( corr_marking_tool )
find_package( OpenCV REQUIRED )
add_executable( corr_marking_tool corr_marking_tool.cpp )
target_link_libraries( corr_marking_tool ${OpenCV_LIBS} )
