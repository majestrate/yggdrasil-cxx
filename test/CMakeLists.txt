

add_executable(test_all
	main_test.cpp
	test_varint.cpp)

target_link_libraries(test_all PUBLIC Catch2 yggdrasil_lib)