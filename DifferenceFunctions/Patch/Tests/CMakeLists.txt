include_directories(../)

# Allow Qt to find it's MOCed files
include_directories(${CMAKE_CURRENT_BINARY_DIR}../)

add_executable(TestGMHDifference TestGMHDifference.cpp ../GMHDifference.hpp)
target_link_libraries(TestGMHDifference ${PatchBasedInpainting_libraries})
add_test(TestGMHDifference TestGMHDifference)

add_executable(TestImagePatchDifference TestImagePatchDifference.cpp ../ImagePatchDifference.hpp)
target_link_libraries(TestImagePatchDifference ${PatchBasedInpainting_libraries})
add_test(TestImagePatchDifference TestImagePatchDifference)
