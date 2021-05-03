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
	\brief Slave's coil-related requests built-in parsing functions
*/

#ifndef LIGHTMODBUS_SCOILS_H
#define LIGHTMODBUS_SCOILS_H

#include <stdint.h>
#include "../libconf.h"
#include "../lightmodbus.h"
#include "../slave.h"

//Functions for parsing requests
#if defined(LIGHTMODBUS_F01S) || defined(LIGHTMODBUS_F02S)
#define modbusParseRequest01 modbusParseRequest0102
#define modbusParseRequest02 modbusParseRequest0102
/**
	\brief Processes requests 01 (read multiple coils) and 02 (read multiple discrete inputs).
	\note Requires `F01S` or `F02S` module (see \ref building)
	\param status The slave structure to work with
	\param parser A parser structure containing request data
	\return A \ref ModbusError error code
*/
extern ModbusError modbusParseRequest0102( ModbusSlave *status, ModbusParser *parser );
#endif

#ifdef LIGHTMODBUS_F05S
/**
	\brief Processes request 05 (write a single coil)
	\note Requires `F05S` module (see \ref building)
	\param status The slave structure to work with
	\param parser A parser structure containing request data
	\return A \ref ModbusError error code
*/
extern ModbusError modbusParseRequest05( ModbusSlave *status, ModbusParser *parser );
#endif

#ifdef LIGHTMODBUS_F15S
/**
	\brief Processes request 15 (write multiple coils)
	\note Requires `F15S` module (see \ref building)
	\param status The slave structure to work with
	\param parser A parser structure containing request data
	\return A \ref ModbusError error code
*/
extern ModbusError modbusParseRequest15( ModbusSlave *status, ModbusParser *parser );
#endif

#endif
