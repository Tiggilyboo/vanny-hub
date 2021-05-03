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

#include <stdlib.h>
#include <string.h>
#include <lightmodbus/lightmodbus.h>
#include <lightmodbus/parser.h>
#include <lightmodbus/slave.h>
#include <lightmodbus/slave/scoils.h>

#if defined(LIGHTMODBUS_F01S) || defined(LIGHTMODBUS_F02S)
ModbusError modbusParseRequest0102( ModbusSlave *status, ModbusParser *parser )
{
	//Read multiple coils or discrete inputs
	//Using data from union pointer

	//Update frame length
	uint8_t frameLength = 8;
	uint16_t i = 0;

	//Check if given pointers are valid
	if ( status == NULL || parser == NULL ) return MODBUS_ERROR_NULLPTR;
	if ( parser->base.function != 1 && parser->base.function != 2 )
	{
		status->parseError = MODBUS_FERROR_BADFUN;
		return MODBUS_ERROR_PARSE;
	}

	//Don't do anything when frame is broadcasted
	//Base of the frame can be always safely checked, because main parser function takes care of that
	if ( parser->base.address == 0 )
	{
		status->parseError = MODBUS_FERROR_BROADCAST;
		return MODBUS_ERROR_PARSE;
	}

	//Check if frame length is valid
	if ( status->request.length != frameLength )
		return modbusBuildExceptionErr( status, parser->base.function, MODBUS_EXCEP_ILLEGAL_VALUE, MODBUS_FERROR_LENGTH );

	//Swap endianness of longer members (but not crc)
	uint16_t index = modbusMatchEndian( parser->request0102.index );
	uint16_t count = modbusMatchEndian( parser->request0102.count );

	//Currently handled data type
	#ifdef LIGHTMODBUS_COIL_CALLBACK
		ModbusDataType datatype = parser->base.function == 1 ? MODBUS_COIL : MODBUS_DISCRETE_INPUT;
	#endif

	//Check if coil count is in valid range
	if ( count == 0 || count > 2000 )
		return modbusBuildExceptionErr( status, parser->base.function, MODBUS_EXCEP_ILLEGAL_VALUE, MODBUS_FERROR_COUNT );

	//Check if coils are accessible
	#ifdef LIGHTMODBUS_COIL_CALLBACK
		if ( status->registerCallback == NULL )
			return modbusBuildExceptionErr( status, parser->base.function, MODBUS_EXCEP_ILLEGAL_ADDRESS, MODBUS_FERROR_NOSRC );
	#else
		if ( ( parser->base.function == 1 ? status->coils : status->discreteInputs ) == NULL )
			return modbusBuildExceptionErr( status, parser->base.function, MODBUS_EXCEP_ILLEGAL_ADDRESS, MODBUS_FERROR_NOSRC );
	#endif

	if ( index >= ( parser->base.function == 1 ? status->coilCount : status->discreteInputCount ) || \
		(uint32_t) index + (uint32_t) count > (uint32_t) ( parser->base.function == 1 ? status->coilCount : status->discreteInputCount ) )
			return modbusBuildExceptionErr( status, parser->base.function, MODBUS_EXCEP_ILLEGAL_ADDRESS, MODBUS_FERROR_RANGE );

	//Check if coils can be read (if using callback function)
	#ifdef LIGHTMODBUS_COIL_CALLBACK
		for ( i = 0; i < count; i++ )
			if ( status->registerCallback( MODBUS_REGQ_R_CHECK, datatype, index + i, 0, status->registerCallbackContext ) == 0 )
				return modbusBuildExceptionErr( status, parser->base.function, MODBUS_EXCEP_SLAVE_FAILURE, MODBUS_FERROR_NOREAD );
	#endif

	//Respond
	frameLength = 5 + modbusBitsToBytes( count );

	#ifndef LIGHTMODBUS_STATIC_MEM_SLAVE_RESPONSE
		status->response.frame = (uint8_t *) calloc( frameLength, sizeof( uint8_t ) ); //Reallocate response frame memory to needed memory
		if ( status->response.frame == NULL ) return MODBUS_ERROR_ALLOC;
	#else
		if ( frameLength > LIGHTMODBUS_STATIC_MEM_SLAVE_RESPONSE ) return MODBUS_ERROR_ALLOC;
		memset( status->response.frame, 0, frameLength );
	#endif

	ModbusParser *builder = (ModbusParser *) status->response.frame;

	//Set up basic response data
	builder->base.address = status->address;
	builder->base.function = parser->base.function;
	builder->response0102.length = modbusBitsToBytes( count );

	//Copy registers to response frame
	for ( i = 0; i < count; i++ )
	{
		uint8_t coil;

		#ifdef LIGHTMODBUS_COIL_CALLBACK
			coil = status->registerCallback( MODBUS_REGQ_R, datatype, index + i, 0, status->registerCallbackContext );
		#else
			if ( ( coil = modbusMaskRead( parser->base.function == 1 ? status->coils : status->discreteInputs, \
				modbusBitsToBytes( parser->base.function == 1 ? status->coilCount : status->discreteInputCount ), i + index ) ) == 255 )
					return MODBUS_ERROR_OTHER;
		#endif

		//Write to new frame
		if ( modbusMaskWrite( builder->response0102.values, builder->response0102.length, i, coil ) == 255 )
			return MODBUS_ERROR_OTHER;
	}

	//Calculate crc
	//That could be written as a single line, without the temporary variable, but it can cause
	//an unaligned memory access, which can cause runtime errors in some platforms like AVR and ARM.
	uint16_t crc = modbusCRC( builder->frame, frameLength - 2 );

	memcpy(builder->frame + frameLength - 2, &crc, 2);

	//Set frame length - frame is ready
	status->response.length = frameLength;
	return MODBUS_ERROR_OK;
}
#endif

#ifdef LIGHTMODBUS_F05S
ModbusError modbusParseRequest05( ModbusSlave *status, ModbusParser *parser )
{
	//Write single coil
	//Using data from union pointer

	//Update frame length
	uint8_t frameLength = 8;

	//Check if given pointers are valid
	if ( status == NULL || parser == NULL ) return MODBUS_ERROR_NULLPTR;

	//Check if frame length is valid
	if ( status->request.length != frameLength )
		return modbusBuildExceptionErr( status, 5, MODBUS_EXCEP_ILLEGAL_VALUE, MODBUS_FERROR_LENGTH );

	//Swap endianness of longer members (but not crc)
	uint16_t index = modbusMatchEndian( parser->request05.index );
	uint16_t value = modbusMatchEndian( parser->request05.value );

	//Check if coil value is valid
	if ( value != 0x0000 && value != 0xFF00 )
		return modbusBuildExceptionErr( status, 5, MODBUS_EXCEP_ILLEGAL_VALUE, MODBUS_FERROR_VALUE );


	//Check if coils are accessible
	#ifdef LIGHTMODBUS_COIL_CALLBACK
		if ( status->registerCallback == NULL )
			return modbusBuildExceptionErr( status, 5, MODBUS_EXCEP_ILLEGAL_ADDRESS, MODBUS_FERROR_NOSRC );
	#else
		if ( status->coils == NULL )
			return modbusBuildExceptionErr( status, 5, MODBUS_EXCEP_ILLEGAL_ADDRESS, MODBUS_FERROR_NOSRC );
	#endif

	//Check if coil is in valid range
	if ( index >= status->coilCount )
		return modbusBuildExceptionErr( status, 5, MODBUS_EXCEP_ILLEGAL_ADDRESS, MODBUS_FERROR_RANGE );


	//Check if reg is allowed to be written
	#ifdef LIGHTMODBUS_COIL_CALLBACK
		if ( status->registerCallback( MODBUS_REGQ_W_CHECK, MODBUS_COIL, index, 0, status->registerCallbackContext ) == 0 )
			return modbusBuildExceptionErr( status, 5, MODBUS_EXCEP_SLAVE_FAILURE, MODBUS_FERROR_NOWRITE );
	#else
		if ( modbusMaskRead( status->coilMask, status->coilMaskLength, index ) == 1 )
			return modbusBuildExceptionErr( status, 5, MODBUS_EXCEP_SLAVE_FAILURE, MODBUS_FERROR_NOWRITE );
	#endif

	//Respond
	frameLength = 8;

	#ifndef LIGHTMODBUS_STATIC_MEM_SLAVE_RESPONSE
		status->response.frame = (uint8_t *) calloc( frameLength, sizeof( uint8_t ) ); //Reallocate response frame memory to needed memory
		if ( status->response.frame == NULL ) return MODBUS_ERROR_ALLOC;
	#else
		if ( frameLength > LIGHTMODBUS_STATIC_MEM_SLAVE_RESPONSE ) return MODBUS_ERROR_ALLOC;
		memset( status->response.frame, 0, frameLength );
	#endif

	ModbusParser *builder = (ModbusParser *) status->response.frame;

	//After all possible exceptions, write coils
	#ifdef LIGHTMODBUS_COIL_CALLBACK
		status->registerCallback( MODBUS_REGQ_W, MODBUS_COIL, index, value == 0xFF00, status->registerCallbackContext );
	#else
		if ( modbusMaskWrite( status->coils, modbusBitsToBytes( status->coilCount ), index, value == 0xFF00 ) == 255 )
			return MODBUS_ERROR_OTHER;
	#endif

	//Do not respond when frame is broadcasted
	if ( parser->base.address == 0 )
	{
		status->parseError = MODBUS_FERROR_OK;
		return MODBUS_ERROR_OK;
	}

	//Set up basic response data
	builder->base.address = status->address;
	builder->base.function = parser->base.function;
	builder->response05.index = parser->request05.index;
	builder->response05.value = parser->request05.value;

	//Calculate crc
	builder->response05.crc = modbusCRC( builder->frame, frameLength - 2 );

	//Set frame length - frame is ready
	status->response.length = frameLength;
	status->parseError = MODBUS_FERROR_OK;
	return MODBUS_ERROR_OK;
}
#endif

#ifdef LIGHTMODBUS_F15S
ModbusError modbusParseRequest15( ModbusSlave *status, ModbusParser *parser )
{
	//Write multiple coils
	//Using data from union pointer

	//Update frame length
	uint16_t i = 0;
	uint8_t frameLength;

	//Check if given pointers are valid
	if ( status == NULL || parser == NULL ) return MODBUS_ERROR_NULLPTR;

	//Check if frame length is valid
	if ( status->request.length >= 7u )
	{
		frameLength = 9 + parser->request15.length;
		if ( status->request.length != frameLength )
			return modbusBuildExceptionErr( status, 15, MODBUS_EXCEP_ILLEGAL_VALUE, MODBUS_FERROR_LENGTH );

	}
	else
	{
		return modbusBuildExceptionErr( status, 15, MODBUS_EXCEP_ILLEGAL_VALUE, MODBUS_FERROR_LENGTH );
	}

	//Swap endianness of longer members (but not crc)
	uint16_t index = modbusMatchEndian( parser->request15.index );
	uint16_t count = modbusMatchEndian( parser->request15.count );

	//Data checks
	if ( parser->request15.length == 0 || \
		count == 0 || \
		modbusBitsToBytes( count ) != parser->request15.length || \
		count > 1968 )
			return modbusBuildExceptionErr( status, 15, MODBUS_EXCEP_ILLEGAL_VALUE, MODBUS_FERROR_COUNT );


	//Check if coils are accessible
	#ifdef LIGHTMODBUS_COIL_CALLBACK
		if ( status->registerCallback == NULL )
			return modbusBuildExceptionErr( status, 15, MODBUS_EXCEP_ILLEGAL_ADDRESS, MODBUS_FERROR_NOSRC );

	#else
		if ( status->coils == NULL )
			return modbusBuildExceptionErr( status, 15, MODBUS_EXCEP_ILLEGAL_ADDRESS, MODBUS_FERROR_NOSRC );
	#endif

	if ( index >= status->coilCount || \
		(uint32_t) index + (uint32_t) count > (uint32_t) status->coilCount )
			return modbusBuildExceptionErr( status, 15, MODBUS_EXCEP_ILLEGAL_ADDRESS, MODBUS_FERROR_RANGE );


	//Check for write protection
	#ifdef LIGHTMODBUS_COIL_CALLBACK
		for ( i = 0; i < count; i++ )
			if ( status->registerCallback( MODBUS_REGQ_W_CHECK, MODBUS_COIL, index + i, 0, status->registerCallbackContext ) == 0 )
				return modbusBuildExceptionErr( status, 15, MODBUS_EXCEP_SLAVE_FAILURE, MODBUS_FERROR_NOWRITE );

	#else
		for ( i = 0; i < count; i++ )
			if ( modbusMaskRead( status->coilMask, status->coilMaskLength, index + i ) == 1 )
				return modbusBuildExceptionErr( status, 15, MODBUS_EXCEP_SLAVE_FAILURE, MODBUS_FERROR_NOWRITE );
	#endif

	//Respond
	frameLength = 8;

	#ifndef LIGHTMODBUS_STATIC_MEM_SLAVE_RESPONSE
		status->response.frame = (uint8_t *) calloc( frameLength, sizeof( uint8_t ) ); //Reallocate response frame memory to needed memory
		if ( status->response.frame == NULL ) return MODBUS_ERROR_ALLOC;
	#else
		if ( frameLength > LIGHTMODBUS_STATIC_MEM_SLAVE_RESPONSE ) return MODBUS_ERROR_ALLOC;
		memset( status->response.frame, 0, frameLength );
	#endif

	ModbusParser *builder = (ModbusParser *) status->response.frame; //Allocate memory for builder union

	//After all possible exceptions write values to registers
	for ( i = 0; i < count; i++ )
	{
		uint8_t coil;
		if ( ( coil = modbusMaskRead( parser->request15.values, parser->request15.length, i ) ) == 255 ) return MODBUS_ERROR_OTHER;

		#ifdef LIGHTMODBUS_COIL_CALLBACK
			status->registerCallback( MODBUS_REGQ_W, MODBUS_COIL, index + i, coil, status->registerCallbackContext );
		#else
			if ( modbusMaskWrite( status->coils, modbusBitsToBytes( status->coilCount ), index + i, coil ) == 255 ) return MODBUS_ERROR_OTHER;
		#endif
	}

	//Do not respond when frame is broadcasted
	if ( parser->base.address == 0 )
	{
		status->parseError = MODBUS_FERROR_OK;
		return MODBUS_ERROR_OK;
	}

	//Set up basic response data
	builder->base.address = status->address;
	builder->base.function = parser->base.function;
	builder->response15.index = parser->request15.index;
	builder->response15.count = parser->request15.count;

	//Calculate crc
	builder->response15.crc = modbusCRC( builder->frame, frameLength - 2 );

	//Set frame length - frame is ready
	status->response.length = frameLength;
	status->parseError = MODBUS_FERROR_OK;
	return MODBUS_OK;
}
#endif
