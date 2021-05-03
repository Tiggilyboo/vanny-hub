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
#include <lightmodbus/master/mbregs.h>

#if defined(LIGHTMODBUS_F03M) || defined(LIGHTMODBUS_F04M)
ModbusError modbusBuildRequest0304( ModbusMaster *status, uint8_t function, uint8_t address, uint16_t index, uint16_t count )
{
	//Build request03 frame, to send it so slave
	//Read multiple holding registers

	//Set frame length
	uint8_t frameLength = 8;

	//Check if given pointer is valid
	if ( status == NULL ) return MODBUS_ERROR_NULLPTR;
	if ( function != 3 && function != 4 )
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

	//Check values pointer
	if ( count == 0 || count > 125 )
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
	#endif

	ModbusParser *builder = (ModbusParser *) status->request.frame;

	builder->base.address = address;
	builder->base.function = function;
	builder->request0304.index = modbusMatchEndian( index );
	builder->request0304.count = modbusMatchEndian( count );

	//Calculate crc
	builder->request0304.crc = modbusCRC( builder->frame, frameLength - 2 );

	status->request.length = frameLength;
	status->predictedResponseLength = 4 + 1 + ( count << 1 );
	status->buildError = MODBUS_FERROR_OK;
	return MODBUS_ERROR_OK;
}
#endif

#ifdef LIGHTMODBUS_F06M
ModbusError modbusBuildRequest06( ModbusMaster *status, uint8_t address, uint16_t index, uint16_t value )
{
	//Build request06 frame, to send it so slave
	//Write single holding reg

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
	#endif

	ModbusParser *builder = (ModbusParser *) status->request.frame;

	builder->base.address = address;
	builder->base.function = 6;
	builder->request06.index = modbusMatchEndian( index );
	builder->request06.value = modbusMatchEndian( value );

	//Calculate crc
	builder->request06.crc = modbusCRC( builder->frame, frameLength - 2 );

	status->request.length = frameLength;
	if ( address ) status->predictedResponseLength = 8;
	status->buildError = MODBUS_FERROR_OK;
	return MODBUS_ERROR_OK;
}
#endif

#ifdef LIGHTMODBUS_F16M
ModbusError modbusBuildRequest16( ModbusMaster *status, uint8_t address, uint16_t index, uint16_t count, uint16_t *values )
{
	//Build request16 frame, to send it so slave
	//Write multiple holding registers

	//Set frame length
	uint8_t frameLength = 9 + ( count << 1 );
	uint8_t i = 0;

	//Check if given pointers are valid
	if ( status == NULL || values == NULL ) return MODBUS_ERROR_NULLPTR;

	//Set output frame length to 0 (in case of interrupts)
	status->request.length = 0;
	status->predictedResponseLength = 0;

	//Check values pointer
	if ( count == 0 || count > 123 )
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
	#endif

	ModbusParser *builder = (ModbusParser *) status->request.frame;

	builder->base.address = address;
	builder->base.function = 16;
	builder->request16.index = modbusMatchEndian( index );
	builder->request16.count = modbusMatchEndian( count );
	builder->request16.length = count << 1;

	for ( i = 0; i < count; i++ )
		builder->request16.values[i] = modbusMatchEndian( values[i] );

	builder->request16.values[count] = modbusCRC( builder->frame, frameLength - 2 );

	status->request.length = frameLength;
	if ( address ) status->predictedResponseLength = 4 + 4;

	status->buildError = MODBUS_FERROR_OK;
	return MODBUS_ERROR_OK;
}
#endif

#ifdef LIGHTMODBUS_F22M
ModbusError modbusBuildRequest22( ModbusMaster *status, uint8_t address, uint16_t index, uint16_t andmask, uint16_t ormask )
{
	//Build request22 frame, to send it so slave
	//Mask write single holding reg

	//Set frame length
	uint8_t frameLength = 10;

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
	#endif

	ModbusParser *builder = (ModbusParser *) status->request.frame;

	builder->base.address = address;
	builder->base.function = 22;
	builder->request22.index = modbusMatchEndian( index );
	builder->request22.andmask = modbusMatchEndian( andmask );
	builder->request22.ormask = modbusMatchEndian( ormask );

	//Calculate crc
	builder->request22.crc = modbusCRC( builder->frame, frameLength - 2 );

	status->request.length = frameLength;
	if ( address ) status->predictedResponseLength = 10;
	status->buildError = MODBUS_FERROR_OK;
	return MODBUS_ERROR_OK;
}
#endif
