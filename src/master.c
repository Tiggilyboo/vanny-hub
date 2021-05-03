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
#include <lightmodbus/master.h>
#include <lightmodbus/lightmodbus.h>
#include <lightmodbus/parser.h>
#include <lightmodbus/master/mpregs.h>
#include <lightmodbus/master/mpcoils.h>

#ifdef LIGHTMODBUS_MASTER_BASE
ModbusError modbusParseException( ModbusMaster *status, ModbusParser *parser )
{
	//Parse exception frame and write data to MODBUSMaster structure

	//Check if given pointers are valid
	if ( status == NULL || parser == NULL ) return MODBUS_ERROR_NULLPTR;

	//Copy data (modbusParseResponse checked if length is 5 so it should be safe)
	status->exception.address = parser->exception.address;
	status->exception.function = parser->exception.function;
	status->exception.code = parser->exception.code;

	return MODBUS_ERROR_EXCEPTION;
}
#endif

#ifdef LIGHTMODBUS_MASTER_BASE
ModbusError modbusParseResponse( ModbusMaster *status )
{
	//This function parses response from master
	//Calling it will lead to losing all data and exceptions stored in MODBUSMaster (space will be reallocated)

	//Note: crc is now checked here

	//If non-zero some parser failed its job
	uint8_t err = 0;

	//Check if given pointer is valid
	if ( status == NULL ) return MODBUS_ERROR_NULLPTR;

	//Reset output registers before parsing frame
	status->exception.address = 0;
	status->exception.function = 0;
	status->exception.code = 0;

	#if !defined( LIGHTMODBUS_STATIC_MEM_MASTER_DATA ) && !defined( LIGHTMODBUS_NO_MASTER_DATA_BUFFER  )
		free( status->data.coils );
		status->data.coils = NULL;
		status->data.regs = NULL;
	#endif
	status->data.length = 0;
	status->data.index = 0;
	status->data.count = 0;
	status->data.type = 0;
	status->data.address = 0;
	status->data.function = 0;

	//Check if frames are not too short and return error (to avoid problems with memory allocation)
	//That enables us to omit the check in each parsing function
	if ( status->response.length < 4u || status->response.frame == NULL || \
		status->request.length < 4u || status->request.frame == NULL )
		{
			status->parseError = MODBUS_FERROR_LENGTH;
			return MODBUS_ERROR_PARSE;
		}

	//Check both response and request frames CRC
	//The CRC of the frames are copied to a variable in order to avoid an unaligned memory access,
	//which can cause runtime errors in some platforms like AVR and ARM.
	uint16_t crcresp;
	uint16_t crcreq;

	memcpy(&crcresp, status->response.frame + status->response.length - 2, 2);
	memcpy(&crcreq,	 status->request.frame	+ status->request.length  - 2, 2);

	if ( crcresp != modbusCRC( status->response.frame, status->response.length - 2 ) ||
		 crcreq	 != modbusCRC( status->request.frame,  status->request.length  - 2 ) )
	{
		status->parseError = MODBUS_FERROR_CRC;
		return MODBUS_ERROR_PARSE;
	}

	ModbusParser *parser = (ModbusParser*) status->response.frame;
	ModbusParser *requestParser = (ModbusParser*) status->request.frame;

	uint8_t functionMatch = 0, functionExec = 0;
	status->parseError = MODBUS_FERROR_OK;

	//Check user defined functions
	#ifdef LIGHTMODBUS_MASTER_USER_FUNCTIONS
		if ( status->userFunctions != NULL )
		{
			uint16_t i;
			for ( i = 0; i < status->userFunctionCount; i++ )
			{
				if ( status->userFunctions[i].function == parser->base.function )
				{
					functionMatch = 1;

					//If the function is overriden and handler pointer is valid, user the callback
					if ( status->userFunctions[i].handler != NULL )
					{
						err = status->userFunctions[i].handler( status, parser, requestParser );
						functionExec = 1;
					}
					else
						functionExec = 0; //Function overriden, but pointer is invalid

					//Search till first match
					break;
				}
			}
		}
	#endif

	if ( !functionMatch )
	{
		functionExec = 1;

		//Catching exceptions can be overriden by user functions
		if ( ( parser->base.function & 128 ) && status->response.length == 5 )
		{
			err = modbusParseException( status, parser );
		}
		else
		{
			switch ( parser->base.function )
			{
				#if defined(LIGHTMODBUS_F01M) || defined(LIGHTMODBUS_F02M)
					case 1: //Read multiple coils
					case 2: //Read multiple discrete inputs
						err = modbusParseResponse0102( status, parser, requestParser );
						break;
				#endif

				#if defined(LIGHTMODBUS_F03M) || defined(LIGHTMODBUS_F04M)
					case 3: //Read multiple holding registers
					case 4: //Read multiple input registers
						err = modbusParseResponse0304( status, parser, requestParser );
						break;
				#endif

				#ifdef LIGHTMODBUS_F05M
					case 5: //Write single coil
						err = modbusParseResponse05( status, parser, requestParser );
						break;
				#endif

				#ifdef LIGHTMODBUS_F06M
					case 6: //Write single holding reg
						err = modbusParseResponse06( status, parser, requestParser );
						break;
				#endif

				#ifdef LIGHTMODBUS_F15M
					case 15: //Write multiple coils
						err = modbusParseResponse15( status, parser, requestParser );
						break;
				#endif

				#ifdef LIGHTMODBUS_F16M
					case 16: //Write multiple holding registers
						err = modbusParseResponse16( status, parser, requestParser );
						break;
				#endif

				#ifdef LIGHTMODBUS_F22M
					case 22: //Mask write holding register
						err = modbusParseResponse22( status, parser, requestParser );
						break;
				#endif

				default: //Function code not known by master
					functionExec = 0;
					break;
			}
		}
	}

	//Function not executed
	if ( !functionExec )
	{
		if ( functionMatch )
			status->parseError = MODBUS_FERROR_NULLFUN; //User override
		else
			status->parseError = MODBUS_FERROR_NOFUN; //Unsupported function

		return MODBUS_ERROR_PARSE;
	}


	return err;
}
#endif

#ifdef LIGHTMODBUS_MASTER_BASE
ModbusError modbusMasterInit( ModbusMaster *status )
{
	//Check if given pointer is valid
	if ( status == NULL ) return MODBUS_ERROR_NULLPTR;

	//Very basic init of master side
	#ifndef LIGHTMODBUS_STATIC_MEM_MASTER_REQUEST
		status->request.frame = NULL;
	#else
		memset( status->request.frame, 0, LIGHTMODBUS_STATIC_MEM_MASTER_REQUEST );
	#endif
	status->request.length = 0;

	#ifndef LIGHTMODBUS_STATIC_MEM_MASTER_RESPONSE
		status->response.frame = NULL;
	#else
		memset( status->response.frame, 0, LIGHTMODBUS_STATIC_MEM_MASTER_RESPONSE );
	#endif
	status->response.length = 0;

	#ifndef LIGHTMODBUS_STATIC_MEM_MASTER_DATA
		status->data.coils = NULL;
		status->data.regs = NULL;
	#else
		memset( status->data.coils, 0, LIGHTMODBUS_STATIC_MEM_MASTER_DATA );
	#endif
	status->data.length = 0;

	status->data.count = 0;
	status->data.index = 0;
	status->data.type = 0;
	status->data.address = 0;

	status->exception.address = 0;
	status->exception.function = 0;
	status->exception.code = 0;

	return MODBUS_ERROR_OK;
}
#endif

#ifdef LIGHTMODBUS_MASTER_BASE
ModbusError modbusMasterEnd( ModbusMaster *status )
{
	//Check if given pointer is valid
	if ( status == NULL ) return MODBUS_ERROR_NULLPTR;

	//Free memory
	#ifndef LIGHTMODBUS_STATIC_MEM_MASTER_REQUEST
		free( status->request.frame );
		status->request.frame = NULL;
	#endif
	#if !defined( LIGHTMODBUS_STATIC_MEM_MASTER_DATA ) && !defined( LIGHTMODBUS_NO_MASTER_DATA_BUFFER  )
		free( status->data.coils );
		status->data.coils = NULL;
		status->data.regs = NULL;
	#endif

	return MODBUS_ERROR_OK;
}
#endif
