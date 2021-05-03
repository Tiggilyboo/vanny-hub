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
	\brief Master's register-related frame building functions
*/

#ifndef LIGHTMODBUS_MBREGS_H
#define LIGHTMODBUS_MBREGS_H

#include <stdint.h>
#include "../libconf.h"
#include "../lightmodbus.h"
#include "../master.h"

//Functions for building requests
#if defined(LIGHTMODBUS_F03M) || defined(LIGHTMODBUS_F04M)
/**
	\brief Builds request 03 (read multiple holding registers) or 04 (read multiple input registers) frame
	\note Requires `F03M` or `F04M` module (see \ref building)
	\note `modbusParseRequest03` and `modbusParseRequest04` macros are aliases of this function
	\param status The master structure to load with the request frame
	\param function Function to be used in the request (03 or 04)
	\param address Address of slave to be requested
	\param index Address of the first register
	\param count Number of registers to be read
	\return A \ref ModbusError error code
*/
extern ModbusError modbusBuildRequest0304( ModbusMaster *status, uint8_t function, uint8_t address, uint16_t index, uint16_t count );

/**
	\brief Builds request 03 (read multiple holding registers) frame
	\note Requires `F03M` or `F04M` module (see \ref building)

	Calls \ref modbusBuildRequest0304 with 3 as the second parameter

	\param status The master structure to load with the request frame
	\param address Address of slave to be requested
	\param index Address of the first register
	\param count Number of registers to be read
	\return A \ref ModbusError error code
*/
static inline ModbusError modbusBuildRequest03( ModbusMaster *status, uint8_t address, uint16_t index, uint16_t count )
	{ return modbusBuildRequest0304( (status), 3, (address), (index), (count) ); }

/**
	\brief Builds request 04 (read multiple input registers) frame
	\note Requires `F03M` or `F04M` module (see \ref building)

	Calls \ref modbusBuildRequest0304 with 4 as the second parameter

	\param status The master structure to load with the request frame
	\param address Address of slave to be requested
	\param index Address of the first register
	\param count Number of registers to be read
	\return A \ref ModbusError error code
*/
static inline ModbusError modbusBuildRequest04( ModbusMaster *status, uint8_t address, uint16_t index, uint16_t count )
	{ return modbusBuildRequest0304( (status), 4, (address), (index), (count) ); }
#endif

#ifdef LIGHTMODBUS_F06M
/**
	\brief Builds request 06 (write a single holding register) frame
	\note Requires `F06M` module (see \ref building)
	\param status The master structure to load with the request frame
	\param address Address of slave to be requested
	\param index Address of the register
	\param value The value to be written
	\return A \ref ModbusError error code
*/
extern ModbusError modbusBuildRequest06( ModbusMaster *status, uint8_t address, uint16_t index, uint16_t value );
#endif

#ifdef LIGHTMODBUS_F16M
/**
	\brief Builds request 16 (write multiple holding registers) frame
	\note Requires `F16M` module (see \ref building)
	\param status The master structure to load with the request frame
	\param address Address of slave to be requested
	\param index Address of the first register
	\param count Number of registers to be written
	\param values The value to be written
	\return A \ref ModbusError error code
*/
extern ModbusError modbusBuildRequest16( ModbusMaster *status, uint8_t address, uint16_t index, uint16_t count, uint16_t *values );
#endif

#ifdef LIGHTMODBUS_F22M
/**
	\brief Builds request 22 (mask-write holding register) frame
	\note Requires `F22M` module (see \ref building)
	\param status The master structure to load with the request frame
	\param address Address of slave to be requested
	\param index Address of the  register
	\param andmask The AND mask value
	\param ormask The OR mask value
	\return A \ref ModbusError error code
*/
extern ModbusError modbusBuildRequest22( ModbusMaster *status, uint8_t address, uint16_t index, uint16_t andmask, uint16_t ormask );
#endif

#endif
