project(VHACD_TEST)
include(${CMAKE_COMMON_INC})

include(FindOpenCL)
include(FindOpenMP)
if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()


add_executable(testVHACD ${PROJECT_CPP_FILES} ${PROJECT_C_FILES} ${PROJECT_INC_FILES} ${PROJECT_INL_FILES})

include_directories("${CMAKE_SOURCE_DIR}/VHACD_Lib/public")

if (OpenCL_FOUND)
    include_directories("${OpenCL_INCLUDE_DIRS}")
    add_definitions( -DOPENCL_FOUND=1 )
endif()

IF(WIN32)
target_link_libraries(testVHACD ${OpenCL_LIBRARIES} VHACD_LIB)
ELSEIF(APPLE)
target_link_libraries(testVHACD ${OpenCL_LIBRARIES} VHACD_LIB)
ELSE ()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")
target_link_libraries(testVHACD ${OpenCL_LIBRARIES} VHACD_LIB rt)
ENDIF()

