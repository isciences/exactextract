# Adapted from https://gitlab.kitware.com/cmake/cmake/blob/aec06dd4922187ce5346d20a9f0d53f01b6ce9fc/Source/CMakeVersionSource.cmake

# Try to identify the current development source version.
set(EXACTEXTRACT_VERSION_SOURCE "0.2.2")

if(EXISTS ${CMAKE_SOURCE_DIR}/.git/HEAD)
  find_program(GIT_EXECUTABLE NAMES git git.cmd)
  mark_as_advanced(GIT_EXECUTABLE)
  if(GIT_EXECUTABLE)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --verify -q --short=7 HEAD
      OUTPUT_VARIABLE head
      OUTPUT_STRIP_TRAILING_WHITESPACE
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      )
    if(head)
      execute_process(
        COMMAND ${GIT_EXECUTABLE} update-index -q --refresh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
      execute_process(
        COMMAND ${GIT_EXECUTABLE} diff-index --name-only HEAD --
        OUTPUT_VARIABLE dirty
        OUTPUT_STRIP_TRAILING_WHITESPACE
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
      if(dirty)
        set(EXACTEXTRACT_VERSION_SOURCE "${EXACTEXTRACT_VERSION_SOURCE} (${head}-dirty)")
      else()
        set(EXACTEXTRACT_VERSION_SOURCE "${EXACTEXTRACT_VERSION_SOURCE} (${head})")
      endif()
    endif()
  endif()
endif()

unset(dirty)
unset(head)
