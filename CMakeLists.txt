cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ble_fsm_demo)
target_sources(app PRIVATE
  			src/main.c
  			led_strip_src/led_strip.c
			motor_src/motor.c
)
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
