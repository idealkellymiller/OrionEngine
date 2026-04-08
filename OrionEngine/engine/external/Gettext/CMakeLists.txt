# add_library(intl STATIC IMPORTED)
#     set_target_properties(intl PROPERTIES
#         IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/intl.lib"
#     )

#     target_include_directories(intl INTERFACE
#         "engine/external/Gettext/include"
#     )

# add_library(libiconv STATIC IMPORTED)
#     set_target_properties(libiconv PROPERTIES
#         IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/libiconv.lib"
#     )

#     target_include_directories(intl INTERFACE
#         "engine/external/Gettext/include"
#     )

# set_property(TARGET intl APPEND PROPERTY INTERFACE_LINK_LIBRARIES libiconv)


# Create an interface target to bundle everything together
add_library(gettext_intl INTERFACE)
    add_library(intl STATIC IMPORTED GLOBAL)
        set_target_properties(intl PROPERTIES
            IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/intl.lib"
        )

    add_library(libiconv STATIC IMPORTED GLOBAL)
        set_target_properties(libiconv PROPERTIES
            IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/lib/iconv.lib"
        )

target_link_libraries(gettext_intl INTERFACE intl libiconv)

target_include_directories(gettext_intl INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")