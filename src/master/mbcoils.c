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
#include <lightmodbus/master/mbcoils.h>

#if defined(LIGHTMODBUS_F01M) || defined(LIGHTMODBUS_F02M)
ModbusError modbusBuildRequest0102( ModbusMaster *status, uint8_t function, uint8_t address, uint16_t index, uint16_t count )
{
	//Build request01 frame, to send it so slave
	//Read multiple coils

	//Set frame length
	uint8_t frameLength = 8;

	//Check if given pointer is valid
	if ( status == NULL ) return MODBUS_ERROR_NULLPTR;
	if ( function != 1 && function != 2 )
	{
		status->buildError = MODBUS_FERROR_BADFUN;
		return MODBUS_ERROR_BUILD;
	}

	if ( address == 0 )
	{
		status->buildError = MODBUS_FERROR_BROADCAST;
		return MODBUS_ERROR_BUILD;
	}

	//Set output frame length to 0 (in case of interrupts)
	status->request.length = 0;
	status->predictedResponseLength = 0;

	//Check count
	if ( count == 0 || count > 2000 )
	{
		status->buildError = MODBUS_FERROR_COUNT;
		return MODBUS_ERROR_BUILD;
	}

	#ifndef LIGHTMODBUS_STATIC_MEM_MASTER_REQUEST
		//Reallocate memory for final frame
		free( status->request.frame );
		status->request.frame = (uint8_t *) calloc( frameLength, sizeof( uint8_t ) );
		if ( status->request.frame == NULL ) return MODBUS_ERROR_ALLOC;
	#else
		if ( frameLength > LIGHTMODBUS_STATIC_MEM_MASTER_REQUEST ) return MODBUS_ERROR_ALLOC;
		memset( status->request.frame, 0, frameLength );
	#endif

	ModbusParser *builder = (ModbusParser *) status->request.frame;

	builder->base.address = address;
	builder->base.function = function;
	builder->request0102.index = modbusMatchEndian( index );
	builder->request0102.count = modbusMatchEndian( count );

	//Calculate crc
	builder->request0102.crc = modbusCRC( builder->frame, frameLength - 2 );

	status->request.length = frameLength;
	status->predictedResponseLength = 4 + 1 + modbusBitsToBytes( count );
	status->buildError = MODBUS_FERROR_OK;
	return MODBUS_ERROR_OK;
}
#endif

#ifdef LIGHTMODBUS_F05M
ModbusError modbusBuildRequest05( ModbusMaster *status, uint8_t address, uint16_t index, uint16_t value )
{
	//Build request05 frame, to send it so slave
	//Write single coil

	//Set frame length
	uint8_t frameLength = 8;

	//Check if given pointer is valid
	if ( status == NULL ) return MODBUS_ERROR_NULLPTR;

	//Set output frame length to 0 (in case of interrupts)
	status->request.length = 0;
	status->predictedResponseLength = 0;

	#ifndef LIGHTMODBUS_STATIC_MEM_MASTER_REQUEST
		//Reallocate memory for final frame
		free( status->request.frame );
		status->request.frame = (uint8_t *) calloc( frameLength, sizeof( uint8_t ) );
		if ( status->request.frame == NULL ) return MODBUS_ERROR_ALLOC;
	#else
		if ( frameLength > LIGHTMODBUS_STATIC_MEM_MASTER_REQUEST ) return MODBUS_ERROR_ALLOC;
		memset( status->request.frame, 0, frameLength );
	#endif

	ModbusParser *builder = (ModbusParser *) status->request.frame;

	value = ( value != 0 ) ? 0xFF00 : 0x0000;

	builder->base.address = address;
	builder->base.function = 5;
	builder->request05.index = modbusMatchEndian( index );
	builder->request05.value = modbusMatchEndian( value );

	//Calculate crc
	builder->request05.crc = modbusCRC( builder->frame, frameLength - 2 );

	status->request.length = frameLength;
	if ( address ) status->predictedResponseLength = 8;

	status->buildError = MODBUS_FERROR_OK;
	return MODBUS_ERROR_OK;
}
#endif

#ifdef LIGHTMODBUS_F15M
ModbusError modbusBuildRequest15( ModbusMaster *status, uint8_t address, uint16_t index, uint16_t count, uint8_t *values )
{
	//Build request15 frame, to send it so slave
	//Write multiple coils

	//Set frame length
	uint8_t frameLength = 9 + modbusBitsToBytes( count );
	uint8_t i = 0;

	//Check if given pointer is valid
	if ( status == NULL || values == NULL ) return MODBUS_ERROR_NULLPTR;

	//Set output frame length to 0 (in case of interrupts)
	status->request.length = 0;
	status->predictedResponseLength = 0;

	//Check values pointer
	if ( count == 0 || count > 1968 )
	{
		status->buildError = MODBUS_FERROR_COUNT;
		return MODBUS_ERROR_BUILD;
	}

	#ifndef LIGHTMODBUS_STATIC_MEM_MASTER_REQUEST
		//Reallocate memory for final frame
		free( status->request.frame );
		status->request.frame = (uint8_t *) calloc( frameLength, sizeof( uint8_t ) );
		if ( status->request.frame == NULL ) return MODBUS_ERROR_ALLOC;
	#else
		if ( frameLength > LIGHTMODBUS_STATIC_MEM_MASTER_REQUEST ) return MODBUS_ERROR_ALLOC;
		memset( status->request.frame, 0, frameLength );
	#endif

	ModbusParser *builder = (ModbusParser *) status->request.frame;

	builder->base.address = address;
	builder->base.function = 15;
	builder->request15.index = modbusMatchEndian( index );
	builder->request15.count = modbusMatchEndian( count );
	builder->request15.length = modbusBitsToBytes( count );

	for ( i = 0; i < builder->request15.length; i++ )
		builder->request15.values[i] = values[i];


	//That could be written as a single line, without the temporary variable, but it can cause
	//an unaligned memory access, which can cause runtime errors in some platforms like AVR and ARM.
	uint16_t crc = modbusCRC( builder->frame, frameLength - 2 );

	memcpy(builder->frame + frameLength - 2, &crc, 2);

	status->request.length = frameLength;
	if ( address ) status->predictedResponseLength = 4 + 4;

	status->buildError = MODBUS_FERROR_OK;
	return MODBUS_ERROR_OK;
}
#endif
