add_executable(test_1)
target_sources(test_1 PRIVATE test_1.cpp)
target_link_libraries(test_1 PRIVATE vendor)
add_test(NAME test_1 COMMAND $<TARGET_FILE:test_1>)

add_executable(test_2)
target_sources(test_2 PRIVATE test_2.cpp)
target_link_libraries(test_2 PRIVATE vendor)
add_test(NAME test_2 COMMAND $<TARGET_FILE:test_2>)