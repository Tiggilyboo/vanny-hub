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
	\brief Master's coil-related frame building functions
*/

#ifndef LIGHTMODBUS_MBCOILS_H
#define LIGHTMODBUS_MBCOILS_H

#include <stdint.h>
#include "../libconf.h"
#include "../lightmodbus.h"
#include "../master.h"

//Functions for building requests
#if defined(LIGHTMODBUS_F01M) || defined(LIGHTMODBUS_F02M)
/**
	\brief Builds request 01 (read multiple coils) or 02 (read multiple discrete inputs) frame
	\note Requires `F01M` or `F02M` module (see \ref building)
	\note `modbusParseRequest01` and `modbusParseRequest02` macros are aliases of this function
	\param status The master structure to load with the request frame
	\param function Function to be used in the request (01 or 02)
	\param address Address of slave to be requested
	\param index Address of the first discrete input/coil
	\param count Number of coils to be read
	\return A \ref ModbusError error code
*/
extern ModbusError modbusBuildRequest0102( ModbusMaster *status, uint8_t function, uint8_t address, uint16_t index, uint16_t count );

/**
	\brief Builds request 01 (read multiple coils) frame
	\note Requires `F01M` or `F02M` module (see \ref building)


	Calls \ref modbusBuildRequest0102 with 1 as the second parameter

	\param status The master structure to load with the request frame
	\param address Address of slave to be requested
	\param index Address of the first coil
	\param count Number of coils to be read
	\return A \ref ModbusError error code
*/
static inline ModbusError modbusBuildRequest01( ModbusMaster *status, uint8_t address, uint16_t index, uint16_t count )
	{ return modbusBuildRequest0102( (status), 1, (address), (index), (count) ); }

/**
	\brief Builds request 02 (read multiple discrete inputs) frame
	\note Requires `F01M` or `F02M` module (see \ref building)

	Calls \ref modbusBuildRequest0102 with 2 as the second parameter

	\param status The master structure to load with the request frame
	\param address Address of slave to be requested
	\param index Address of the first discrete input
	\param count Number of discrete inputs to be read
	\return A \ref ModbusError error code
*/
static inline ModbusError modbusBuildRequest02( ModbusMaster *status, uint8_t address, uint16_t index, uint16_t count )
	{ return modbusBuildRequest0102( (status), 2, (address), (index), (count) ); }
#endif

#ifdef LIGHTMODBUS_F05M
/**
	\brief Builds request 05 (write a single coil) frame
	\note Requires `F01M` or `F02M` module (see \ref building)

	\param status The master structure to load with the request frame
	\param address Address of slave to be requested
	\param index Address of the coil
	\param value Coil value to be written
	\return A \ref ModbusError error code
*/
extern ModbusError modbusBuildRequest05( ModbusMaster *status, uint8_t address, uint16_t index, uint16_t value );
#endif

#ifdef LIGHTMODBUS_F15M
/**
	\brief Builds request 15 (write multiple coils) frame
	\note Requires `LIGHTMODBUS_F15M` macro to be defined
	\note Requires `F05M` module (see \ref building)

	\param status The master structure to load with the request frame
	\param address Address of slave to be requested
	\param index Address of the first coil
	\param count Number of coils to be written
	\param values Coil values to be written (each bit corresponds to one coil)
	\return A \ref ModbusError error code
*/
extern ModbusError modbusBuildRequest15( ModbusMaster *status, uint8_t address, uint16_t index, uint16_t count, uint8_t *values );
#endif

#endif
