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
#include <lightmodbus/lightmodbus.h>
#include <lightmodbus/parser.h>
#include <lightmodbus/master.h>
#include <lightmodbus/master/mpregs.h>

#if defined(LIGHTMODBUS_F03M) || defined(LIGHTMODBUS_F04M)
ModbusError modbusParseResponse0304( ModbusMaster *status, ModbusParser *parser, ModbusParser *requestParser )
{
	//Parse slave response to request 03
	//Read multiple holding registers

	uint8_t i = 0;

	//Check if given pointers are valid
	if ( status == NULL || parser == NULL || requestParser == NULL ) return MODBUS_ERROR_NULLPTR;
	if ( parser->base.function != 3 && parser->base.function != 4 )
	{
		status->parseError = MODBUS_FERROR_BADFUN;
		return MODBUS_ERROR_PARSE;
	}

	//Check if frame length is valid
	//Frame has to be at least 4 bytes long so byteCount can always be accessed in this case
	if ( status->response.length != 5 + parser->response0304.length || status->request.length != 8 )
	{
		status->parseError = MODBUS_FERROR_LENGTH;
		return MODBUS_ERROR_PARSE;
	}

	uint16_t count = modbusMatchEndian( requestParser->request0304.count );

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

	if ( parser->response0304.length == 0 || parser->response0304.length != count << 1 || parser->response0304.length > 250 )
	{
		status->parseError = MODBUS_FERROR_LENGTH;
		return MODBUS_ERROR_PARSE;
	}

	#ifdef LIGHTMODBUS_NO_MASTER_DATA_BUFFER
		//When no data buffer is used, pointer has to point inside frame provided by user
		//That implies, frame cannot be copied for parsing!
		status->data.regs = parser->response0304.values;
		status->data.coils = (uint8_t*) status->data.regs;

		//If frame is allowed to be modified
		#ifdef LIGHTMODBUS_MASTER_INVASIVE_PARSING
			//Overwrite data in buffer
			for ( i = 0; i < count; i++ )
				status->data.regs[i] = modbusMatchEndian( parser->response0304.values[i] );
		#endif
	#else
		#ifndef LIGHTMODBUS_STATIC_MEM_MASTER_DATA
			//Allocate memory for ModbusData structures array
			status->data.coils = (uint8_t*) calloc( parser->response0304.length >> 1, sizeof( uint16_t ) );
			status->data.regs = (uint16_t*) status->data.coils;
			if ( status->data.coils == NULL ) return MODBUS_ERROR_ALLOC;
		#else
			if ( ( parser->response0304.length >> 1 ) * sizeof( uint16_t ) > LIGHTMODBUS_STATIC_MEM_MASTER_DATA ) return MODBUS_ERROR_ALLOC;
		#endif

		//Copy received data (with swapping endianness)
		for ( i = 0; i < count; i++ )
			status->data.regs[i] = modbusMatchEndian( parser->response0304.values[i] );
	#endif

	status->data.address = parser->base.address;
	status->data.function = parser->base.function;
	status->data.type = parser->base.function == 3 ? MODBUS_HOLDING_REGISTER : MODBUS_INPUT_REGISTER;
	status->data.index = modbusMatchEndian( requestParser->request0304.index );
	status->data.count = count;
	status->data.length = parser->response0304.length;
	status->parseError = MODBUS_FERROR_OK;
	return MODBUS_ERROR_OK;
}
#endif

#ifdef LIGHTMODBUS_F06M
ModbusError modbusParseResponse06( ModbusMaster *status, ModbusParser *parser, ModbusParser *requestParser )
{
	//Parse slave response to request 06 (write single holding reg)

	//Check if given pointers are valid
	if ( status == NULL || parser == NULL || requestParser == NULL ) return MODBUS_ERROR_NULLPTR;

	//Check if frame length is valid
	//Frame has to be at least 4 bytes long so byteCount can always be accessed in this case
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
	if ( parser->response06.index != requestParser->request06.index )
	{
		status->parseError = MODBUS_FERROR_MISM_INDEX;
		return MODBUS_ERROR_PARSE;
	}

	//Check value in response and request
	if ( parser->response06.value != requestParser->request06.value )
	{
		status->parseError = MODBUS_FERROR_MISM_VALUE;
		return MODBUS_ERROR_PARSE;
	}

	#ifdef LIGHTMODBUS_NO_MASTER_DATA_BUFFER
		//When no data buffer is used, pointer has to point inside frame provided by user
		//That implies, frame cannot be copied for parsing!
		status->data.regs = &parser->response06.value;
		status->data.coils = (uint8_t*) status->data.regs;

		//If frame is allowed to be modified
		#ifdef LIGHTMODBUS_MASTER_INVASIVE_PARSING
			//Overwrite data in buffer
			status->data.regs[0] = modbusMatchEndian( parser->response06.value );
		#endif
	#else
		#ifndef LIGHTMODBUS_STATIC_MEM_MASTER_DATA
			//Set up new data table
			status->data.coils = (uint8_t*) calloc( 1, sizeof( uint16_t ) );
			status->data.regs = (uint16_t*) status->data.coils;
			if ( status->data.coils == NULL ) return MODBUS_ERROR_ALLOC;
		#else
			if ( 1 * sizeof( uint16_t ) > LIGHTMODBUS_STATIC_MEM_MASTER_DATA ) return MODBUS_ERROR_ALLOC;
		#endif

		//Copy the data
		status->data.regs[0] = modbusMatchEndian( parser->response06.value );
	#endif

	status->data.function = 6;
	status->data.address = parser->base.address;
	status->data.type = MODBUS_HOLDING_REGISTER;
	status->data.index = modbusMatchEndian( parser->response06.index );
	status->data.count = 1;
	status->data.length = 2;
	status->parseError = MODBUS_FERROR_OK;
	return MODBUS_ERROR_OK;
}
#endif

#ifdef LIGHTMODBUS_F16M
ModbusError modbusParseResponse16( ModbusMaster *status, ModbusParser *parser, ModbusParser *requestParser )
{
	//Parse slave response to request 16 (write multiple holding reg)

	//Check if given pointers are valid
	if ( status == NULL || parser == NULL || requestParser == NULL ) return MODBUS_ERROR_NULLPTR;

	//Check frame lengths
	if ( status->request.length < 7u || status->request.length != 9 + requestParser->request16.length || status->response.length != 8 )
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

	if ( parser->response16.index != requestParser->request16.index )
	{
		status->parseError = MODBUS_FERROR_MISM_INDEX;
		return MODBUS_ERROR_PARSE;
	}

	if ( parser->response16.count != requestParser->request16.count )
	{
		status->parseError = MODBUS_FERROR_MISM_COUNT;
		return MODBUS_ERROR_PARSE;
	}

	uint16_t count = modbusMatchEndian( parser->response16.count );
	if ( count > 123 )
	{
		status->parseError = MODBUS_FERROR_COUNT;
		return MODBUS_ERROR_PARSE;
	}

	//Set up data length - response successfully parsed
	status->data.address = parser->base.address;
	status->data.function = 16;
	status->data.type = MODBUS_HOLDING_REGISTER;
	status->data.index = modbusMatchEndian( parser->response16.index );
	status->data.count = count;
	status->data.length = 0;
	status->parseError = MODBUS_FERROR_OK;
	return MODBUS_ERROR_OK;
}
#endif

#ifdef LIGHTMODBUS_F22M
ModbusError modbusParseResponse22( ModbusMaster *status, ModbusParser *parser, ModbusParser *requestParser )
{
	//Parse slave response to request 22 (mask write single holding reg)

	//Check if given pointers are valid
	if ( status == NULL || parser == NULL || requestParser == NULL ) return MODBUS_ERROR_NULLPTR;

	//Check if frame length is valid
	//Frame has to be at least 4 bytes long so byteCount can always be accessed in this case
	if ( status->response.length != 10 || status->request.length != 10 )
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

	if ( parser->response22.index != requestParser->request22.index )
	{
		status->parseError = MODBUS_FERROR_MISM_INDEX;
		return MODBUS_ERROR_PARSE;
	}

	if ( parser->response22.andmask != requestParser->request22.andmask || parser->response22.ormask != requestParser->request22.ormask )
	{
		status->parseError = MODBUS_FERROR_MISM_MASK;
		return MODBUS_ERROR_PARSE;
	}

	//Set up new data table
	status->data.address = parser->base.address;
	status->data.function = 22;
	status->data.type = MODBUS_HOLDING_REGISTER;
	status->data.index = modbusMatchEndian( parser->response22.index );
	status->data.count = 1;
	status->data.length = 0;
	status->parseError = MODBUS_FERROR_OK;
	return MODBUS_ERROR_OK;
}
#endif
