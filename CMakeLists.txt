cmake_minimum_required(VERSION 3.26.4)

project(imgInspect)

add_library(imgInspect INTERFACE)
target_sources(imgInspect PRIVATE
	"imgInspect.h"
)
target_include_directories(imgInspect INTERFACE .)