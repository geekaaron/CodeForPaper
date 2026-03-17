project (optee_example_seg_with_optee C)

set (SRC host/main.c)

add_executable (${PROJECT_NAME} ${SRC})

include_directories("/home/q/projects/rpi3/buildroot/output/staging/usr/include")
link_directories("/home/q/projects/rpi3/buildroot/output/staging/usr/lib")

target_include_directories(${PROJECT_NAME}
			   PRIVATE ta/include
			   PRIVATE include)

target_link_libraries (${PROJECT_NAME} PRIVATE teec
    /home/q/projects/rpi3/buildroot/output/staging/usr/lib/libcurl.so
	/home/q/projects/rpi3/buildroot/output/staging/usr/lib/libssl.so
    /home/q/projects/rpi3/buildroot/output/staging/usr/lib/libcrypto.so
	/home/q/projects/rpi3/buildroot/output/staging/usr/lib/libz.so)

install (TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})
