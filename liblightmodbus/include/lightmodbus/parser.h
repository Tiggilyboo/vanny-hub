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
	\brief Contains the \ref modbusParser union used during frame creation and parsing.

	\note This header file is suitable for C++
*/

#ifndef LIGHTMODBUS_PARSER_H
#define LIGHTMODBUS_PARSER_H

// For C++
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "libconf.h"


/**
	\brief A big union of structures used for parsing standard Modbus requests and building responses.
*/
typedef union modbusParser
{
	uint8_t frame[256];

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
	} base; //Base shared bytes, common for all frames, which always have the same meaning

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint8_t code;
		uint16_t crc;
	} exception;

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint16_t index;
		uint16_t count;
		uint16_t crc;
	} request0102; //Read multiple coils or discrete inputs

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint8_t length;
		uint8_t values[250];
		uint16_t crc;
	} response0102; //Read multiple coils or discrete inputs - response

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint16_t index;
		uint16_t count;
		uint16_t crc;
	} request0304; //Read multiple holding registers or input registers

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint8_t length;
		uint16_t values[125];
		uint16_t crc;
	} response0304; //Read multiple holding registers or input registers - response

	//TODO merge request 05 and 06
	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint16_t index;
		uint16_t value;
		uint16_t crc;
	} request05; //Write single coil

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint16_t index;
		uint16_t value;
		uint16_t crc;
	} response05; //Write single coil - response

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint16_t index;
		uint16_t value;
		uint16_t crc;
	} request06; //Write single holding register

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint16_t index;
		uint16_t value;
		uint16_t crc;
	} response06; //Write single holding register

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint16_t index;
		uint16_t count;
		uint8_t length;
		uint8_t values[246];
		uint16_t crc;
	} request15; //Write multiple coils

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint16_t index;
		uint16_t count;
		uint16_t crc;
	} response15; //Write multiple coils - response

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint16_t index;
		uint16_t count;
		uint8_t length;
		uint16_t values[123];
		uint16_t crc;
	} request16; //Write multiple holding registers

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint16_t index;
		uint16_t count;
		uint16_t crc;
	} response16; //Write multiple holding registers

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint16_t index;
		uint16_t andmask;
		uint16_t ormask;
		uint16_t crc;
	} request22; //Mask write single holding register

	struct __attribute__( ( __packed__ ) )
	{
		uint8_t address;
		uint8_t function;
		uint16_t index;
		uint16_t andmask;
		uint16_t ormask;
		uint16_t crc;
	} response22; //Mask write single holding register
} ModbusParser;

// For C++ (closes `extern "C"` )
#ifdef __cplusplus
}
#endif

#endif
