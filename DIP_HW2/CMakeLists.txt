add_executable(
    DIP_HW2
    DIP_HW2.cpp
)

target_link_libraries(
    DIP_HW2
    PRIVATE
    ${OpenCV_LIBS}
)

add_custom_command(
    TARGET DIP_HW2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/selfie.jpg"
        "$<TARGET_FILE_DIR:DIP_HW2>/selfie.jpg"
)