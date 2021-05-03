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
	\brief Slave's register-related built-in parsing functions
*/

#ifndef LIGHTMODBUS_SREGS_H
#define LIGHTMODBUS_SREGS_H

#include <stdint.h>
#include "../libconf.h"
#include "../lightmodbus.h"
#include "../slave.h"

//Functions for parsing requests
#if defined(LIGHTMODBUS_F03S) || defined(LIGHTMODBUS_F04S)
#define modbusParseRequest03 modbusParseRequest0304
#define modbusParseRequest04 modbusParseRequest0304
/**
	\brief Processes request 03 (read multiple holding registers) and 04 (read multiple input registers)
	\note Requires `F03S` or `F04S` module (see \ref building)
	\param status The slave structure to work with
	\param parser A parser structure containing request data
	\return A \ref ModbusError error code
*/
extern ModbusError modbusParseRequest0304( ModbusSlave *status, ModbusParser *parser );
#endif

#ifdef LIGHTMODBUS_F06S
/**
	\brief Processes request 06 (write multiple holding registers)
	\note Requires `F06S` module (see \ref building)
	\param status The slave structure to work with
	\param parser A parser structure containing request data
	\return A \ref ModbusError error code
*/
extern ModbusError modbusParseRequest06( ModbusSlave *status, ModbusParser *parser );
#endif

#ifdef LIGHTMODBUS_F16S
/**
	\brief Processes request 15 (write multiple holding registers)
	\note Requires `F16S` module (see \ref building)
	\param status The slave structure to work with
	\param parser A parser structure containing request data
	\return A \ref ModbusError error code
*/
extern ModbusError modbusParseRequest16( ModbusSlave *status, ModbusParser *parser );
#endif

#ifdef LIGHTMODBUS_F22S
/**
	\brief Processes request 22 (mask-write holding register)
	\note Requires `F22S` module (see \ref building)
	\param status The slave structure to work with
	\param parser A parser structure containing request data
	\return A \ref ModbusError error code
*/
extern ModbusError modbusParseRequest22( ModbusSlave *status, ModbusParser *parser );
#endif

#endif
