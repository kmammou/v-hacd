project(VHACD_LIB CXX C)
include(${CMAKE_COMMON_INC})

if (NOT NO_OPENCL)
	#include(FindOpenCL OPTIONAL)
	find_package(OpenCL)
endif()
if (NOT NO_OPENMP)
	#include(FindOpenMP OPTIONAL)
	find_package(OpenMP)
endif()
if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()
set(LIB_TYPE "STATIC" CACHE STRING "STATIC, SHARED or MODULE?")
message("[VHACD] \t LIB_TYPE " ${LIB_TYPE})
add_library(VHACD_LIB ${PROJECT_CPP_FILES} ${PROJECT_C_FILES} ${PROJECT_INC_FILES} ${PROJECT_INL_FILES} ${PROJECT_CL_FILES})

if (OpenCL_FOUND)
    target_include_directories(VHACD_LIB PRIVATE "${OpenCL_INCLUDE_DIRS}")
    target_compile_definitions(VHACD_LIB PRIVATE -DOPENCL_FOUND=1 )
    target_compile_definitions(VHACD_LIB PRIVATE -DOPENCL_CL_FILES="${PROJECT_CL_FILES}" )
endif()




message("[VHACD] \t -> CMAKE_INSTALL_PREFIX " ${CMAKE_INSTALL_PREFIX})
install(TARGETS VHACD_LIB EXPORT VHACD_LIB-targets DESTINATION lib)
install(FILES ${PROJECT_INC_FILES} DESTINATION include)
install(FILES ${PROJECT_INL_FILES} DESTINATION include)


set(VHACD_LIB_VERSION 3.2.0)
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/VHACD_LIB/VHACD_LIB-config-version.cmake"
  VERSION ${VHACD_LIB_VERSION}
  COMPATIBILITY AnyNewerVersion
)

export(EXPORT VHACD_LIB-targets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/VHACD_LIB/VHACD_LIB-targets.cmake"
)

configure_file(cmake/VHACD_LIB-config.cmake
  "${CMAKE_CURRENT_BINARY_DIR}/VHACD_LIB/VHACD_LIB-config.cmake"
  COPYONLY
)

set(ConfigPackageLocation lib/cmake/VHACD_LIB)
install(EXPORT VHACD_LIB-targets
  FILE
    VHACD_LIB-targets.cmake
  DESTINATION
    ${ConfigPackageLocation}
)
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/VHACD_LIB/VHACD_LIB-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/VHACD_LIB/VHACD_LIB-config-version.cmake"
  DESTINATION
    ${ConfigPackageLocation}
  COMPONENT
    Devel
)

