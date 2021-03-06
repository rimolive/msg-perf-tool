set(LITESTOMP_DIR "/usr" CACHE STRING "Litestomp directory")

find_path(LITESTOMP_INCLUDE_DIR litestomp-0/stomp_types.h
		HINTS ${LITESTOMP_DIR}/include)
find_library(LITESTOMP_LIB NAMES litestomp-0
		HINTS ${LITESTOMP_DIR}/lib ${LITESTOMP_DIR}/lib64)

message(STATUS "LiteSTOMP hint ${LITESTOMP_DIR}")
message(STATUS "LiteSTOMP headers found at ${LITESTOMP_INCLUDE_DIR}")
message(STATUS "LiteSTOMP library found at ${LITESTOMP_LIB}")

include_directories(${LITESTOMP_INCLUDE_DIR})

add_library(
	mpt-stomp SHARED
        stomp-context.c
        stomp-wrapper.c
)

target_link_libraries(
    mpt-stomp
		mpt-common
    ${LITESTOMP_LIB}
)

install(TARGETS mpt-stomp
	LIBRARY DESTINATION ${CPP_LIBRARY_DIR}
)
