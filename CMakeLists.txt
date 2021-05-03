#List of all slave modules
set( SLAVE_MODULES
	"F01S"
	"F02S"
	"F03S"
	"F04S"
	"F05S"
	"F06S"
	"F15S"
	"F16S"
	"F22S"
	"SLAVE_USER_FUNCTIONS"
	"REGISTER_CALLBACK"
	"COIL_CALLBACK"
)

#List of all master modules
set( MASTER_MODULES
	"F01M"
	"F02M"
	"F03M"
	"F04M"
	"F05M"
	"F06M"
	"F15M"
	"F16M"
	"F22M"
	"MASTER_USER_FUNCTIONS"
	"NO_MASTER_DATA_BUFFER"
	"MASTER_INVASIVE_PARSING"
)

#List of all modules that can be enabled
set( AVAILABLE_MODULES
	"SLAVE_BASE"
	"${SLAVE_MODULES}"
	"MASTER_BASE"
	"${MASTER_MODULES}"

	#Misc
	"EXPERIMENTAL"
)

#Get git version (LIGHTMODBUS_GIT_VERSION)
execute_process( COMMAND "git" "describe" "--abbrev=6" "--dirty" "--always" "--tag" RESULT_VARIABLE GIT_EXIT_CODE OUTPUT_VARIABLE PROJECT_GIT_VERSION )
if ( GIT_EXIT_CODE )
    set( PROJECT_GIT_VERSION "no-vcs-found" )
endif( )
string( REGEX REPLACE "\n$" "" PROJECT_GIT_VERSION "${PROJECT_GIT_VERSION}" ) #Strip newline

#Esp-idf specific configuration
if (ESP_PLATFORM) 
    # ESP32 is a little endian chip
    set(LIBCONF "\n#include \"sdkconfig.h\"\n\n")
    set(LIBCONF "${LIBCONF}#define LIGHTMODBUS_LITTLE_ENDIAN\n")

    foreach(MODULE ${AVAILABLE_MODULES})
        set(LIBCONF "${LIBCONF}#ifdef CONFIG_LIGHTMODBUS_${MODULE}\n#define LIGHTMODBUS_${MODULE}\n#endif\n")
    endforeach()

    # static memory allocation buffers
    set(LIBCONF "${LIBCONF}#ifdef CONFIG_LIGHTMODBUS_MEM_SLAVE_REQUEST_ENABLED\n#define LIGHTMODBUS_MEM_SLAVE_REQUEST CONFIG_LIGHTMODBUS_MEM_SLAVE_REQUEST\n#endif\n")
    set(LIBCONF "${LIBCONF}#ifdef CONFIG_LIGHTMODBUS_MEM_SLAVE_RESPONSE_ENABLED\n#define LIGHTMODBUS_MEM_SLAVE_RESPONSE CONFIG_LIGHTMODBUS_MEM_SLAVE_RESPONSE\n#endif\n")
    set(LIBCONF "${LIBCONF}#ifdef CONFIG_LIGHTMODBUS_MEM_MASTER_REQUEST_ENABLED\n#define LIGHTMODBUS_MEM_MASTER_REQUEST CONFIG_LIGHTMODBUS_MEM_MASTER_REQUEST\n#endif\n")
    set(LIBCONF "${LIBCONF}#ifdef CONFIG_LIGHTMODBUS_MEM_MASTER_RESPONSE_ENABLED\n#define LIGHTMODBUS_MEM_MASTER_RESPONSE CONFIG_LIGHTMODBUS_MEM_MASTER_RESPONSE\n#endif\n")

    file(MAKE_DIRECTORY ${BUILD_DIR}/lightmodbus)
    file(MAKE_DIRECTORY ${BUILD_DIR}/lightmodbus/slave)
    file(MAKE_DIRECTORY ${BUILD_DIR}/lightmodbus/master)

    #Emit configuration file
    configure_file(
        "${COMPONENT_DIR}/include/lightmodbus/libconf.h.in"
        "${BUILD_DIR}/lightmodbus/libconf.h"
    )

    idf_component_register(
        SRC_DIRS src src/slave src/master
        INCLUDE_DIRS include . ${BUILD_DIR}/lightmodbus ${BUILD_DIR}/lightmodbus/master ${BUILD_DIR}/lightmodbus/slave
    )

    return()
endif()

#Basic stuff
cmake_minimum_required( VERSION 3.3 )
project( liblightmodbus
	VERSION 2.0
	)
include( TestBigEndian )

#Debug/release options
if ( NOT DEFINED CMAKE_C_FLAGS)
	set( CMAKE_C_FLAGS "-Wall" )
	set( CMAKE_C_FLAGS_DEBUG "-g -O0 -fno-builtin" )
	set( CMAKE_C_FLAGS_RELEASE "-Os" )
endif()

# Clang generates some warnings when building with static memory allocation
# These warnings are obviously correct, but they are also harmless
if ( NOT DEFINED AVR AND "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang" )
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-tautological-pointer-compare -Wno-tautological-constant-out-of-range-compare")
endif( )

#For coverage testing
if ( DEFINED COVERAGE_TEST AND NOT DEFINED AVR )
	message( STATUS "Build for coverage testing" )
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG} -Wall --coverage" )
endif( )

#User defined compiler and linker
if ( DEFINED CC )
	set( CMAKE_C_COMPILER ${CC} )
endif( )
if( DEFINED LD )
	set( CMAKE_LINKER ${LD} )
endif( )

#AVR build options
if ( DEFINED AVR )
	message( STATUS "Building for AVR" )

	#MCU has to be set
	if ( "${AVR}" STREQUAL "" )
		message( FATAL_ERROR "Please specify AVR MCU type!" )
	endif( )

	#Set MCU
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mmcu=${AVR}" )

	#Default AVR compiler
	if ( NOT DEFINED CC )
		message( STATUS "Using default AVR compiler - avr-gcc")
		set( CMAKE_C_COMPILER "avr-gcc" ) #Default AVR compiler
	endif( )

	#Default AVR linker
	if ( NOT DEFINED LD )
		message( STATUS "Using default AVR linker - avr-ld")
		set( CMAKE_LINKER "avr-ld" ) #Default AVR linker
	endif( )

	#AVRs are little endian
	set( ENDIANNESS "little" )
endif( )

#Include dirs
include_directories( "${PROJECT_SOURCE_DIR}/include" )

#List of modules enabled by default
set( DEFAULT_MODULES
	#Slave
	"SLAVE_BASE"
	"F01S" "F02S" "F03S" "F04S" "F05S" "F06S" "F15S" "F16S" "F22S"
	"SLAVE_USER_FUNCTIONS"

	#Master
	"MASTER_BASE"
	"F01M" "F02M" "F03M" "F04M" "F05M" "F06M" "F15M" "F16M" "F22M"
	"MASTER_USER_FUNCTIONS"
)

#Macros to be written to libconf.h
set( LIBCONF "" )

#If module set is not specified, include all known modules
if ( NOT DEFINED MODULES )
	set( MODULES ${DEFAULT_MODULES} )
	message( STATUS "MODULES not set. Enabling deafults.")
endif( )

#Add modules deined in ADD_MODULES
if ( DEFINED ADD_MODULES )
	set( MODULES "${MODULES};${ADD_MODULES}" )
endif( )

#Iterate through modules list
foreach( MODULE ${MODULES} )
	#If that module doesn't exit raise an error
	if ( NOT ${MODULE} IN_LIST AVAILABLE_MODULES )
		message( FATAL_ERROR "`${MODULE}' is not a valid module name. Available modules are: `${AVAILABLE_MODULES}'")
	endif( )

	#If module needs base, but base is not included
	if ( ${MODULE} IN_LIST SLAVE_MODULES AND NOT "SLAVE_BASE" IN_LIST MODULES )
		message( FATAL_ERROR "You seem to be needing ${MODULE} module, but it requires slave base module to be included. Please add SLAVE_BASE to your MODULES list" )
	endif( )
	if ( ${MODULE} IN_LIST MASTER_MODULES AND NOT "MASTER_BASE" IN_LIST MODULES )
		message( FATAL_ERROR "You seem to be needing ${MODULE} module, but it requires master base module to be included. Please add MASTER_BASE to your MODULES list" )
	endif( )

	#Generate proper #define directive
	set( LIBCONF "${LIBCONF}#define LIGHTMODBUS_${MODULE}\n" )
	message( STATUS "Enabling LIGHTMODBUS_${MODULE}" )
endforeach( )

#Fixed size buffers
if ( STATIC_MEM_SLAVE_REQUEST )
	if ( STATIC_MEM_SLAVE_REQUEST LESS 2 )
		message( FATAL_ERROR "STATIC_MEM_SLAVE_REQUEST has to be greater than 1" )
	endif( )
	message( STATUS "Enabling static slave request buffer of size ${STATIC_MEM_SLAVE_REQUEST} bytes" )
	set( LIBCONF "${LIBCONF}#define LIGHTMODBUS_STATIC_MEM_SLAVE_REQUEST ${STATIC_MEM_SLAVE_REQUEST}\n" )
endif( )
if ( STATIC_MEM_SLAVE_RESPONSE )
	if ( STATIC_MEM_SLAVE_RESPONSE LESS 2 )
		message( FATAL_ERROR "STATIC_MEM_SLAVE_RESPONSE has to be greater than 1" )
	endif( )
	message( STATUS "Enabling static slave response buffer of size ${STATIC_MEM_SLAVE_RESPONSE} bytes" )
	set( LIBCONF "${LIBCONF}#define LIGHTMODBUS_STATIC_MEM_SLAVE_RESPONSE ${STATIC_MEM_SLAVE_RESPONSE}\n" )
endif( )
if ( STATIC_MEM_MASTER_REQUEST )
	if ( STATIC_MEM_MASTER_REQUEST LESS 2 )
		message( FATAL_ERROR "STATIC_MEM_MASTER_REQUEST has to be greater than 1" )
	endif( )
	message( STATUS "Enabling static master request buffer of size ${STATIC_MEM_MASTER_REQUEST} bytes" )
	set( LIBCONF "${LIBCONF}#define LIGHTMODBUS_STATIC_MEM_MASTER_REQUEST ${STATIC_MEM_MASTER_REQUEST}\n" )
endif( )
if ( STATIC_MEM_MASTER_RESPONSE )
	if ( STATIC_MEM_MASTER_RESPONSE LESS 2 )
		message( FATAL_ERROR "STATIC_MEM_MASTER_RESPONSE has to be greater than 1" )
	endif( )
	message( STATUS "Enabling static master response buffer of size ${STATIC_MEM_MASTER_RESPONSE} bytes" )
	set( LIBCONF "${LIBCONF}#define LIGHTMODBUS_STATIC_MEM_MASTER_RESPONSE ${STATIC_MEM_MASTER_RESPONSE}\n" )
endif( )
if ( STATIC_MEM_MASTER_DATA )
	if ( STATIC_MEM_MASTER_DATA LESS 2 )
		message( FATAL_ERROR "STATIC_MEM_MASTER_DATA has to be greater than 1" )
	endif( )
	message( STATUS "Enabling static master data buffer of size ${STATIC_MEM_MASTER_DATA} bytes" )
	set( LIBCONF "${LIBCONF}#define LIGHTMODBUS_STATIC_MEM_MASTER_DATA ${STATIC_MEM_MASTER_DATA}\n" )
endif( )

#Detect endianness if not defined
if ( NOT DEFINED ENDIANNESS )
	message( STATUS "ENDIANNESS not defined - will be automatically adjusted for this system" )
	TEST_BIG_ENDIAN( IS_BIG_ENDIAN )
	if ( IS_BIG_ENDIAN )
		set( ENDIANNESS "big" )
	else( )
		set( ENDIANNESS "little" )
	endif( )
endif( )

#Write endianness to libconf.h
string( TOLOWER "${ENDIANNESS}" ENDIANNESS )
if ( "${ENDIANNESS}" STREQUAL "big" )
	message( STATUS "Using big-endian configuration" )
	set( LIBCONF "${LIBCONF}#define LIGHTMODBUS_BIG_ENDIAN\n" )
else( )
	if ( "${ENDIANNESS}" STREQUAL "little" )
		message( STATUS "Using little-endian configuration" )
		set( LIBCONF "${LIBCONF}#define LIGHTMODBUS_LITTLE_ENDIAN\n" )
	else( )
		message( FATAL_ERROR "Bad ENDIANNESS value. Use 'big' or 'little'." )
	endif( )
endif( )

#Emit configuration file
configure_file(
	"${PROJECT_SOURCE_DIR}/include/lightmodbus/libconf.h.in"
	"${PROJECT_SOURCE_DIR}/include/lightmodbus/libconf.h"
	)

#Library sources list
add_library(
	lightmodbus
	"${PROJECT_SOURCE_DIR}/src/lightmodbus.c"
	"${PROJECT_SOURCE_DIR}/src/slave.c"
	"${PROJECT_SOURCE_DIR}/src/master.c"
	"${PROJECT_SOURCE_DIR}/src/slave/scoils.c"
	"${PROJECT_SOURCE_DIR}/src/slave/sregs.c"
	"${PROJECT_SOURCE_DIR}/src/master/mpcoils.c"
	"${PROJECT_SOURCE_DIR}/src/master/mbcoils.c"
	"${PROJECT_SOURCE_DIR}/src/master/mpregs.c"
	"${PROJECT_SOURCE_DIR}/src/master/mbregs.c"
	)

#Set library version
set_target_properties( lightmodbus PROPERTIES VERSION ${PROJECT_VERSION} )

#Coverage test
if ( DEFINED COVERAGE_TEST AND NOT DEFINED AVR )
	add_executable(
		coverage-test
		"test/test.c"
		"src/addons/examine.c"
	)
	target_link_libraries(
		coverage-test
		lightmodbus
	)
endif( )

#Run avr-size after build
if ( DEFINED AVR )
	add_custom_command(
		TARGET lightmodbus POST_BUILD
		COMMAND "avr-size" "-C" "--mcu=${AVR}" "${CMAKE_BINARY_DIR}/liblightmodbus.a"
	)
endif( )

include(GNUInstallDirs)
#Installation
install(
	TARGETS lightmodbus
	ARCHIVE DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/"
)

#Install headers
install(
	DIRECTORY "${CMAKE_SOURCE_DIR}/include/" DESTINATION "${CMAKE_INSTALL_PREFIX}/include/" FILES_MATCHING PATTERN "*.h"
)
