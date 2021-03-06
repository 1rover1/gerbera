find_package(Threads REQUIRED)

add_executable(testsearchhandler
        $<TARGET_OBJECTS:libgerbera>
        main.cc
        test_searchhandler.cc
        )

include_directories(
        ${UPNP_INCLUDE_DIRS}
        ${UUID_INCLUDE_DIRS}
        ${MAGIC_INCLUDE_DIRS}
        ${ZLIB_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
        ${LASTFMLIB_INCLUDE_DIRS}
        ${FFMPEG_INCLUDE_DIR}
        ${EXIF_INCLUDE_DIRS}
        ${TAGLIB_INCLUDE_DIRS}
        ${EXPAT_INCLUDE_DIRS}
        ${FFMPEGTHUMBNAILER_INCLUDE_DIR}
        ${DUKTAPE_INCLUDE_DIRS}
        ${MYSQL_INCLUDE_DIRS}
        ${SQLITE3_INCLUDE_DIRS}
        ${ICONV_INCLUDE_DIR}
        ${GTEST_INCLUDE_DIRS}
)

target_link_libraries(testsearchhandler PRIVATE
        ${UUID_LIBRARIES}
        ${UPNP_LIBRARIES}
        ${MAGIC_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${CURL_LIBRARIES}
        ${LASTFMLIB_LIBRARIES}
        ${FFMPEG_LIBRARIES}
        ${EXIF_LIBRARIES}
        ${TAGLIB_LIBRARIES}
        ${EXPAT_LIBRARIES}
        ${FFMPEGTHUMBNAILER_LIBRARIES}
        ${DUKTAPE_LIBRARIES}
        ${MYSQL_CLIENT_LIBS}
        ${SQLITE3_LIBRARIES}
        ${ICONV_LIBRARIES}
        ${GTEST_LIBRARIES}
        ${GERBERA_INTERFACE_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
        )

add_test(NAME testsearchhandler
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND ./test/test_searchhandler/testsearchhandler)
