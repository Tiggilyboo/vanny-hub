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
#include <lightmodbus/master.h>
#include <lightmodbus/master/mpcoils.h>

#if defined(LIGHTMODBUS_F01M) || defined(LIGHTMODBUS_F02M)
ModbusError modbusParseResponse0102( ModbusMaster *status, ModbusParser *parser, ModbusParser *requestParser )
{
	//Parse slave response to request 01 (read multiple coils)

	//Check if given pointers are valid
	if ( status == NULL || parser == NULL || requestParser == NULL ) return MODBUS_ERROR_NULLPTR;
	if ( parser->base.function != 1 && parser->base.function != 2 )
	{
		status->parseError = MODBUS_FERROR_BADFUN;
		return MODBUS_ERROR_PARSE;
	}

	//Check if frame length is valid
	//Frame has to be at least 4 bytes long so byteCount can always be accessed in this case
	if ( status->response.length != 5 + parser->response0102.length || status->request.length != 8 )
	{
		status->parseError = MODBUS_FERROR_LENGTH;
		return MODBUS_ERROR_PARSE;
	}

	uint16_t count = modbusMatchEndian( requestParser->request0102.count );

	//Check between data sent to slave and received from slave
	if ( parser->base.address == 0 )
	{
		status->parseError = MODBUS_FERROR_BROADCAST;
		return MODBUS_ERROR_PARSE;
	}

	if ( parser->base.address != requestParser->base.address )
	{
		status->parseError = MODBUS_FERROR_MISM_ADDR;
		return MODBUS_ERROR_PARSE;
	}

	if ( parser->base.function != requestParser->base.function )
	{
		status->parseError = MODBUS_FERROR_MISM_FUN;
		return MODBUS_ERROR_PARSE;
	}

	//Length checks
	if ( parser->response0102.length == 0 ||  parser->response0102.length > 250 || parser->response0102.length != modbusBitsToBytes( count ) )
	{
		status->parseError = MODBUS_FERROR_LENGTH;
		return MODBUS_ERROR_PARSE;
	}

	#ifdef LIGHTMODBUS_NO_MASTER_DATA_BUFFER
		//When no data buffer is used, pointer has to point inside frame provided by user
		//That implies, frame cannot be copied for parsing!
		status->data.coils = parser->response0102.values;
		status->data.regs = (uint16_t*) status->data.coils;
	#else
		#ifndef LIGHTMODBUS_STATIC_MEM_MASTER_DATA
			status->data.coils = (uint8_t*) calloc( modbusBitsToBytes( count ), sizeof( uint8_t ) );
			status->data.regs = (uint16_t*) status->data.coils;
			if ( status->data.coils == NULL ) return MODBUS_ERROR_ALLOC;
		#else
			if ( modbusBitsToBytes( count ) * sizeof( uint8_t ) > LIGHTMODBUS_STATIC_MEM_MASTER_DATA ) return MODBUS_ERROR_ALLOC;
			memset( status->data.coils, 0, modbusBitsToBytes( count ) * sizeof( uint8_t ) );
		#endif

		//Copy the data
		memcpy( status->data.coils, parser->response0102.values, parser->response0102.length );
	#endif

	status->data.function = parser->base.function;
	status->data.address = parser->base.address;
	status->data.type = parser->base.function == 1 ? MODBUS_COIL : MODBUS_DISCRETE_INPUT;
	status->data.index = modbusMatchEndian( requestParser->request0102.index );
	status->data.count = count;
	status->data.length = parser->response0102.length;
	status->parseError = MODBUS_FERROR_OK;
	return MODBUS_ERROR_OK;
}
#endif

#ifdef LIGHTMODBUS_F05M
ModbusError modbusParseResponse05( ModbusMaster *status, ModbusParser *parser, ModbusParser *requestParser )
{
	//Parse slave response to request 05 (write single coil)

	//Check if given pointers are valid
	if ( status == NULL || parser == NULL || requestParser == NULL ) return MODBUS_ERROR_NULLPTR;

	//Check frame lengths
	if ( status->response.length != 8 || status->request.length != 8 )
	{
		status->parseError = MODBUS_FERROR_LENGTH;
		return MODBUS_ERROR_PARSE;
	}

	//Check if frame was broadcast
	if ( parser->base.address == 0 )
	{
		status->parseError = MODBUS_FERROR_BROADCAST;
		return MODBUS_ERROR_PARSE;
	}

	//Check slave address in response and request
	if ( parser->base.address != requestParser->base.address )
	{
		status->parseError = MODBUS_FERROR_MISM_ADDR;
		return MODBUS_ERROR_PARSE;
	}

	//Check function in response and request
	if ( parser->base.function != requestParser->base.function )
	{
		status->parseError = MODBUS_FERROR_MISM_FUN;
		return MODBUS_ERROR_PARSE;
	}

	//Check index in response and request
	if ( parser->response05.index != requestParser->request05.index )
	{
		status->parseError = MODBUS_FERROR_MISM_INDEX;
		return MODBUS_ERROR_PARSE;
	}

	//Check index in response and request
	if ( parser->response05.value != requestParser->request05.value )
	{
		status->parseError = MODBUS_FERROR_MISM_VALUE;
		return MODBUS_ERROR_PARSE;
	}


	#ifdef LIGHTMODBUS_NO_MASTER_DATA_BUFFER
		//When no data buffer is used, pointer has to point inside frame provided by user
		//That implies, frame cannot be copied for parsing!
		status->data.coils = (uint8_t*) &parser->response05.value;
		status->data.regs = (uint16_t*) status->data.coils;
	#else
		#ifndef LIGHTMODBUS_STATIC_MEM_MASTER_DATA
			status->data.coils = (uint8_t*) calloc( 1, sizeof( uint8_t ) );
			status->data.regs = (uint16_t*) status->data.coils;
			if ( status->data.coils == NULL ) return MODBUS_ERROR_ALLOC;
		#else
			if ( 1 * sizeof( uint8_t ) > LIGHTMODBUS_STATIC_MEM_MASTER_DATA ) return MODBUS_ERROR_ALLOC;
			memset( status->data.coils, 0, 1 * sizeof( uint8_t ) );
		#endif

		//Copy the data
		status->data.coils[0] = parser->response05.value != 0;
	#endif

	status->data.function = 5;
	status->data.address = parser->base.address;
	status->data.type = MODBUS_COIL;
	status->data.index = modbusMatchEndian( requestParser->request05.index );
	status->data.count = 1;
	status->data.length = 1;
	status->parseError = MODBUS_FERROR_OK;
	return MODBUS_ERROR_OK;
}
#endif

#ifdef LIGHTMODBUS_F15M
ModbusError modbusParseResponse15( ModbusMaster *status, ModbusParser *parser, ModbusParser *requestParser )
{
	//Parse slave response to request 15 (write multiple coils)

	//Check if given pointers are valid
	if ( status == NULL || parser == NULL || requestParser == NULL ) return MODBUS_ERROR_NULLPTR;

	//Check frame lengths
	if ( status->request.length < 7u || status->request.length != 9 + requestParser->request15.length || status->response.length != 8 )
	{
		status->parseError = MODBUS_FERROR_LENGTH;
		return MODBUS_ERROR_PARSE;
	}

	//Check between data sent to slave and received from slave
	if ( parser->base.address == 0 )
	{
		status->parseError = MODBUS_FERROR_BROADCAST;
		return MODBUS_ERROR_PARSE;
	}

	if ( parser->base.address != requestParser->base.address )
	{
		status->parseError = MODBUS_FERROR_MISM_ADDR;
		return MODBUS_ERROR_PARSE;
	}

	if ( parser->base.function != requestParser->base.function )
	{
		status->parseError = MODBUS_FERROR_MISM_FUN;
		return MODBUS_ERROR_PARSE;
	}

	if ( parser->response15.index != requestParser->request15.index )
	{
		status->parseError = MODBUS_FERROR_MISM_INDEX;
		return MODBUS_ERROR_PARSE;
	}

	if ( parser->response15.count != requestParser->request15.count )
	{
		status->parseError = MODBUS_FERROR_MISM_COUNT;
		return MODBUS_ERROR_PARSE;
	}

	uint16_t count = modbusMatchEndian( parser->response15.count );
	if ( count > 1968 )
	{
		status->parseError = MODBUS_FERROR_COUNT;
		return MODBUS_ERROR_PARSE;
	}

	status->data.address = parser->base.address;
	status->data.function = 15;
	status->data.type = MODBUS_COIL;
	status->data.index = modbusMatchEndian( parser->response15.index );
	status->data.count = modbusMatchEndian( parser->response15.count );
	status->data.length = 0;
	status->parseError = MODBUS_FERROR_OK;
	return MODBUS_ERROR_OK;
}
#endif
