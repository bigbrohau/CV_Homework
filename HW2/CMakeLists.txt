add_executable(
    HW2
    HW2.cpp
)

target_link_libraries(
    HW2
    PRIVATE
    ${OpenCV_LIBS}
)

add_custom_command(
    TARGET HW2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/lena.bmp"
        "$<TARGET_FILE_DIR:HW2>/lena.bmp"
)