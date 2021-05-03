/*
	liblightmodbus - a lightweight, multiplatform Modbus library
	Copyright (C) 2017 Jacek Wieczorek <mrjjot@gmail.com>

	This file is part of liblightmodbus.

	Liblightmodbus is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Liblightmodbus is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
	\file
	\brief General Modbus master functions

	\note This header file is suitable for C++
*/

#ifndef LIGHTMODBUS_MASTER_H
#define LIGHTMODBUS_MASTER_H

// For C++
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "parser.h"
#include "libconf.h"
#include "lightmodbus.h"

#ifdef LIGHTMODBUS_MASTER_BASE

#if defined( LIGHTMODBUS_MASTER_INVASIVE_PARSING ) && !defined( LIGHTMODBUS_NO_MASTER_DATA_BUFFER )
	#warning LIGHTMODBUS_MASTER_INVASIVE_PARSING has no effect if LIGHTMODBUS_NO_MASTER_DATA_BUFFER is not defined
#endif

#if defined( LIGHTMODBUS_NO_MASTER_DATA_BUFFER ) && !defined( LIGHTMODBUS_EXPERIMENTAL )
	#error Disabling exclusive master data buffer is an experimental feature that may cause problems. Please define LIGHTMODBUS_EXPERIMENTAL to dismiss this error message, but please make sure your system permits unaligned memory acces beforehand.
#endif

#if defined( LIGHTMODBUS_NO_MASTER_DATA_BUFFER ) && defined( LIGHTMODBUS_STATIC_MEM_MASTER_DATA )
	#error LIGHTMODBUS_STATIC_MEM_MASTER_DATA and LIGHTMODBUS_NO_MASTER_DATA_BUFFER cannot be used at once. Please make up your mind.
#endif

#ifdef LIGHTMODBUS_MASTER_USER_FUNCTIONS
	struct modbusMaster;

	/**
		\brief Associates user-defined response parser function with the function ID
	*/
	typedef struct modbusMasterUserFunction
	{
		uint8_t function; //!< The function code
		ModbusError ( *handler )( struct modbusMaster *status, ModbusParser *parser, ModbusParser *requestParser ); //!< Pointer to the user defined parsing function
	} ModbusMasterUserFunction;
#endif

/**
	\brief Represents Modbus master device's status and configuration
*/
typedef struct modbusMaster
{
	//! Predicted number of response bytes to be received from slave upon succesful request.
	uint8_t predictedResponseLength;

	/**
		\brief Struct containing master's request for the slave

		\note Declaration of the `frame` member depends on the library configuration.
		It can be either a statically allocated array or a pointer to dynamically allocated memory.
		The behavior is dependant on definition of the `LIGHTMODBUS_STATIC_MEM_MASTER_REQUEST` macro

		\see \ref static-mem
	*/
	struct
	{
		#ifdef LIGHTMODBUS_STATIC_MEM_MASTER_REQUEST
			//! Statically allocated memory for the request frame
			uint8_t frame[LIGHTMODBUS_STATIC_MEM_MASTER_REQUEST];
		#else
			//! A pointer to dynamically allocated memory for the request frame
			uint8_t *frame;
		#endif

		//! Length of the request frame in bytes
		uint8_t length;
	} request;

	/**
		\brief Struct containing slave's response to the master's request

		\note Declaration of the `frame` member depends on the library configuration.
		It can be either a statically allocated array or a pointer to dynamically allocated memory.
		The behavior is dependant on definition of the `LIGHTMODBUS_STATIC_MEM_MASTER_RESPONSE` macro

		\see \ref static-mem
	*/
	struct //Response from slave should be put here
	{
		#ifdef LIGHTMODBUS_STATIC_MEM_MASTER_RESPONSE
			//! Statically allocated memory for the response frame
			uint8_t frame[LIGHTMODBUS_STATIC_MEM_MASTER_RESPONSE];
		#else
			//! A pointer to dynamically allocated memory for the response frame
			const uint8_t *frame;
		#endif

		//! Length of the response frame in bytes
		uint8_t length;
	} response;

	/**
		\brief Contains data received from the slave

		The space for data inside this structure can either be dynamically or
		statically allocated (see \ref static-mem).
	*/
	struct
	{
		uint8_t address; //!< Addres of slave that sent in the data
		uint16_t index; //!< Modbus address of the first register/coil
		uint16_t count; //!< Number of data units (coils, registers, etc.)
		uint8_t length; //!< Length of data in bytes
		ModbusDataType type; //!< Type of data
		uint8_t function; //!< Function that accessed the data

		#ifdef LIGHTMODBUS_STATIC_MEM_MASTER_DATA
			union
			{
				/**
					\brief Statically allocated array for received coils data.

					Each bit of this array corresponds to one coil value.
					\see modbusMaskRead()

				*/
				uint8_t coils[LIGHTMODBUS_STATIC_MEM_MASTER_DATA];


				/**
					\brief Statically allocated array for received registers data

					Data in this array always has currently used platform's native endianness.
				*/
				uint16_t regs[LIGHTMODBUS_STATIC_MEM_MASTER_DATA >> 1];
			};
		#else
			//Two separate pointers are used in case pointer size differed between types (possible on some weird architectures)

			/**
				\brief A pointer to dynamically allocated memory for the received coils data

				Each bit of this array corresponds to one coil value.
				\see modbusMaskRead()
			*/
			uint8_t *coils;

			/**
				\brief A pointer to dynamically allocated memory for the received registers data

				Data in this array always has currently used platform's native endianness.
			*/
			uint16_t *regs;
		#endif
	} data;

	/**
		\brief Contains exception data received from the slave

		\note Data in this structure is only valid if the processed
		response frame was an exception frame.
	*/
	struct
	{
		uint8_t address; //!< Slave device address
		uint8_t function; //!< Function that has thrown the exception
		ModbusExceptionCode code; //!< Exception code
	} exception;

	//! More precise information according encountered frame parsing error
	ModbusFrameError parseError;

	//! More precise information according encountered frame building error
	ModbusFrameError buildError;

	//User defined functions
	#ifdef LIGHTMODBUS_MASTER_USER_FUNCTIONS
		/**
			\brief A pointer to an array of user-defined Modbus functions
			\see user-functions
		*/
		ModbusMasterUserFunction *userFunctions;
		uint16_t userFunctionCount; //!< Number of the user functions in the array
	#endif

} ModbusMaster;
#endif

#include "master/mbregs.h"
#include "master/mbcoils.h"

#ifdef LIGHTMODBUS_MASTER_BASE

/**
	\brief Interprets the incoming response frame located in the master structure

	Calling this function results in processing the frame located in \ref ModbusMaster::response.
	The received data will be located in \ref ModbusMaster::data or \ref ModbusMaster::exception
	if slave has responded with an exception frame.

	Firstly, this function resets and freed data previously stored in
	\ref ModbusMaster::exception and \ref ModbusMaster::data.

	Afterwards, the frame is validated using \ref modbusCRC() function.
	If it's not consistent, it's discarded at this point and
	\ref MODBUS_ERROR_PARSE is returned.

	Then, array of user-defined functions is searched. It is important
	to mention that, user functions override built-in functions. That means,
	one can disable specific function by simply inserting NULL pointer
	into the array,

	If matching function is found among user-defined or built-in functions,
	the modbusParseResponse() returns the exact error code the parsing
	function had returned. Otherwise, \ref MODBUS_ERROR_PARSE is returned.

	\param status The master structure to work on
	\return A \ref ModbusError error code
*/
extern ModbusError modbusParseResponse( ModbusMaster *status );

/**
	\brief Performs initialization of the \ref ModbusMaster structure
	\param status The master structure to be initialized
	\return A \ref ModbusError error code
*/
extern ModbusError modbusMasterInit( ModbusMaster *status );

/**
	\brief Frees memory used by the \ref ModbusMaster structure, previously initialized with \ref modbusMasterInit
	\param status The master structure to be freed
	\return A \ref ModbusError error code
*/
extern ModbusError modbusMasterEnd( ModbusMaster *status );
#endif

// For C++ (closes `extern "C"` )
#ifdef __cplusplus
}
#endif

#endif
