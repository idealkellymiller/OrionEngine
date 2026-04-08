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