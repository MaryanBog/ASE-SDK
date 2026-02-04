cmake_minimum_required(VERSION 3.16)
project(ASE_SDK LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ASE_BUILD_EXAMPLES "Build ASE examples" ON)
option(ASE_BUILD_TESTS    "Build ASE tests" ON)
option(ASE_BUILD_INTERNAL "Build internal (non-shipping) simulations" OFF)

add_library(ase INTERFACE)
target_include_directories(ase INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Warnings for everything we compile here (examples + tests + internal)
set(ASE_WARNINGS -Wall -Wextra -Wpedantic)

# ----------------------------
# Examples (public)
# ----------------------------
if (ASE_BUILD_EXAMPLES)
  add_executable(scalar_demo examples/scalar_demo.cpp)
  target_link_libraries(scalar_demo PRIVATE ase)
  target_compile_options(scalar_demo PRIVATE ${ASE_WARNINGS})

  add_executable(project_demo examples/project_demo.cpp)
  target_link_libraries(project_demo PRIVATE ase)
  target_compile_options(project_demo PRIVATE ${ASE_WARNINGS})

  add_executable(host_loop_demo examples/host_loop_demo.cpp)
  target_link_libraries(host_loop_demo PRIVATE ase)
  target_compile_options(host_loop_demo PRIVATE ${ASE_WARNINGS})
endif()

# ----------------------------
# Internal simulations (non-shipping)
# ----------------------------
if (ASE_BUILD_INTERNAL)
  add_executable(learning_loop_demo internal/learning_loop_demo.cpp)
  target_link_libraries(learning_loop_demo PRIVATE ase)
  target_compile_options(learning_loop_demo PRIVATE ${ASE_WARNINGS})

  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/internal/learning_loop_state_demo.cpp)
  add_executable(learning_loop_state_demo internal/learning_loop_state_demo.cpp)
  target_link_libraries(learning_loop_state_demo PRIVATE ase)
  target_compile_options(learning_loop_state_demo PRIVATE ${ASE_WARNINGS})
  endif()

  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/internal/learning_loop_adam_state_demo.cpp)
  add_executable(learning_loop_adam_state_demo internal/learning_loop_adam_state_demo.cpp)
  target_link_libraries(learning_loop_adam_state_demo PRIVATE ase)
  target_compile_options(learning_loop_adam_state_demo PRIVATE ${ASE_WARNINGS})
  endif()

endif()

# ----------------------------
# Tests
# ----------------------------
if (ASE_BUILD_TESTS)
  enable_testing()

  add_executable(test_core tests/test_core.cpp)
  target_link_libraries(test_core PRIVATE ase)
  target_compile_options(test_core PRIVATE ${ASE_WARNINGS} -Werror)
  add_test(NAME ASE_CoreTests COMMAND test_core)

  add_executable(test_integration tests/test_integration.cpp)
  target_link_libraries(test_integration PRIVATE ase)
  target_compile_options(test_integration PRIVATE ${ASE_WARNINGS} -Werror)
  add_test(NAME ASE_IntegrationTests COMMAND test_integration)

  # Optional: learning-loop envelope test (only if file exists)
  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_learning_envelope.cpp)
    add_executable(test_learning_envelope tests/test_learning_envelope.cpp)
    target_link_libraries(test_learning_envelope PRIVATE ase)
    target_compile_options(test_learning_envelope PRIVATE ${ASE_WARNINGS} -Werror)
    add_test(NAME ASE_LearningEnvelopeTests COMMAND test_learning_envelope)
  endif()

  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_learning_state_envelope.cpp)
  add_executable(test_learning_state_envelope tests/test_learning_state_envelope.cpp)
  target_link_libraries(test_learning_state_envelope PRIVATE ase)
  target_compile_options(test_learning_state_envelope PRIVATE ${ASE_WARNINGS} -Werror)
  add_test(NAME ASE_LearningStateEnvelopeTests COMMAND test_learning_state_envelope)
  endif()

  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_learning_adam_state_envelope.cpp)
  add_executable(test_learning_adam_state_envelope tests/test_learning_adam_state_envelope.cpp)
  target_link_libraries(test_learning_adam_state_envelope PRIVATE ase)
  target_compile_options(test_learning_adam_state_envelope PRIVATE ${ASE_WARNINGS} -Werror)
  add_test(NAME ASE_LearningAdamStateEnvelopeTests COMMAND test_learning_adam_state_envelope)
  endif()

endif()
