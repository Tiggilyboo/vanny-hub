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
	\brief Core Modbus functions

	This is main header file that is ought to be included as library
	\note This header file is suitable for C++
*/

#ifndef LIGHTMODBUS_H
#define LIGHTMODBUS_H

// For C++
#ifdef __cplusplus
extern "C" {
#endif

//Include proper header files
#include <stdint.h>
#include "libconf.h"

//Some protection
#if defined(LIGHTMODBUS_BIG_ENDIAN) && defined(LIGHTMODBUS_LITTLE_ENDIAN)
#error LIGHTMODBUS_BIG_ENDIAN and LIGHTMODBUS_LITTLE_ENDIAN cannot be used at once!
#endif

/**
	\brief Represents a library runtime error code.
*/
typedef enum modbusError
{
	MODBUS_ERROR_OK = 0, //!< No error
	/**
		\brief Indicates that slave had thrown an exception.

		This exception can be thrown either by master's parsing function
		(indicating incoming exception frame) or by slave's building function
		(indicating that some problem caused the slave to **build an exception frame**).

		\note This error code handles the superset of problems handled by \ref MODBUS_ERROR_PARSE.

		When thrown on slave side, check \ref ModbusSlave.lastException and \ref ModbusSlave.parseError
		for more information.
	*/
	MODBUS_ERROR_EXCEPTION = 1,
	/**
		\brief Memory problem

		Either one of memory allocation functions returned NULL or
		fixed-size buffer is not big enough to fit the data (see \ref static-mem).
	*/
	MODBUS_ERROR_ALLOC, //!< Memory allocation problem
	MODBUS_ERROR_OTHER, //!< Other reason causing the function to abort (eg. bad function parameter)
	MODBUS_ERROR_NULLPTR, //!< A NULL pointer provided as some crucial parameter
	/**
		Parsing error occurred - check \ref ModbusSlave.parseError

		\note This error code is returned instead of \ref MODBUS_ERROR_EXCEPTION
		when exception should have been thrown, but wasn't (eg. due to broadcasted
		request frame). These two error code should be treated similarly.
	*/
	MODBUS_ERROR_PARSE,
	MODBUS_ERROR_BUILD, //!< Frame building error occurred - check \ref ModbusMaster.buildError
	MODBUS_OK = MODBUS_ERROR_OK, //!< No error. Alias of \ref MODBUS_ERROR_OK
} ModbusError;

/**
	\brief Provides more information on frame building/parsing error.

	These error code should serve as an additional source of information for the user.
*/
typedef enum modbusFrameError
{
	MODBUS_FERROR_OK = MODBUS_OK, //!< Modbus frame OK. No error.
	MODBUS_FERROR_CRC, //!< Invalid CRC
	MODBUS_FERROR_LENGTH, //!< Invalid frame length
	MODBUS_FERROR_COUNT, //!< Invalid declared data item count
	MODBUS_FERROR_VALUE, //!< Illegal data value (eg. when writing a single coil)
	MODBUS_FERROR_RANGE, //!< Invalid register range
	MODBUS_FERROR_NOSRC, //!< There's neither callback function nor value array provided for this data type
	MODBUS_FERROR_NOREAD, //!< No read access to at least one of requested regsiters
	MODBUS_FERROR_NOWRITE, //!< No write access to one of requested regsiters
	MODBUS_FERROR_NOFUN, //!< Function not supported
	MODBUS_FERROR_BADFUN, //!< Requested a parsing function to parse a frame with wrong function code
	MODBUS_FERROR_NULLFUN, //!< Function overriden by user with NULL pointer.
	MODBUS_FERROR_MISM_FUN, //!< Function request-response mismatch
	MODBUS_FERROR_MISM_ADDR, //!< Slave address request-response mismatch
	MODBUS_FERROR_MISM_INDEX, //!< Index value request-response mismatch
	MODBUS_FERROR_MISM_COUNT, //!< Count value request-response mismatch
	MODBUS_FERROR_MISM_VALUE, //!< Data value request-response mismatch
	MODBUS_FERROR_MISM_MASK, //!< Mask value request-response mismatch
	MODBUS_FERROR_BROADCAST //!< Received response for broadcast message

} ModbusFrameError;

/**
	\brief Represents a Modbus exception code, defined by the standart.
*/
typedef enum modbusExceptionCode
{
	MODBUS_EXCEP_ILLEGAL_FUNCTION = 1, //!< Illegal function code
	MODBUS_EXCEP_ILLEGAL_ADDRESS = 2, //!< Illegal data address
	MODBUS_EXCEP_ILLEGAL_VALUE = 3, //!< Illegal data value
	MODBUS_EXCEP_SLAVE_FAILURE = 4, //!< Slave could not process the request
	MODBUS_EXCEP_ACK = 5, //!< Acknowledge
	MODBUS_EXCEP_NACK = 7 //!< Negative acknowledge
} ModbusExceptionCode;

/**
	\brief Stores information about Modbus data types
*/
typedef enum modbusDataType
{
	MODBUS_HOLDING_REGISTER = 1, //!< Holding register
	MODBUS_INPUT_REGISTER = 2, //!< Input register
	MODBUS_COIL = 4, //!< Coil
	MODBUS_DISCRETE_INPUT = 8 //!< Discrete input
} ModbusDataType;

/**
	\brief Converts number of bits to number of bytes required to store them
	\param n Number of bits
	\returns Number of bytes of required memory
*/
static inline uint16_t modbusBitsToBytes( uint16_t n )
{
	return n != 0 ? ( 1 + ( ( n - 1 ) >> 3 ) ) : 0;
}

/**
	\brief Swaps endianness of provided 16-bit data portion

	\note This function, unlike \ref modbusMatchEndian, works unconditionally

	\param data A 16-bit data portion.
	\returns The same data, but with bytes swapped
	\see modbusMatchEndian
*/
static inline uint16_t modbusSwapEndian( uint16_t data ) { return ( data << 8 ) | ( data >> 8 ); }

/**
	\brief Swaps endianness of provided 16-bit data portion if needed

	\note This function works only if the system is not big-endian

	\param data A 16-bit data portion.
	\returns The same data, but with bytes swapped if the system is little-endian
	\see modbusSwapEndian
*/
#ifdef LIGHTMODBUS_BIG_ENDIAN
	static inline uint16_t modbusMatchEndian( uint16_t data ) { return data; }
#else
	static inline uint16_t modbusMatchEndian( uint16_t data ) { return modbusSwapEndian( data ); }
#endif

/**
	\brief Reads n-th bit from an array

	\param mask A pointer to the array
	\param maskLength The length of the array in bytes
	\param bit Number of the bit to be read
	\returns The bit value, or 255 if the bit lies outside the array.
*/
extern uint8_t modbusMaskRead( const uint8_t *mask, uint16_t maskLength, uint16_t bit );

/**
	\brief Writes n-th bit in an array

	\param mask A pointer to the array
	\param maskLength The length of the array in bytes
	\param bit Number of the bit to write
	\param value Bit value to be written
	\returns Bit value on success, 255 if the bit lies outside the array.
*/
extern uint8_t modbusMaskWrite( uint8_t *mask, uint16_t maskLength, uint16_t bit, uint8_t value );

/**
	\brief Calculates 16-bit Modbus CRC of provided data

	\param data A pointer to the data to be processed
	\param length Number of bytes, starting at the `data` pointer, to process
	\returns 16-bit Modbus CRC value
*/
extern uint16_t modbusCRC( const uint8_t *data, uint16_t length );

//For user convenience
#include "master.h"
#include "slave.h"

// For C++ (closes `extern "C"` )
#ifdef __cplusplus
}
#endif

#endif
