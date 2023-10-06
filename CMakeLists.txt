file(READ version.txt POWER_METER_VERSION)
message("POWER_METER_VERSION : ${POWER_METER_VERSION}")

# sources
file(GLOB_RECURSE POWER_METER_SOURCES power_meter/*.*)
add_library(power_meter ${POWER_METER_SOURCES})

# include dirs
target_include_directories(power_meter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# defines
target_compile_definitions(power_meter PUBLIC -DPOWER_METER_VERSION="${POWER_METER_VERSION}")

# depends
target_link_libraries(power_meter etl)
target_link_libraries(power_meter periph)