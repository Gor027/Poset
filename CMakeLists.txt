cmake_minimum_required(VERSION 3.15)
project(Poset LANGUAGES C CXX)

function(make_target name file)
    add_executable(${name})
    target_sources(${name}
        PRIVATE
        src/poset.cc
        src/${file}
        PUBLIC
        src/poset.h)
    set_property(TARGET ${name} PROPERTY CXX_STANDARD 17)
    target_compile_options(${name}
        PRIVATE
        -Wall -Wextra)
endfunction()

make_target(example1 poset_example1.c)
make_target(example2 poset_example2.cc)
make_target(transitive transitive.cc)