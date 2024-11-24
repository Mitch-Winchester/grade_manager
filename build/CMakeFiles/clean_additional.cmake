# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "third_party/QXlsx/CMakeFiles/QXlsx_autogen.dir/AutogenUsed.txt"
  "third_party/QXlsx/CMakeFiles/QXlsx_autogen.dir/ParseCache.txt"
  "third_party/QXlsx/QXlsx_autogen"
  )
endif()
