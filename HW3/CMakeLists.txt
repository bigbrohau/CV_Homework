add_executable(
    HW3
    HW3.cpp
)

target_link_libraries(
    HW3
    PRIVATE
    ${OpenCV_LIBS}
)

add_custom_command(
    TARGET HW3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/lena.bmp"
        "$<TARGET_FILE_DIR:HW3>/lena.bmp"
)