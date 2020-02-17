project("mjc")
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.3)

set(CMAKE_CXX_STANDARD 17)

SET(CMAKE_PROJECT_NAME "mjc")
SET(EXECUTABLE_NAME "mjc")

find_package(BISON 3.0.4)
find_package(FLEX 2.6.1)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/src/minijava)

BISON_TARGET(Parser src/minijava/parser.y ${CMAKE_CURRENT_BINARY_DIR}/src/minijava/parser.cc)
FLEX_TARGET(Lexer src/minijava/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/src/minijava/lexer.cc)
ADD_FLEX_BISON_DEPENDENCY(Lexer Parser)

MESSAGE(STATUS "Dir: ${CMAKE_CURRENT_BINARY_DIR}")

include_directories(${CMAKE_CURRENT_BINARY_DIR}/src/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

SET(SOURCE_FILES src/main.cc
        src/minijava/exp.cc
        src/minijava/stm.cc
        src/minijava/error.cc
        src/minijava/parser_context.cc
        src/minijava/symbol.cc
        src/minijava/type.cc
        src/minijava/typecheck.cc
        src/intermediate/names.cc
        src/intermediate/tree.cc
        src/intermediate/tree_exp.cc
        src/intermediate/tree_stm.cc
        src/intermediate/canonizer.cc
        src/intermediate/tracer.cc
        src/backend/x86/x86_registers.cc
        src/backend/x86/x86_instr.cc
        src/backend/x86/x86_function.cc
        src/backend/x86/x86_target.cc
        src/backend/x86/x86_assem.cc
        )
include_directories("src")

SET_SOURCE_FILES_PROPERTIES(
  ${SOURCE_FILES}
  PROPERTIES
  COMPILE_FLAGS "-Wall"
)

add_executable(${EXECUTABLE_NAME}
  ${SOURCE_FILES}
  ${FLEX_Lexer_OUTPUTS}
  ${BISON_Parser_OUTPUTS}
)

target_link_libraries(${EXECUTABLE_NAME} stdc++fs)


enable_testing()

find_program (PYTHON python3)

if (PYTHON)
  # Compilation tests
  
  file(GLOB files "testcases/Small/*.java")
  foreach(file ${files})
    get_filename_component(name ${file} NAME_WE)
    add_test(NAME Small_${name}
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
             COMMAND ${PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/src/test/test_compilation.py 
                               $<TARGET_FILE:mjc>  
                               ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime.c
                               ${file})
  endforeach()          

  file(GLOB files "testcases/Medium/*.java")
  foreach(file ${files})
    get_filename_component(name ${file} NAME_WE)
    add_test(NAME Medium_${name}
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
             COMMAND ${PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/src/test/test_compilation.py 
                               $<TARGET_FILE:mjc>  
                               ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime.c
                               ${file})
  endforeach()          
  
  file(GLOB files "testcases/Large/*.java")
  foreach(file ${files})
    get_filename_component(name ${file} NAME_WE)
    add_test(NAME Large_${name}
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
             COMMAND ${PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/src/test/test_compilation.py 
                               $<TARGET_FILE:mjc>  
                               ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime.c
                               ${file})
  endforeach()          

  # Failure tests
  
  file(GLOB files "testcases/ShouldFail/ParseErrors/*.java" "testcases/ShouldFail/TypeErrors/*.java")
  foreach(file ${files})
    get_filename_component(name ${file} NAME_WE)
    add_test(NAME CompileError_${name}
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
             COMMAND ${PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/src/test/test_compile_error.py 
                               $<TARGET_FILE:mjc>  
                               ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime.c
                               ${file})
  endforeach()          

  
  file(GLOB files "testcases/ShouldFail/RuntimeErrors/*.java")
  foreach(file ${files})
    get_filename_component(name ${file} NAME_WE)
    add_test(NAME RuntimeError_${name}
             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
             COMMAND ${PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/src/test/test_runtime_error.py 
                               $<TARGET_FILE:mjc>  
                               ${CMAKE_CURRENT_SOURCE_DIR}/src/runtime.c
                               ${file})
  endforeach()          
endif (PYTHON)
