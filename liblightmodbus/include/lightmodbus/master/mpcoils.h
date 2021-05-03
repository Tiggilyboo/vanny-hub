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
	\brief Master's coil-related parsing functions
*/

#ifndef LIGHTMODBUS_MPCOILS_H
#define LIGHTMODBUS_MPCOILS_H

#include <stdint.h>
#include "../libconf.h"
#include "../lightmodbus.h"
#include "../master.h"

//Functions for parsing responses
#if defined(LIGHTMODBUS_F01M) || defined(LIGHTMODBUS_F02M)
#define modbusParseResponse01 modbusParseResponse0102
#define modbusParseResponse02 modbusParseResponse0102
/**
	\brief Processes responses for requests 01 (read multiple coils) and 02 (read multiple discrete inputs)
	\note Requires `F01M` or `F02M` module (see \ref building)
	\note `modbusParseResponse01` and `modbusParseResponse02` macros are aliases of this function
	\param status The master structure to work with
	\param parser A parser structure containing response data
	\param requestParser A parser structure containing request data
	\return A \ref ModbusError error code
*/
extern ModbusError modbusParseResponse0102( ModbusMaster *status, ModbusParser *parser, ModbusParser *requestParser );
#endif

#ifdef LIGHTMODBUS_F05M
/**
	\brief Processes responses for request 05 (write a single coil)
	\note Requires `F05M` module (see \ref building)
	\param status The master structure to work with
	\param parser A parser structure containing response data
	\param requestParser A parser structure containing request data
	\return A \ref ModbusError error code
*/
extern ModbusError modbusParseResponse05( ModbusMaster *status, ModbusParser *parser, ModbusParser *requestParser );
#endif

#ifdef LIGHTMODBUS_F15M
/**
	\brief Processes responses for request 15 (write multiple coils)
	\note Requires `F15M` module (see \ref building)
	\param status The master structure to work with
	\param parser A parser structure containing response data
	\param requestParser A parser structure containing request data
	\return A \ref ModbusError error code
*/
extern ModbusError modbusParseResponse15( ModbusMaster *status, ModbusParser *parser,  ModbusParser *requestParser );
#endif

#endif
