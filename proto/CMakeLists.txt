include_directories(${PROTOBUF_INCLUDE_DIR})
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER pluto.proto)
add_library(pluto_proto ${PROTO_HEADER} ${PROTO_SRC})


#todo: we can either link this to protobuf/${Protobuf_LIBRARIES} or link the final executable, both work. which makes more sense?
target_link_libraries(pluto_proto PUBLIC ${Protobuf_LIBRARIES})

target_include_directories(pluto_proto INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
