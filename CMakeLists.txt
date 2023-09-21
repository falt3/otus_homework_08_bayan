cmake_minimum_required(VERSION 3.2)


set(PATCH_VERSION "0" CACHE INTERNAL "Patch version")
set(PROJECT_VESRION 0.0.${PATCH_VERSION})

set(myProject bayan)
project(${myProject} VERSION ${PROJECT_VESRION})

find_package(Boost COMPONENTS 
    program_options
    filesystem
    regex
    # unit_test_framework 
    REQUIRED)

set(nameApp bayan)
add_executable(${nameApp} main.cpp)
# add_executable(testIp test.cpp ip.cpp ip.h)

set_target_properties(
    ${nameApp} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
# target_include_directories(listIp PRIVATE 
#     "${CMAKE_BINARY_DIR}"
# )

# set_target_properties(
#     testIp PROPERTIES
#     COMPILE_DEFINITIONS BOOST_TEST_DYN_LINK
#     INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR}
# )

target_link_libraries(${nameApp} ${Boost_LIBRARIES})
# target_link_libraries(helloworld_cli PRIVATE
#     helloworld
# )
# target_link_libraries(test_version
#     ${Boost_LIBRARIES}
#     helloworld
# )

if (MSVC)
    target_compile_options(${nameApp} PRIVATE /W4)
    # target_compile_options(testIp PRIVATE /W4)
else ()
    target_compile_options(
        ${nameApp} PRIVATE
        -Wall -Wextra -pedantic -Werror
    )    
    # target_compile_options(
    #     testIp PRIVATE
    #     -Wall -Wextra -pedantic -Werror
    # )
endif()

# enable_testing()
# add_test(NAME testFilterIp COMMAND testIp)


install(TARGETS ${nameApp} RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT example@example.com)
include(CPack)
