
set(HEADERS         logger.h
		    map.h
		    node.h
		    tinyxml2.h
		    algorithm.h
		    )

set(SOURCES         map.cpp
                    tinyxml2.cpp
                    decomposition.cpp
                    moves.cpp
                    algorithm.cpp
                    main.cpp
		    )

add_executable(PushAndRotate ${HEADERS} ${SOURCES} main.cpp)

add_library(PPLIB ${HEADERS} ${SOURCES})

install(TARGETS PushAndRotate 
        CONFIGURATIONS Debug
        DESTINATION Bin)

install(TARGETS PushAndRotate 
        CONFIGURATIONS Release
        DESTINATION Bin)