set(DLL_NAME nampower)

include_directories(Include ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})

set(SOURCE_FILES
        types.h
        logging.hpp
        logging.cpp
        cdatastore.hpp
        cdatastore.cpp
        castqueue.h
        game.hpp
        game.cpp
        main.hpp
        main.cpp
        offsets.hpp
        helper.cpp
        helper.hpp
        spellcast.hpp
        spellcast.cpp
        spellchannel.hpp
        spellchannel.cpp
        spellevents.hpp
        spellevents.cpp
        scripts.hpp
        scripts.cpp
)

add_library(${DLL_NAME} SHARED ${SOURCE_FILES})
target_link_libraries(${DLL_NAME} shlwapi.lib asmjit.lib udis86.lib)

install(TARGETS ${DLL_NAME} RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}")
