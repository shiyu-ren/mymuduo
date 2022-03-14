set(SRC_LIST HttpServer_test.cc ../HttpContext.cc ../HttpResponse.cc ../HttpServer.cc)

add_executable(httptest ${SRC_LIST})
target_link_libraries(httptest mymuduo pthread)

add_definitions(-std=c++17)