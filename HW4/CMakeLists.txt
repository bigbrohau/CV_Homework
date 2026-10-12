add_executable(
    HW4
    HW4.cpp
)

target_link_libraries(
    HW4
    PRIVATE
    ${OpenCV_LIBS}
)

add_custom_command(
    TARGET HW4 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/lena.bmp"
        "$<TARGET_FILE_DIR:HW4>/lena.bmp"
)