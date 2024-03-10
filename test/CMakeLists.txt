

add_executable(test_all
	main_test.cpp
	test_varint.cpp)

target_link_libraries(test_all PUBLIC Catch2 yggdrasil_lib)
add_custom_target(check DEPENDS test_all COMMAND $<TARGET_FILE:test_all>)