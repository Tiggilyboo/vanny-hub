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
	\brief Master's register-related parsing functions
*/

#ifndef LIGHTMODBUS_MPREGS_H
#define LIGHTMODBUS_MPREGS_H

#include <stdint.h>
#include "../libconf.h"
#include "../lightmodbus.h"
#include "../master.h"

//Functions for parsing responses
#if defined(LIGHTMODBUS_F03M) || defined(LIGHTMODBUS_F04M)
#define modbusParseResponse03 modbusParseResponse0304
#define modbusParseResponse04 modbusParseResponse0304
/**
	\brief Processes responses for requests 03 (read multiple holding registers) and 04 (read multiple input registers)
	\note Requires `F03M` or `F04M` module (see \ref building)
	\note `modbusParseResponse03` and `modbusParseResponse04` macros are aliases of this function
	\param status The master structure to work with
	\param parser A parser structure containing response data
	\param requestParser A parser structure containing request data
	\return A \ref ModbusError error code
*/
extern ModbusError modbusParseResponse0304( ModbusMaster *status, ModbusParser *parser, ModbusParser *requestParser );
#endif

#ifdef LIGHTMODBUS_F06M
/**
	\brief Processes responses for request 06 (write a single holding register)
	\note Requires `F06M` module (see \ref building)
	\param status The master structure to work with
	\param parser A parser structure containing response data
	\param requestParser A parser structure containing request data
	\return A \ref ModbusError error code
*/
extern ModbusError modbusParseResponse06( ModbusMaster *status, ModbusParser *parser, ModbusParser *requestParser );
#endif

#ifdef LIGHTMODBUS_F16M
/**
	\brief Processes responses for request 15 (write multiple holding registers)
	\note Requires `F16M` module (see \ref building)
	\param status The master structure to work with
	\param parser A parser structure containing response data
	\param requestParser A parser structure containing request data
	\return A \ref ModbusError error code
*/
extern ModbusError modbusParseResponse16( ModbusMaster *status, ModbusParser *parser, ModbusParser *requestParser );
#endif

#ifdef LIGHTMODBUS_F22M
/**
	\brief Processes responses for request 22 (mask-write holding register)
	\note Requires `F22M` module (see \ref building)
	\param status The master structure to work with
	\param parser A parser structure containing response data
	\param requestParser A parser structure containing request data
	\return A \ref ModbusError error code
*/
extern ModbusError modbusParseResponse22( ModbusMaster *status, ModbusParser *parser, ModbusParser *requestParser );
#endif

#endif
