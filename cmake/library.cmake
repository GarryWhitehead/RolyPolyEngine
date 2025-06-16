

function(rpe_add_compiler_flags)
    set(single_value_args TARGET)
    cmake_parse_arguments(
        COMPILE_FLAGS
        ""
        "${single_value_args}"
        ""
        "${ARGN}"
    )

    if(MSVC)
        # common flags for windows
        set(compile_options
            /bigobj 
            /EHsc
            #/Zc:preprocessor
            /std:c17
            /experimental:c11atomics 
            /NODEFAULTLIB:library
            /std:clatest
        )
    else()
        # common flags for gcc and clang
        set(compile_options
            -D_FORTIFY_SOURCE=2
            -D_GLIBCXX_ASSERTIONS
            -Wall
            -Wextra
            -Wno-unused-parameter
            -Wno-missing-braces
            -pipe         
            -Wreturn-type  
            -Werror=return-type    
            -Werror=format-security
            -fno-strict-aliasing
            -fno-common
            -fvisibility=hidden
            -fPIC
            -fno-omit-frame-pointer
        )

        # clang/gcc specific flags
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            list(APPEND compile_options -Wreturn-local-addr -Werror=return-local-addr)
        #elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        #    list(APPEND compile_options -Wreturn-stack-address  -Werror=return-stack-address)
        endif()

    endif()

    # build type specific compiler flags    
    if(MSVC)
        set(build_type_debug            ${compile_options} ${library_type_flag} /Zi /Ob0 /Od /MDd /Oy-) # Dr Memory docs suggest not to use: /RTC1
        set(build_type_relwithdebinfo   ${compile_options} ${library_type_flag} /O2 /Gy /Zi /MD)
        set(build_type_release          ${compile_options} ${library_type_flag} /O2 /MD)

    else()
        set(build_type_debug            ${compile_options} -g -fno-inline)
        set(build_type_relwithdebinfo   ${compile_options} -g -O2)
        set(build_type_release          ${compile_options} -O2)
    endif()
    
    # multi config build env or single build type?
    get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

    if(is_multi_config)
        target_compile_options(
            ${COMPILE_FLAGS_TARGET}
            PRIVATE 
            $<$<CONFIG:Debug>:          ${build_type_debug}>
            $<$<CONFIG:Release>:        ${build_type_release}>
            $<$<CONFIG:RelWithDebInfo>: ${build_type_relwithdebinfo}>
        )
    else()
        if(CMAKE_BUILD_TYPE)
            string(TOLOWER ${CMAKE_BUILD_TYPE} build_type_lowercase)
        endif()
        target_compile_options(
            ${COMPILE_FLAGS_TARGET}
            PRIVATE 
            ${build_type_${build_type_lowercase}}
        )
    endif()

    # intrinsics flags - basic setup for now.
    if (MSVC)
        set(intrinsic_flags ${intrinsic_flags} /arch:AVX2)
    else()
        set(intrinsic_flags ${intrinsic_flags} -march=haswell)
    endif()
    target_compile_options(
        ${COMPILE_FLAGS_TARGET} 
        PUBLIC 
        ${intrinsic_flags}
    )
    

endfunction()