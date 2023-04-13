# gnome = import('gnome')

# gresources = gnome.compile_resources(
#   'gresource-multistream-server',
#   'multistream-server.gresources.xml'
# )


# Step 1:
# find_program(GLIB_COMPILE_RESOURCES NAMES glib-compile-resources REQUIRED)

# set(GRESOURCE_C   gresource-multistream-server.h)

# set(GRESOURCE_XML multistream-server.gresources.xml)

# # Step 2:
# add_custom_command(
#     OUTPUT ${GRESOURCE_C}
#     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
#     COMMAND ${GLIB_COMPILE_RESOURCES}
#     ARGS
#         --target=${CMAKE_CURRENT_BINARY_DIR}/${GRESOURCE_C}
#         ${GRESOURCE_XML}
#     VERBATIM
#     MAIN_DEPENDENCY ${GRESOURCE_XML}
#     # DEPENDS
#     #     for.glade
#     #     bar.glade
# )

# # Step 3:
# add_custom_target(
#     gresource-multistream-server
#     DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${GRESOURCE_C}
# )

# # Step 4:
# add_executable(${PROJECT_NAME} dummy.c ${CMAKE_CURRENT_BINARY_DIR}/${GRESOURCE_C})
# set_source_files_properties(
#     ${CMAKE_CURRENT_BINARY_DIR}/${GRESOURCE_C}
#     PROPERTIES GENERATED TRUE
# )

# # Step 5:
# add_dependencies(${PROJECT_NAME} gresource-multistream-server)


# glib-compile-resources --target=gresource-multistream-server.c --generate-source multistream-server.gresources.xml
# glib-compile-resources --target=gresource-multistream-server.h --generate-header multistream-server.gresources.xml


add_library(gresource-multistream-server STATIC gresource-multistream-server.c gresource-multistream-server.h)

# Throw the kitchen sink at it
target_link_libraries(gresource-multistream-server PRIVATE aux_util aux_gstreamer ${GST_LIBRARIES} ${GST_SDP_LIBRARIES} ${GST_WEBRTC_LIBRARIES} ${GLIB_LIBRARIES} ${LIBSOUP_LIBRARIES} ${JSONGLIB_LIBRARIES} ${GIO_LIBRARIES})

target_include_directories(gresource-multistream-server PRIVATE ${GLIB_INCLUDE_DIRS} ${GST_INCLUDE_DIRS} ${LIBSOUP_INCLUDE_DIRS} ${JSONGLIB_INCLUDE_DIRS} ${GIO_INCLUDE_DIRS})


target_include_directories(gresource-multistream-server INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

