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
#include <lightmodbus/slave/sregs.h>

#if defined(LIGHTMODBUS_F03S) || defined(LIGHTMODBUS_F04S)
ModbusError modbusParseRequest0304( ModbusSlave *status, ModbusParser *parser )
{
	//Read multiple holding registers or input registers
	//Using data from union pointer

	//Update frame length
	uint8_t frameLength = 8;
	uint8_t i = 0;

	//Check if given pointers are valid
	if ( status == NULL || parser == NULL ) return MODBUS_ERROR_NULLPTR;
	if ( parser->base.function != 3 && parser->base.function != 4 )
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
	{
		return modbusBuildExceptionErr( status, parser->base.function, MODBUS_EXCEP_ILLEGAL_VALUE, MODBUS_FERROR_LENGTH );
	}

	//Swap endianness of longer members (but not crc)
	uint16_t index = modbusMatchEndian( parser->request0304.index );
	uint16_t count = modbusMatchEndian( parser->request0304.count );

	//Currently handled data type
	#ifdef LIGHTMODBUS_REGISTER_CALLBACK
		ModbusDataType datatype = parser->base.function == 3 ? MODBUS_HOLDING_REGISTER : MODBUS_INPUT_REGISTER;
	#endif

	//Check if reg is in valid range
	if ( count == 0 || count > 125 )
	{
		//Illegal data value error
		return modbusBuildExceptionErr( status, parser->base.function, MODBUS_EXCEP_ILLEGAL_VALUE, MODBUS_FERROR_COUNT );
	}

	//Check if registers are accessible
	#ifdef LIGHTMODBUS_REGISTER_CALLBACK
		if ( status->registerCallback == NULL )
			return modbusBuildExceptionErr( status, parser->base.function, MODBUS_EXCEP_ILLEGAL_ADDRESS, MODBUS_FERROR_NOSRC );
	#else
		if ( ( parser->base.function == 3 ? status->registers : status->inputRegisters ) == NULL )
			return modbusBuildExceptionErr( status, parser->base.function, MODBUS_EXCEP_ILLEGAL_ADDRESS, MODBUS_FERROR_NOSRC );
	#endif

	if ( index >= ( parser->base.function == 3 ? status->registerCount : status->inputRegisterCount ) || \
		(uint32_t) index + (uint32_t) count > \
		(uint32_t) ( parser->base.function == 3 ? status->registerCount : status->inputRegisterCount ) )
	{
		//Illegal data address exception
		return modbusBuildExceptionErr( status, parser->base.function, MODBUS_EXCEP_ILLEGAL_ADDRESS, MODBUS_FERROR_RANGE );
	}

	//Check if all registers can be read (when using callback function)
	#ifdef LIGHTMODBUS_REGISTER_CALLBACK
		for ( i = 0; i < count; i++ )
			if ( status->registerCallback( MODBUS_REGQ_R_CHECK, datatype, index + i, 0, status->registerCallbackContext ) == 0 )
				return modbusBuildExceptionErr( status, parser->base.function, MODBUS_EXCEP_SLAVE_FAILURE, MODBUS_FERROR_NOREAD );
	#endif

	//Respond
	frameLength = 5 + ( count << 1 );

	#ifndef LIGHTMODBUS_STATIC_MEM_SLAVE_RESPONSE
		status->response.frame = (uint8_t *) calloc( frameLength, sizeof( uint8_t ) ); //Reallocate response frame memory to needed memory
		if ( status->response.frame == NULL )return MODBUS_ERROR_ALLOC;
	#else
		if ( frameLength > LIGHTMODBUS_STATIC_MEM_SLAVE_RESPONSE ) return MODBUS_ERROR_ALLOC;
	#endif

	ModbusParser *builder = (ModbusParser *) status->response.frame;

	//Set up basic response data
	builder->response0304.address = status->address;
	builder->response0304.function = parser->request0304.function;
	builder->response0304.length = count << 1;

	//Copy registers to response frame
	#ifdef LIGHTMODBUS_REGISTER_CALLBACK
		for ( i = 0; i < count; i++ )
			builder->response0304.values[i] = modbusMatchEndian( status->registerCallback( MODBUS_REGQ_R, datatype, index + i, 0, status->registerCallbackContext ) );
	#else
		for ( i = 0; i < count; i++ )
			builder->response0304.values[i] = modbusMatchEndian( ( parser->base.function == 3 ? status->registers : status->inputRegisters )[index + i] );
	#endif

	//Calculate and write crc (it can be unaligned)
	uint16_t crc = modbusCRC( builder->frame, frameLength - 2 );
	memcpy(builder->frame + frameLength - 2, &crc, 2);

	//Set frame length - frame is ready
	status->response.length = frameLength;
	status->parseError = MODBUS_FERROR_OK;
	return MODBUS_ERROR_OK;
}
#endif

#ifdef LIGHTMODBUS_F06S
ModbusError modbusParseRequest06( ModbusSlave *status, ModbusParser *parser )
{
	//Write single holding reg
	//Using data from union pointer

	//Update frame length
	uint8_t frameLength = 8;

	//Check if given pointers are valid
	if ( status == NULL || parser == NULL ) return MODBUS_ERROR_NULLPTR;

	//Check if frame length is valid
	if ( status->request.length != frameLength )
		return modbusBuildExceptionErr( status, 6, MODBUS_EXCEP_ILLEGAL_VALUE, MODBUS_FERROR_LENGTH );


	//Swap endianness of longer members (but not crc)
	uint16_t index = modbusMatchEndian( parser->request06.index );
	uint16_t value = modbusMatchEndian( parser->request06.value );

	//Check if reg is in valid range
	#ifdef LIGHTMODBUS_REGISTER_CALLBACK
		if ( index >= status->registerCount || status->registerCallback == NULL )
			return modbusBuildExceptionErr( status, 6, MODBUS_EXCEP_ILLEGAL_ADDRESS, MODBUS_FERROR_RANGE );
	#else
		if ( index >= status->registerCount || status->registers == NULL )
			return modbusBuildExceptionErr( status, 6, MODBUS_EXCEP_ILLEGAL_ADDRESS, MODBUS_FERROR_RANGE );
	#endif

	//Check if reg is allowed to be written
	#ifdef LIGHTMODBUS_REGISTER_CALLBACK
		if ( status->registerCallback( MODBUS_REGQ_W_CHECK, MODBUS_HOLDING_REGISTER, index, 0, status->registerCallbackContext ) == 0 )
			return modbusBuildExceptionErr( status, 6, MODBUS_EXCEP_SLAVE_FAILURE, MODBUS_FERROR_NOWRITE );
	#else
		if ( modbusMaskRead( status->registerMask, status->registerMaskLength, index ) == 1 )
			return modbusBuildExceptionErr( status, 6, MODBUS_EXCEP_SLAVE_FAILURE, MODBUS_FERROR_NOWRITE );
	#endif

	//Respond
	frameLength = 8;

	#ifndef LIGHTMODBUS_STATIC_MEM_SLAVE_RESPONSE
		status->response.frame = (uint8_t *) calloc( frameLength, sizeof( uint8_t ) ); //Reallocate response frame memory to needed memory
		if ( status->response.frame == NULL ) return MODBUS_ERROR_ALLOC;
	#else
		if ( frameLength > LIGHTMODBUS_STATIC_MEM_SLAVE_RESPONSE ) return MODBUS_ERROR_ALLOC;
	#endif

	ModbusParser *builder = (ModbusParser *) status->response.frame;

	//After all possible exceptions, write reg
	#ifdef LIGHTMODBUS_REGISTER_CALLBACK
		status->registerCallback( MODBUS_REGQ_W, MODBUS_HOLDING_REGISTER, index, value, status->registerCallbackContext );
	#else
		status->registers[index] = value;
	#endif

	//Do not respond when frame is broadcasted
	if ( parser->base.address == 0 )
	{
		status->parseError = MODBUS_FERROR_OK;
		return MODBUS_ERROR_OK;
	}

	//Set up basic response data
	builder->response06.address = status->address;
	builder->response06.function = parser->request06.function;
	builder->response06.index = parser->request06.index;
	builder->response06.value = parser->request06.value;

	//Calculate crc
	builder->response06.crc = modbusCRC( builder->frame, frameLength - 2 );

	//Set frame length - frame is ready
	status->response.length = frameLength;
	status->parseError = MODBUS_FERROR_OK;
	return MODBUS_ERROR_OK;
}
#endif

#ifdef LIGHTMODBUS_F16S
ModbusError modbusParseRequest16( ModbusSlave *status, ModbusParser *parser )
{
	//Write multiple holding registers
	//Using data from union pointer

	//Update frame length
	uint8_t i = 0;
	uint8_t frameLength;

	//Check if given pointers are valid
	if ( status == NULL || parser == NULL ) return MODBUS_ERROR_NULLPTR;

	//Check if frame length is valid
	if ( status->request.length >= 7u )
	{
		frameLength = 9 + parser->request16.length;
		if ( status->request.length != frameLength )
			return modbusBuildExceptionErr( status, 16, MODBUS_EXCEP_ILLEGAL_VALUE, MODBUS_FERROR_LENGTH );

	}
	else return modbusBuildExceptionErr( status, 16, MODBUS_EXCEP_ILLEGAL_VALUE, MODBUS_FERROR_LENGTH );

	//Swap endianness of longer members (but not crc)
	uint16_t index = modbusMatchEndian( parser->request16.index );
	uint16_t count = modbusMatchEndian( parser->request16.count );

	//Data checks
	if ( parser->request16.length == 0 || \
		count == 0 || \
		count != ( parser->request16.length >> 1 ) || \
		count > 123 )
			return modbusBuildExceptionErr( status, 16, MODBUS_EXCEP_ILLEGAL_VALUE, MODBUS_FERROR_COUNT );


	//Check if registers are accessible
	#ifdef LIGHTMODBUS_REGISTER_CALLBACK
		if ( status->registerCallback == NULL )
			return modbusBuildExceptionErr( status, 16, MODBUS_EXCEP_ILLEGAL_ADDRESS, MODBUS_FERROR_NOSRC );
	#else
		if ( status->registers == NULL )
			return modbusBuildExceptionErr( status, 16, MODBUS_EXCEP_ILLEGAL_ADDRESS, MODBUS_FERROR_NOSRC );
	#endif

	if ( index >= status->registerCount || \
		(uint32_t) index + (uint32_t) count > (uint32_t) status->registerCount )
			return modbusBuildExceptionErr( status, 16, MODBUS_EXCEP_ILLEGAL_ADDRESS, MODBUS_FERROR_RANGE );

	//Check for write protection
	#ifdef LIGHTMODBUS_REGISTER_CALLBACK
		for ( i = 0; i < count; i++ )
			if ( status->registerCallback( MODBUS_REGQ_W_CHECK, MODBUS_HOLDING_REGISTER, index + i, 0, status->registerCallbackContext ) == 0 )
				return modbusBuildExceptionErr( status, 16, MODBUS_EXCEP_SLAVE_FAILURE, MODBUS_FERROR_NOWRITE );
	#else
		for ( i = 0; i < count; i++ )
			if ( modbusMaskRead( status->registerMask, status->registerMaskLength, index + i ) == 1 )
				return modbusBuildExceptionErr( status, 16, MODBUS_EXCEP_SLAVE_FAILURE, MODBUS_FERROR_NOWRITE );
	#endif

	//Respond
	frameLength = 8;

	#ifndef LIGHTMODBUS_STATIC_MEM_SLAVE_RESPONSE
		status->response.frame = (uint8_t *) calloc( frameLength, sizeof( uint8_t ) ); //Reallocate response frame memory to needed memory
		if ( status->response.frame == NULL ) return MODBUS_ERROR_ALLOC;
	#else
		if ( frameLength > LIGHTMODBUS_STATIC_MEM_SLAVE_RESPONSE ) return MODBUS_ERROR_ALLOC;
	#endif

	ModbusParser *builder = (ModbusParser *) status->response.frame;


	//After all possible exceptions, write values to registers
	#ifdef LIGHTMODBUS_REGISTER_CALLBACK
		for ( i = 0; i < count; i++ )
			status->registerCallback( MODBUS_REGQ_W, MODBUS_HOLDING_REGISTER, index + i, modbusMatchEndian( parser->request16.values[i] ), status->registerCallbackContext );
	#else
		for ( i = 0; i < count; i++ )
			status->registers[index + i] = modbusMatchEndian( parser->request16.values[i] );
	#endif

	//Do not respond when frame is broadcasted
	if ( parser->base.address == 0 )
	{
		status->parseError = MODBUS_FERROR_OK;
		return MODBUS_ERROR_OK;
	}

	//Set up basic response data
	builder->response16.address = status->address;
	builder->response16.function = parser->request16.function;
	builder->response16.index = parser->request16.index;
	builder->response16.count = parser->request16.count;

	//Calculate crc
	builder->response16.crc = modbusCRC( builder->frame, frameLength - 2 );

	//Set frame length - frame is ready
	status->response.length = frameLength;
	status->parseError = MODBUS_FERROR_OK;
	return MODBUS_ERROR_OK;
}
#endif

#ifdef LIGHTMODBUS_F22S
ModbusError modbusParseRequest22( ModbusSlave *status, ModbusParser *parser )
{
	//Mask write single holding reg
	//Using data from union pointer

	//Update frame length
	uint8_t frameLength = 10;

	//Check if given pointers are valid
	if ( status == NULL || parser == NULL ) return MODBUS_ERROR_NULLPTR;

	//Check if frame length is valid
	if ( status->request.length != frameLength )
		return modbusBuildExceptionErr( status, 22, MODBUS_EXCEP_ILLEGAL_VALUE, MODBUS_FERROR_LENGTH );

	//Swap endianness of longer members (but not crc)
	uint16_t index = modbusMatchEndian( parser->request22.index );
	uint16_t andmask = modbusMatchEndian( parser->request22.andmask );
	uint16_t ormask = modbusMatchEndian( parser->request22.ormask );

	//Check if registers are accessible
	#ifdef LIGHTMODBUS_REGISTER_CALLBACK
		if ( status->registerCallback == NULL )
			return modbusBuildExceptionErr( status, 22, MODBUS_EXCEP_ILLEGAL_ADDRESS, MODBUS_FERROR_NOSRC );

	#else
		if ( status->registers == NULL )
			return modbusBuildExceptionErr( status, 22, MODBUS_EXCEP_ILLEGAL_ADDRESS, MODBUS_FERROR_NOSRC );
	#endif

	//Check if reg is in valid range
	if ( index >= status->registerCount )
		return modbusBuildExceptionErr( status, 22, MODBUS_EXCEP_ILLEGAL_ADDRESS, MODBUS_FERROR_RANGE );

	//Check if reg is allowed to be written and read
	#ifdef LIGHTMODBUS_REGISTER_CALLBACK
		if ( status->registerCallback( MODBUS_REGQ_R_CHECK, MODBUS_HOLDING_REGISTER, index, 0, status->registerCallbackContext ) == 0 )
			return modbusBuildExceptionErr( status, 22, MODBUS_EXCEP_SLAVE_FAILURE, MODBUS_FERROR_NOREAD );
		if ( status->registerCallback( MODBUS_REGQ_W_CHECK, MODBUS_HOLDING_REGISTER, index, 0, status->registerCallbackContext ) == 0 )
			return modbusBuildExceptionErr( status, 22, MODBUS_EXCEP_SLAVE_FAILURE, MODBUS_FERROR_NOWRITE );
	#else
		if ( modbusMaskRead( status->registerMask, status->registerMaskLength, index ) == 1 )
			return modbusBuildExceptionErr( status, 22, MODBUS_EXCEP_SLAVE_FAILURE, MODBUS_FERROR_NOWRITE );
	#endif

	//Respond
	frameLength = 10;

	#ifndef LIGHTMODBUS_STATIC_MEM_SLAVE_RESPONSE
		status->response.frame = (uint8_t *) calloc( frameLength, sizeof( uint8_t ) ); //Reallocate response frame memory to needed memory
		if ( status->response.frame == NULL ) return MODBUS_ERROR_ALLOC;
	#else
		if ( frameLength > LIGHTMODBUS_STATIC_MEM_SLAVE_RESPONSE ) return MODBUS_ERROR_ALLOC;
	#endif

	ModbusParser *builder = (ModbusParser *) status->response.frame;

	//After all possible exceptions, read reg
	uint16_t value; //Value as stored in memory
	#ifdef LIGHTMODBUS_REGISTER_CALLBACK
		value = status->registerCallback( MODBUS_REGQ_R, MODBUS_HOLDING_REGISTER, index, 0, status->registerCallbackContext );
	#else
		value = status->registers[index];
	#endif

	//Do the bitwise magic
	value = ( value & andmask ) | ( ormask & ~andmask );

	#ifdef LIGHTMODBUS_REGISTER_CALLBACK
		status->registerCallback( MODBUS_REGQ_W, MODBUS_HOLDING_REGISTER, index, value, status->registerCallbackContext );
	#else
		status->registers[index] = value;
	#endif

	//Do not respond when frame is broadcasted
	if ( parser->base.address == 0 )
	{
		status->parseError = MODBUS_FERROR_OK;
		return MODBUS_ERROR_OK;
	}

	//Set up basic response data
	builder->response22.address = status->address;
	builder->response22.function = parser->request22.function;
	builder->response22.index = parser->request22.index;
	builder->response22.andmask = parser->request22.andmask;
	builder->response22.ormask = parser->request22.ormask;

	//Calculate crc
	builder->response22.crc = modbusCRC( builder->frame, frameLength - 2 );

	//Set frame length - frame is ready
	status->response.length = frameLength;
	status->parseError = MODBUS_FERROR_OK;
	return MODBUS_ERROR_OK;
}
#endif
