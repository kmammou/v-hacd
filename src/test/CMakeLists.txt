project(VHACD_TEST)
include(${CMAKE_COMMON_INC})
add_executable(testVHACD ${PROJECT_CPP_FILES} ${PROJECT_C_FILES} ${PROJECT_INC_FILES} ${PROJECT_INL_FILES})

target_link_libraries(testVHACD VHACD_LIB)
include_directories("${CMAKE_SOURCE_DIR}/VHACD_Lib/inc")
