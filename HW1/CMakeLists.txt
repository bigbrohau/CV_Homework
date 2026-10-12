add_executable(
    HW1
    HW1.cpp
)

target_link_libraries(
    HW1
    PRIVATE
    ${OpenCV_LIBS}
)

add_custom_command(
    TARGET HW1 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/lena.bmp"
        "$<TARGET_FILE_DIR:HW1>/lena.bmp"
)