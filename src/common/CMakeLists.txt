if (${CMAKE_SYSTEM_NAME} EQUAL "Linux")
 add_definitions(-DLINUX_BUILD)
endif (${CMAKE_SYSTEM_NAME} EQUAL "Linux")

add_library(
	mpt-common SHARED
        contrib/options.c
        config.c
        vmsl.c
        msgctxt.c
        statistics.c
        process_utils.c
	msgdata.c
)


install(TARGETS mpt-common
	LIBRARY DESTINATION ${CPP_LIBRARY_DIR}
)




