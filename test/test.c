/*
This is really simple test suite, it covers ~95% of library code
AKA. The Worst Test File Ever
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include <lightmodbus/lightmodbus.h>
#include <lightmodbus/master.h>
#include <lightmodbus/slave.h>
#include <lightmodbus/addons/examine.h>

#define DUMPMF( ) printf( "Dump the frame:\n\t" ); for ( i = 0; i < mstatus.request.length; i++ ) printf( "%.2x%s", mstatus.request.frame[i], ( i == mstatus.request.length - 1 ) ? "\n" : "-" );
#define DUMPSF( ) printf( "Dump response - length = %d:\n\t", sstatus.response.length ); for ( i = 0; i < sstatus.response.length; i++ ) printf( "%x%s", sstatus.response.frame[i], ( i == sstatus.response.length - 1 ) ? "\n" : ", " );

ModbusMaster mstatus;
struct modbusSlave sstatus; //This can be done this way as well
uint16_t registers[8] = { 0, 1, 2, 3, 4, 5, 6, 7 };
uint8_t coils[4] = { 0, 0, 0, 0 };
uint8_t discreteInputs[2] = { 255, 0 };
uint16_t inputRegisters[4] = { 1, 2, 3, 4 };
uint16_t defaults[512] = { 0xdead, 0xface, 0x1570, 0x01 };
uint16_t TestValues[512] = { 0xface, 0xdead, 0xCC, 0xDD, 0xEE, 0xFF, 0xAAFF, 0xBBFF };
uint16_t TestValues2[512] = {0};
uint8_t TestValues3[512] = { 0xCC, 0x00 };
uint8_t ec;

void TermRGB( unsigned char R, unsigned char G, unsigned char B )
{
	if ( R > 5u || G > 5u || B > 5u ) return;
	printf( "\033[38;5;%dm", 16 + B + G * 6 + R * 36 );
}

const char *modbuserrstr( ModbusError err )
{
	static const char *str[]=
	{
		"MODBUS_ERROR_OK",
		"MODBUS_ERROR_EXCEPTION",
		"MODBUS_ERROR_ALLOC",
		"MODBUS_ERROR_OTHER",
		"MODBUS_ERROR_NULLPTR",
		"MODBUS_ERROR_PARSE",
		"MODBUS_ERROR_BUILD",
	};

	if ( err < sizeof(str) / sizeof(str[0]) )
		return str[err];
	else return NULL;
}

const char *modbusferrstr( ModbusFrameError err )
{
	static const char *str[]=
	{
		"MODBUS_FERROR_OK",
		"MODBUS_FERROR_CRC",
		"MODBUS_FERROR_LENGTH",
		"MODBUS_FERROR_COUNT",
		"MODBUS_FERROR_VALUE",
		"MODBUS_FERROR_RANGE",
		"MODBUS_FERROR_NOSRC",
		"MODBUS_FERROR_NOREAD",
		"MODBUS_FERROR_NOWRITE",
		"MODBUS_FERROR_NOFUN",
		"MODBUS_FERROR_BADFUN",
		"MODBUS_FERROR_NULLFUN",
		"MODBUS_FERROR_MISM_FUN",
		"MODBUS_FERROR_MISM_ADDR",
		"MODBUS_FERROR_MISM_INDEX",
		"MODBUS_FERROR_MISM_COUNT",
		"MODBUS_FERROR_MISM_VALUE",
		"MODBUS_FERROR_MISM_MASK",
		"MODBUS_FERROR_BROADCAST",
	};

	if ( err < sizeof(str) / sizeof(str[0]) )
		return str[err];
	else return NULL;
}

//User defined function
ModbusError userModbusFunction( ModbusSlave *status, ModbusParser *parser )
{
	modbusBuildException( status, 78, MODBUS_EXCEP_ACK );
	printf( "<<Hello, this is the parsing function. I'm alive and working>>\n" );
	return MODBUS_OK;
}
ModbusError userModbusMasterFunction( ModbusMaster *status, ModbusParser *parser, ModbusParser *rqp )
{
	printf( "<<And I am the master parsing function!>>\n" );
	return MODBUS_OK;
}

//Register callback function
#if defined(LIGHTMODBUS_REGISTER_CALLBACK) || defined(LIGHTMODBUS_COIL_CALLBACK)
int dummy_ctx_var = 567;
uint16_t reg_callback( ModbusRegisterQuery query, ModbusDataType datatype, uint16_t index, uint16_t value, void* ctx )
{
	//Assure the context doesn't change
	assert( ctx == (void*) &dummy_ctx_var );

	//All can be written and read
	if ( query == MODBUS_REGQ_R_CHECK || query == MODBUS_REGQ_W_CHECK ) return 1;

	//Read
	if ( query == MODBUS_REGQ_R )
	{
		switch( datatype )
		{
			case MODBUS_HOLDING_REGISTER:
				return registers[index];

			case MODBUS_INPUT_REGISTER:
				return inputRegisters[index];

			case MODBUS_COIL:
				return modbusMaskRead( coils, sizeof( coils ) * 8, index );

			case MODBUS_DISCRETE_INPUT:
				return modbusMaskRead( discreteInputs, sizeof( discreteInputs ) * 8, index );

			default:
				return 0;
		}
	}

	//Write
	if ( query == MODBUS_REGQ_W )
	{
		switch( datatype )
		{
			case MODBUS_HOLDING_REGISTER:
				registers[index] = value;
				break;

			case MODBUS_COIL:
				modbusMaskWrite( coils, sizeof( coils ) * 8, index, value != 0 );
				break;

			default:
				assert( 0 && "tried to write input register or discrete input" );
				break;
		}
	}

	return 0;
}
#endif

void examinedump( struct modbusFrameInfo info )
{
	int i;

	printf( "Frame examine:\n"\
			"\tdirection: %s (%d)\n"\
 			"\taddress: %d\n"\
			"\tfunction: %d\n"\
			"\texception: %d\n"\
			"\ttype: %s (%d)\n"\
			"\tindex: %d\n"\
			"\tcount: %d\n"\
			"\taccess: %s (%d)\n"\
			"\tcrc: %04x\n"\
			"\tandmask: %04x\n"\
			"\tormask: %04x\n"\
			//"\tdataptr: ---discarded, to make output comaprison easier--\n"
			"\tdatalen: %d\n"\
			"\tdata: ",\

			info.direction == MODBUS_EXAMINE_REQUEST ? "request" : ( info.direction == MODBUS_EXAMINE_RESPONSE ? "response" : "bad" ),\
			info.direction,\
			info.address,\
			info.function,\
			info.exception,\
			info.type  == MODBUS_HOLDING_REGISTER ? "reg" : ( info.type == MODBUS_COIL ? "coil" : ( info.type == MODBUS_DISCRETE_INPUT ? "discrete" : ( info.type == MODBUS_INPUT_REGISTER ? "input reg" : "bad"))),\
			info.type,\
			info.index,\
			info.count,\
			info.access == MODBUS_EXAMINE_READ ? "read" : ( info.access == MODBUS_EXAMINE_WRITE ? "write" : "bad" ),\
			info.access,\
			info.crc,\
			info.andmask,\
			info.ormask,\
			//info.data,
			info.length\
		);

	if ( info.data != NULL )
	{
		for ( i = 0; i < info.length; i++ )
		{
			printf( "%02x ", ((uint8_t*)info.data)[i] );
		}
	}
	printf( "\n" );
}

void examinem( )
{
	struct modbusFrameInfo info;
	uint8_t err = modbusExamine( &info, MODBUS_EXAMINE_REQUEST, mstatus.request.frame, mstatus.request.length );
	if ( err == MODBUS_OK || err == MODBUS_ERROR_EXCEPTION ) examinedump( info );
	else printf( "request frame examination error: %s\n", modbuserrstr(err) );
}

void examines( )
{
	struct modbusFrameInfo info;
	uint8_t err = modbusExamine( &info, MODBUS_EXAMINE_RESPONSE, sstatus.response.frame, sstatus.response.length );
	if ( err == MODBUS_OK || err == MODBUS_ERROR_EXCEPTION ) examinedump( info );
	else printf( "response frame examination error: %s\n", modbuserrstr(err) );
}

#if !defined(LIGHTMODBUS_REGISTER_CALLBACK) && !defined(LIGHTMODBUS_COIL_CALLBACK)
void maxlentest( )
{
	#define CK2( n ) printf( "mec=%d, sec=%d\n", mec, sec ); printf( memcmp( mstatus.data.regs, bak, mstatus.data.length ) ? "ERROR!\n" : "OK\n" );
	#define CK( n ) printf( "mec=%d, sec=%d\n", mec, sec ); printf( memcmp( vals, bak, n ) ? "ERROR!\n" : "OK\n" );
	#define GEN( n ) for ( i = 0; i < n; i++ ) bak[i] = rand( );
	uint8_t vals[250];
	uint8_t bak[250];
	uint8_t i = 0;
	uint8_t sec, mec;

	printf( "\n-------Checking max-size operations--------\n" );
	sstatus.coils = vals;
	sstatus.coilCount = 2000;
	sstatus.coilMaskLength = 0;
	sstatus.discreteInputs = bak;
	sstatus.discreteInputCount = 2000;
	sstatus.coilMaskLength = 0;

	sstatus.registerCount = 125;
	sstatus.inputRegisterCount = 125;
	sstatus.registerMaskLength = 0;
	sstatus.registers = (uint16_t*) vals;
	sstatus.inputRegisters = (uint16_t*) bak;

	GEN( 246 );
	mec = modbusBuildRequest15( &mstatus, 0x20, 0, 1968, bak );
	#ifdef LIGHTMODBUS_STATIC_MEM_SLAVE_REQUEST
		memcpy( sstatus.request.frame, mstatus.request.frame, mstatus.request.length );
	#else
		sstatus.request.frame = mstatus.request.frame;
	#endif
	sstatus.request.length = mstatus.request.length;
	sec = modbusParseRequest( &sstatus );
	CK( 246 );

	GEN( 246 );
	mec = modbusBuildRequest16( &mstatus, 0x20, 0, 123, (uint16_t*)bak );
	#ifdef LIGHTMODBUS_STATIC_MEM_SLAVE_REQUEST
		memcpy( sstatus.request.frame, mstatus.request.frame, mstatus.request.length );
	#else
		sstatus.request.frame = mstatus.request.frame;
	#endif
	sstatus.request.length = mstatus.request.length;
	sec = modbusParseRequest( &sstatus );
	CK( 246 );

	sstatus.coils = bak;
	sstatus.registers = (uint16_t*) bak;

	GEN( 250 );
	mec = modbusBuildRequest03( &mstatus, 0x20, 0, 125 );
	#ifdef LIGHTMODBUS_STATIC_MEM_SLAVE_REQUEST
		memcpy( sstatus.request.frame, mstatus.request.frame, mstatus.request.length );
	#else
		sstatus.request.frame = mstatus.request.frame;
	#endif
	sstatus.request.length = mstatus.request.length;
	sec = modbusParseRequest( &sstatus );

	#ifdef LIGHTMODBUS_STATIC_MEM_MASTER_RESPONSE
		memcpy( mstatus.response.frame, sstatus.response.frame, sstatus.response.length );
	#else
		mstatus.response.frame = sstatus.response.frame;
	#endif
	mstatus.response.length = sstatus.response.length;
	mec = modbusParseResponse( &mstatus );
	CK2( 250 );

	GEN( 250 );
	mec = modbusBuildRequest04( &mstatus, 0x20, 0, 125 );
	#ifdef LIGHTMODBUS_STATIC_MEM_SLAVE_REQUEST
		memcpy( sstatus.request.frame, mstatus.request.frame, mstatus.request.length );
	#else
		sstatus.request.frame = mstatus.request.frame;
	#endif
	sstatus.request.length = mstatus.request.length;
	sec = modbusParseRequest( &sstatus );
	#ifdef LIGHTMODBUS_STATIC_MEM_MASTER_RESPONSE
		memcpy( mstatus.response.frame, sstatus.response.frame, sstatus.response.length );
	#else
		mstatus.response.frame = sstatus.response.frame;
	#endif
	mstatus.response.length = sstatus.response.length;
	mec = modbusParseResponse( &mstatus );
	CK2( 250 );

	GEN( 250 );
	mec = modbusBuildRequest01( &mstatus, 0x20, 0, 2000 );
	#ifdef LIGHTMODBUS_STATIC_MEM_SLAVE_REQUEST
		memcpy( sstatus.request.frame, mstatus.request.frame, mstatus.request.length );
	#else
		sstatus.request.frame = mstatus.request.frame;
	#endif
	sstatus.request.length = mstatus.request.length;
	sec = modbusParseRequest( &sstatus );
	DUMPSF( );
	#ifdef LIGHTMODBUS_STATIC_MEM_MASTER_RESPONSE
		memcpy( mstatus.response.frame, sstatus.response.frame, sstatus.response.length );
	#else
		mstatus.response.frame = sstatus.response.frame;
	#endif
	mstatus.response.length = sstatus.response.length;
	mec = modbusParseResponse( &mstatus );
	CK2( 250 );

	GEN( 250 );
	mec = modbusBuildRequest02( &mstatus, 0x20, 0, 2000 );
	#ifdef LIGHTMODBUS_STATIC_MEM_SLAVE_REQUEST
		memcpy( sstatus.request.frame, mstatus.request.frame, mstatus.request.length );
	#else
		sstatus.request.frame = mstatus.request.frame;
	#endif
	sstatus.request.length = mstatus.request.length;
	sec = modbusParseRequest( &sstatus );
	#ifdef LIGHTMODBUS_STATIC_MEM_MASTER_RESPONSE
		memcpy( mstatus.response.frame, sstatus.response.frame, sstatus.response.length );
	#else
		mstatus.response.frame = sstatus.response.frame;
	#endif
	mstatus.response.length = sstatus.response.length;
	mec = modbusParseResponse( &mstatus );
	CK2( 250 );

}
#endif

void Test( )
{
	uint8_t i = 0;
	uint8_t SlaveError, MasterError;
	uint8_t mok, sok;

	//Clear registers
	//memset( registers, 0, 8 * 2 );
	//memset( inputRegisters, 0, 4 * 2 );
	memset( coils, 0, 4 );
	memset( discreteInputs, 0, 2 );
	memcpy( registers, defaults, 16 );
	memcpy( inputRegisters, defaults, 4 * 2 );
	//memcpy( coils, defaults, 4 );
	//memcpy( discreteInputs, defaults, 2 );

	//Start test
	//TermRGB( 4, 2, 0 );
	printf( "Master build errors - %s\n", modbusferrstr(mstatus.buildError) );
	printf( "Test started...\n" );

	//Dump frame
	//TermRGB( 2, 4, 0 );
	printf( "Dump the frame:\n\t" );
	for ( i = 0; i < mstatus.request.length; i++ )
		printf( "%.2x%s", mstatus.request.frame[i], ( i == mstatus.request.length - 1 ) ? "\n" : "-" );

	//Dump slave registers
	//TermRGB( 2, 4, 2 );
	printf( "Dump registers:\n\t" );
	for ( i = 0; i < sstatus.registerCount; i++ )
		printf( "0x%.2x%s", registers[i], ( i == sstatus.registerCount - 1 ) ? "\n" : ", " );

	printf( "Dump input registers:\n\t" );
	for ( i = 0; i < sstatus.inputRegisterCount; i++ )
		printf( "0x%.2x%s", inputRegisters[i], ( i == sstatus.inputRegisterCount - 1 ) ? "\n" : ", " );

	printf( "Dump coils:\n\t" );
	for ( i = 0; i < sstatus.coilCount; i++ )
		printf( "%d%s", modbusMaskRead( coils, sstatus.coilCount, i ), ( i == sstatus.coilCount - 1 ) ? "\n" : ", " );

	printf( "Dump discrete inputs:\n\t" );
	for ( i = 0; i < sstatus.discreteInputCount; i++ )
		printf( "%d%s", modbusMaskRead( discreteInputs, sstatus.discreteInputCount, i ), ( i == sstatus.discreteInputCount - 1 ) ? "\n" : ", " );

	//Parse request

	uint8_t f1[256];
	uint8_t l = mstatus.request.length;
	memcpy( f1, mstatus.request.frame, l );

	printf( "Let slave parse frame...\n" );
	#ifdef LIGHTMODBUS_STATIC_MEM_SLAVE_REQUEST
		memcpy( sstatus.request.frame, mstatus.request.frame, mstatus.request.length );
	#else
		sstatus.request.frame = mstatus.request.frame;
	#endif
	sstatus.request.length = mstatus.request.length;
	sok = SlaveError = modbusParseRequest( &sstatus );
	printf( "\tError - %s\n\tFinished - %d\n", modbuserrstr(SlaveError), 1 );
	printf( "\tparseError - %s\n", modbusferrstr(SlaveError != MODBUS_OK ? sstatus.parseError : 0) );

	if ( memcmp( f1, mstatus.request.frame, l ) ) printf( "!!!Slave has malformed the frame!!!\n" );

	//Dump slave registers
	printf( "Dump registers:\n\t" );
	for ( i = 0; i < sstatus.registerCount; i++ )
		printf( "0x%.2x%s", registers[i], ( i == sstatus.registerCount - 1 ) ? "\n" : ", " );

	printf( "Dump input registers:\n\t" );
	for ( i = 0; i < sstatus.inputRegisterCount; i++ )
		printf( "0x%.2x%s", inputRegisters[i], ( i == sstatus.inputRegisterCount - 1 ) ? "\n" : ", " );

	printf( "Dump coils:\n\t" );
	for ( i = 0; i < sstatus.coilCount; i++ )
		printf( "%d%s", modbusMaskRead( coils, sstatus.coilCount, i ), ( i == sstatus.coilCount - 1 ) ? "\n" : ", " );

	printf( "Dump discrete inputs:\n\t" );
	for ( i = 0; i < sstatus.discreteInputCount; i++ )
		printf( "%d%s", modbusMaskRead( discreteInputs, sstatus.discreteInputCount, i ), ( i == sstatus.discreteInputCount - 1 ) ? "\n" : ", " );

	//Dump response
	printf( "Dump response - length = %d:\n\t", sstatus.response.length );
	for ( i = 0; i < sstatus.response.length; i++ )
		printf( "%x%s", sstatus.response.frame[i], ( i == sstatus.response.length - 1 ) ? "\n" : ", " );

	//Process response
	l = sstatus.response.length;
	memcpy( f1, sstatus.response.frame, l );
	printf( "Let master process response...\n" );
	#ifdef LIGHTMODBUS_STATIC_MEM_MASTER_RESPONSE
		memcpy( mstatus.response.frame, sstatus.response.frame, sstatus.response.length );
	#else
		mstatus.response.frame = sstatus.response.frame;
	#endif
	mstatus.response.length = sstatus.response.length;
	mok = MasterError = modbusParseResponse( &mstatus );
	if ( !SlaveError && mstatus.predictedResponseLength != sstatus.response.length && sstatus.response.length )
		printf( "Response prediction doesn't match!! (p. %d vs a. %d)\n", mstatus.predictedResponseLength, \
			sstatus.response.length );

	if ( memcmp( f1, sstatus.response.frame, l ) ) printf( "!!!Master has malformed the frame!!!\n" );

	//Dump parsed data
	printf( "\tError - %s\n\tFinished - 1\n", modbuserrstr(MasterError) );
	printf( "\tparseError - %s\n", modbusferrstr(MasterError != MODBUS_OK && MasterError !=  MODBUS_ERROR_EXCEPTION? mstatus.parseError : 0) );

	if ( mstatus.data.length )
		for ( i = 0; i < mstatus.data.count; i++ )
		{
			printf( "\t - { addr: 0x%x, type: 0x%x, reg: 0x%x, val: 0x%x }\n", mstatus.data.address, mstatus.data.type, mstatus.data.index + i,\
			( mstatus.data.type == MODBUS_HOLDING_REGISTER || mstatus.data.type == MODBUS_INPUT_REGISTER ) ? mstatus.data.regs[i] : \
			modbusMaskRead( mstatus.data.coils, mstatus.data.length, i ) );
		}

	if ( MasterError == MODBUS_ERROR_EXCEPTION )
	{
		//TermRGB( 4, 1, 0 );
		printf( "\t - ex addr: 0x%x, fun: 0x%x, code: 0x%x\n\r", mstatus.exception.address, mstatus.exception.function, mstatus.exception.code );
	}

	if ( mok == MODBUS_OK || mok == MODBUS_ERROR_EXCEPTION ) examinem( );
	if ( sok == MODBUS_OK || sok == MODBUS_ERROR_EXCEPTION ) examines( );

	printf( "----------------------------------------\n\n" );
	mstatus.request.length = 0;
}

void libinit( )
{
	//Init slave and master
	sstatus.registerCount = 8;
	sstatus.inputRegisterCount = 4;
	#ifdef LIGHTMODBUS_REGISTER_CALLBACK
		sstatus.registerCallback = reg_callback;
	#else
		sstatus.registers = registers;
		sstatus.inputRegisters = inputRegisters;
	#endif

	sstatus.coilCount = 32;
	sstatus.discreteInputCount = 16;
	#ifdef LIGHTMODBUS_COIL_CALLBACK
		sstatus.registerCallback = reg_callback;
	#else
		sstatus.coils = coils;
		sstatus.discreteInputs = discreteInputs;
	#endif


	sstatus.address = 32;

	printf( "slave init - %d\n", modbusSlaveInit( &sstatus ) );
	printf( "master init - %d\n\n\n", modbusMasterInit( &mstatus ) );

	//This must be after modbusSlaveInit()
	#if defined( LIGHTMODBUS_REGISTER_CALLBACK ) || defined( LIGHTMODBUS_COIL_CALLBACK )
	sstatus.registerCallbackContext = &dummy_ctx_var;
	#endif
}

void MainTest( )
{
	//request03 - ok
	printf( "\t\t03 - correct request...\n" );
	modbusBuildRequest03( &mstatus, 0x20, 0x00, 0x08 );
	Test( );

	//request03 - bad CRC
	printf( "\t\t03 - bad CRC...\n" );
	modbusBuildRequest03( &mstatus, 0x20, 0x00, 0x08 );
	mstatus.request.frame[mstatus.request.length - 1]++;
	Test( );

	//request03 - bad first register
	printf( "\t\t03 - bad first register...\n" );
	modbusBuildRequest03( &mstatus,0x20, 0xff, 0x08 );
	Test( );

	//request03 - bad register count
	printf( "\t\t03 - bad register count...\n" );
	modbusBuildRequest03( &mstatus, 0x20, 0x00, 65 );
	Test( );

	//request03 - bad register count and first register
	printf( "\t\t03 - bad register count and first register...\n" );
	modbusBuildRequest03( &mstatus,0x20, 9, 32 );
	Test( );

	//request03 - broadcast
	printf( "\t\t03 - broadcast...\n" );
	modbusBuildRequest03( &mstatus,0x00, 0x00, 0x08 );
	Test( );

	//request03 - bad register count and first register
	printf( "\t\t03 - bad broadcast...\n" );
	modbusBuildRequest03( &mstatus,0x00, 9, 32 );
	Test( );

	//request03 - other slave address
	printf( "\t\t03 - other address...\n" );
	modbusBuildRequest03( &mstatus,0x10, 0x00, 0x08 );
	Test( );

	//request06 - ok
	printf( "\t\t06 - correct request...\n" );
	modbusBuildRequest06( &mstatus,0x20, 0x06, 0xface );
	Test( );

	//request06 - bad CRC
	printf( "\t\t06 - bad CRC...\n" );
	modbusBuildRequest06( &mstatus,0x20, 0x06, 0xf6 );
	mstatus.request.frame[mstatus.request.length - 1]++;
	Test( );

	//request06 - bad register
	printf( "\t\t06 - bad register...\n" );
	modbusBuildRequest06( &mstatus,0x20, 0xf6, 0xf6 );
	Test( );

	//request06 - broadcast
	printf( "\t\t06 - broadcast...\n" );
	modbusBuildRequest06( &mstatus, 0x00, 0x06, 0xf6 );
	Test( );

	//request06 - bad register
	printf( "\t\t06 - bad broadcast...\n" );
	modbusBuildRequest06( &mstatus,0x00, 0xf6, 0xf6 );
	Test( );

	//request06 - other slave address
	printf( "\t\t06 - other address...\n" );
	modbusBuildRequest06( &mstatus, 0x10, 0x06, 0xf6 );
	Test( );

	//request16 - ok
	printf( "\t\t16 - correct request...\n" );
	modbusBuildRequest16( &mstatus, 0x20, 0x00, 0x04, TestValues );
	Test( );

	//request16 - bad CRC
	printf( "\t\t16 - bad CRC...\n" );
	modbusBuildRequest16( &mstatus, 0x20, 0x00, 0x04, TestValues );
	mstatus.request.frame[mstatus.request.length - 1]++;
	Test( );

	//request16 - bad start reg
	printf( "\t\t16 - bad first register...\n" );
	modbusBuildRequest16( &mstatus, 0x20, 0xFF, 0x04, TestValues );
	Test( );

	//request16 - bad register range
	printf( "\t\t16 - bad register range...\n" );
	modbusBuildRequest16( &mstatus, 0x20, 0x00, 65, TestValues2 );
	Test( );

	//request16 - bad register range 2
	printf( "\t\t16 - bad register range 2...\n" );
	modbusBuildRequest16( &mstatus, 0x20, 0x00, 0x20, TestValues2 );
	Test( );

	//request16 - confusing register range
	printf( "\t\t16 - confusing register range...\n" );
	modbusBuildRequest16( &mstatus, 0x20, 0x00, 0x08, TestValues );
	Test( );

	//request16 - confusing register range 2
	printf( "\t\t16 - confusing register range 2...\n" );
	modbusBuildRequest16( &mstatus, 0x20, 0x01, 0x08, TestValues );
	Test( );

	//request16 - broadcast
	printf( "\t\t16 - broadcast...\n" );
	modbusBuildRequest16( &mstatus, 0x00, 0x00, 0x04, TestValues );
	Test( );

	//request16 - bad register range
	printf( "\t\t16 - bad broadcast...\n" );
	modbusBuildRequest16( &mstatus, 0x00, 0x00, 65, TestValues2 );
	Test( );

	//request16 - other slave address
	printf( "\t\t16 - other address...\n" );
	modbusBuildRequest16( &mstatus, 0x10, 0x00, 0x04, TestValues );
	Test( );

	//request02 - ok
	printf( "\t\t02 - correct request...\n" );
	modbusBuildRequest02( &mstatus, 0x20, 0x00, 0x10 );
	Test( );

	//request02 - bad CRC
	printf( "\t\t02 - bad CRC...\n" );
	modbusBuildRequest02( &mstatus, 0x20, 0x00, 0x10 );
	mstatus.request.frame[mstatus.request.length - 1]++;
	Test( );

	//request02 - bad first discrete input
	printf( "\t\t02 - bad first discrete input...\n" );
	modbusBuildRequest02( &mstatus, 0x20, 0xff, 0x10 );
	Test( );

	//request02 - bad discrete input count
	printf( "\t\t02 - bad discrete input count...\n" );
	modbusBuildRequest02( &mstatus, 0x20, 0x00, 0xff );
	Test( );

	//request02 - bad register count and first discrete input
	printf( "\t\t02 - bad register count and first discrete input...\n" );
	modbusBuildRequest02( &mstatus, 0x20, 0xff, 0xff );
	Test( );

	//request02 - broadcast
	printf( "\t\t02 - broadcast...\n" );
	modbusBuildRequest02( &mstatus, 0x00, 0x00, 0x10 );
	Test( );

	//request02 - bad first discrete input
	printf( "\t\t02 - bad broadcast...\n" );
	modbusBuildRequest02( &mstatus, 0x00, 0xff, 0x10 );
	Test( );

	//request02 - other slave address
	printf( "\t\t02 - other address...\n" );
	modbusBuildRequest02( &mstatus, 0x10, 0x00, 0x10 );
	Test( );

	printf( "\nREINIT OF LIBRARY\n" );
	modbusSlaveEnd( &sstatus );
	modbusMasterEnd( &mstatus );
	libinit( );

	//request01 - ok
	printf( "\t\t01 - correct request...\n" );
	modbusBuildRequest01( &mstatus, 0x20, 0x00, 0x04 );
	Test( );

	//request01 - bad CRC
	printf( "\t\t01 - bad CRC...\n" );
	modbusBuildRequest01( &mstatus, 0x20, 0x00, 0x04 );
	mstatus.request.frame[mstatus.request.length - 1]++;
	Test( );

	//request01 - bad first register
	printf( "\t\t01 - bad first coil...\n" );
	modbusBuildRequest01( &mstatus, 0x20, 0xff, 0x04 );
	Test( );

	//request01 - bad register count
	printf( "\t\t01 - bad coil count...\n" );
	modbusBuildRequest01( &mstatus, 0x20, 0x00, 0xff );
	Test( );

	//request01 - bad register count and first register
	printf( "\t\t01 - bad coil count and first coil...\n" );
	modbusBuildRequest01( &mstatus, 0x20, 0xffff, 1250 );
	Test( );

	//request01 - broadcast
	printf( "\t\t01 - broadcast...\n" );
	modbusBuildRequest01( &mstatus, 0x00, 0x00, 0x04 );
	Test( );

	//request01 - bad register count
	printf( "\t\t01 - bad broadcast...\n" );
	modbusBuildRequest01( &mstatus, 0x00, 0x00, 0xff );
	Test( );

	//request01 - other slave address
	printf( "\t\t01 - other address...\n" );
	modbusBuildRequest01( &mstatus, 0x10, 0x00, 0x04 );
	Test( );

	//request05 - ok
	printf( "\t\t05 - correct request...\n" );
	modbusBuildRequest05( &mstatus, 0x20, 0x03, 0xff00 );
	Test( );

	//request05 - bad CRC
	printf( "\t\t05 - bad CRC...\n" );
	modbusBuildRequest05( &mstatus, 0x20, 0x03, 0xff00 );
	mstatus.request.frame[mstatus.request.length - 1]++;
	Test( );

	//request05 - ok
	printf( "\t\t05 - correct request, non 0xff00 number...\n" );
	modbusBuildRequest05( &mstatus, 0x20, 0x03, 1 );
	Test( );

	//request05 - bad register
	printf( "\t\t05 - bad coil...\n" );
	modbusBuildRequest05( &mstatus, 0x20, 0xf3, 0xff00 );
	Test( );

	//request05 - broadcast
	printf( "\t\t05 - broadcast...\n" );
	modbusBuildRequest05( &mstatus, 0x00, 0x03, 0xff00 );
	Test( );

	//request05 - bad register
	printf( "\t\t05 - bad broadcast...\n" );
	modbusBuildRequest05( &mstatus, 0x00, 0xf3, 0xff00 );
	Test( );

	//request05 - other slave address
	printf( "\t\t05 - other address...\n" );
	modbusBuildRequest05( &mstatus, 0x10, 0x03, 0xff00 );
	Test( );

	//request15 - ok
	printf( "\t\t15 - correct request...\n" );
	modbusBuildRequest15( &mstatus, 0x20, 0x00, 0x04, TestValues3 );
	Test( );

	//request15 - bad CRC
	printf( "\t\t15 - bad CRC...\n" );
	modbusBuildRequest15( &mstatus, 0x20, 0x00, 0x04, TestValues3 );
	mstatus.request.frame[mstatus.request.length - 1]++;
	Test( );

	//request15 - bad start reg
	printf( "\t\t15 - bad first coil...\n" );
	modbusBuildRequest15( &mstatus, 0x20, 0xFF, 0x04, TestValues3 );
	Test( );

	//request15 - bad register range
	printf( "\t\t15 - bad coil range...\n" );
	modbusBuildRequest15( &mstatus, 0x20, 0x00, 65, TestValues3 );
	Test( );

	//request15 - bad register range 2
	printf( "\t\t15 - bad coil range 2...\n" );
	modbusBuildRequest15( &mstatus, 0x20, 0x00, 0x20, TestValues3 );
	Test( );

	//request15 - confusing register range
	printf( "\t\t15 - confusing coil range...\n" );
	modbusBuildRequest15( &mstatus, 0x20, 0x00, 0x40, TestValues3 );
	Test( );

	//request15 - confusing register range 2
	printf( "\t\t15 - confusing coil range 2...\n" );
	modbusBuildRequest15( &mstatus, 0x20, 0x01, 0x40, TestValues3 );
	Test( );

	//request15 - broadcast
	printf( "\t\t15 - broadcast...\n" );
	modbusBuildRequest15( &mstatus, 0x00, 0x00, 0x04, TestValues3 );
	Test( );

	//request15 - bad register range 2
	printf( "\t\t15 - bad broadcast...\n" );
	modbusBuildRequest15( &mstatus, 0x00, 0x00, 0x20, TestValues3 );
	Test( );

	//request15 - other slave address
	printf( "\t\t15 - other address...\n" );
	modbusBuildRequest15( &mstatus, 0x10, 0x00, 0x04, TestValues3 );
	Test( );

	//request04 - ok
	printf( "\t\t04 - correct request...\n" );
	modbusBuildRequest04( &mstatus, 0x20, 0x00, 0x04 );
	Test( );

	//request04 - bad CRC
	printf( "\t\t04 - bad CRC...\n" );
	modbusBuildRequest04( &mstatus, 0x20, 0x00, 0x04 );
	mstatus.request.frame[mstatus.request.length - 1]++;
	Test( );

	//request04 - bad first register
	printf( "\t\t04 - bad first register...\n" );
	modbusBuildRequest04( &mstatus, 0x20, 0x05, 0x04 );
	Test( );

	//request04 - bad register count
	printf( "\t\t04 - bad register count...\n" );
	modbusBuildRequest04( &mstatus, 0x20, 0x00, 0x05 );
	Test( );

	//request04 - bad register count and first register
	printf( "\t\t04 - bad register count and first register...\n" );
	modbusBuildRequest04( &mstatus, 0x20, 0x01, 0x05 );
	Test( );

	//request04 - broadcast
	printf( "\t\t04 - broadcast...\n" );
	modbusBuildRequest04( &mstatus, 0x00, 0x00, 0x04 );
	Test( );

	//request04 - bad register count and first register
	printf( "\t\t04 - bad broadcast...\n" );
	modbusBuildRequest04( &mstatus, 0x00, 0x01, 0x05 );
	Test( );

	//request04 - other slave address
	printf( "\t\t04 - other address...\n" );
	modbusBuildRequest04( &mstatus, 0x10, 0x00, 0x04 );
	Test( );

	//request06 - ok
	printf( "\t\t22 - correct request...\n" );
	modbusBuildRequest22( &mstatus,0x20, 0x06, 14 << 8, 56 << 8 );
	Test( );

	//request06 - bad CRC
	printf( "\t\t22 - bad CRC...\n" );
	modbusBuildRequest22( &mstatus,0x20, 0x06, 14 << 8, 56 << 8 );
	mstatus.request.frame[mstatus.request.length - 1]++;
	Test( );

	//request06 - bad register
	printf( "\t\t22 - bad register...\n" );
	modbusBuildRequest22( &mstatus,0x20, 0xf6, 14 << 8, 56 << 8 );
	Test( );

	//request06 - broadcast
	printf( "\t\t22 - broadcast...\n" );
	modbusBuildRequest22( &mstatus, 0x00, 0x06, 14 << 8, 56 << 8 );
	Test( );

	//request06 - bad register
	printf( "\t\t22 - bad broadcast...\n" );
	modbusBuildRequest22( &mstatus,0x00, 0xf6, 14 << 8, 56 << 8 );
	Test( );

	//request06 - other slave address
	printf( "\t\t22 - other address...\n" );
	modbusBuildRequest22( &mstatus, 0x10, 0x06, 14 << 8, 56 << 8 );
	Test( );

	//WRITE PROTECTION TEST

	#if !defined(LIGHTMODBUS_COIL_CALLBACK) || !defined(LIGHTMODBUS_REGISTER_CALLBACK)
		uint8_t mask[1] = { 0 };
	#endif

	#ifdef LIGHTMODBUS_REGISTER_CALLBACK
	#else
		printf( "\t\t--Register write protection test--\n" );

		sstatus.registerMask = mask;
		sstatus.registerMaskLength = 1;

		modbusMaskWrite( mask, 1, 2, 1 );

		modbusBuildRequest06( &mstatus, 0x20, 2, 16 );
		Test( );
		modbusBuildRequest06( &mstatus, 0x20, 0, 16 );
		Test( );
		modbusBuildRequest22( &mstatus,0x20, 0x02, 14 << 8, 56 << 8 );
		Test( );

		modbusBuildRequest16( &mstatus, 0x20, 0, 4, TestValues2 );
		Test( );
		modbusBuildRequest16( &mstatus, 0x20, 0, 2, TestValues2 );
		Test( );
		modbusBuildRequest22( &mstatus,0x20, 0x00, 14 << 8, 56 << 8 );
		Test( );
		sstatus.registerMaskLength = 0;
	#endif

	//WRITE PROTECTION TEST 2
	#ifdef LIGHTMODBUS_COIL_CALLBACK
	#else
		printf( "\t\t--Coil write protection test--\n" );
		sstatus.coilMask = mask;
		sstatus.coilMaskLength = 1;

		modbusMaskWrite( mask, 1, 2, 1 );

		modbusBuildRequest05( &mstatus, 0x20, 2, 16 );
		Test( );
		modbusBuildRequest05( &mstatus, 0x20, 0, 16 );
		Test( );

		modbusBuildRequest15( &mstatus, 0x20, 0, 16, TestValues3 );
		Test( );
		modbusBuildRequest15( &mstatus, 0x20, 0, 2, TestValues3 );
		Test( );

		printf( "Bitval: %d\r\n", modbusMaskRead( mask, 1, 0 ) );
		printf( "Bitval: %d\r\n", modbusMaskRead( mask, 1, 1 ) );
		printf( "Bitval: %d\r\n", modbusMaskRead( mask, 1, 2 ) );
		printf( "Bitval: %d\r\n", modbusMaskRead( mask, 1, 3 ) );
		printf( "Bitval: %d\r\n", modbusMaskRead( mask, 1, 4 ) );
		sstatus.coilMaskLength = 0;
	#endif

}


//User defined functions check
void userf_test( )
{
	//SLAVE
	#ifdef LIGHTMODBUS_SLAVE_USER_FUNCTIONS
		static ModbusSlaveUserFunction userf[2] =
		{
			{254, NULL},
			{255, userModbusFunction}
		};

		sstatus.userFunctions = userf;
		sstatus.userFunctionCount = 2;
	#endif

	//MASTER
	#ifdef LIGHTMODBUS_MASTER_USER_FUNCTIONS
		static ModbusMasterUserFunction muserf[2] =
		{
			{254, NULL},
			{255, userModbusMasterFunction}
		};
		mstatus.userFunctions = muserf;
		mstatus.userFunctionCount = 2;
		#ifndef LIGHTMODBUS_STATIC_MEM_MASTER_REQUEST
			free( mstatus.request.frame );
		#endif
	#endif

	int test_slave = 1;
	int test_master = 1;

	//Frames setup
	#if defined(LIGHTMODBUS_SLAVE_USER_FUNCTIONS) || defined(LIGHTMODBUS_MASTER_USER_FUNCTIONS)
		printf( "\n\n-----------\n\nUser defined functions test\n" );
		uint8_t userf_frame1[] = {0x20, 0xFF, 0x58, 0x30};
		printf( "using frame: " );
		uint8_t k;
		for ( k = 0; k < sizeof userf_frame1; k++ ) printf( "%x, ", userf_frame1[k] );
		printf( "\nCRC: 0x%x\n", modbusCRC( userf_frame1, sizeof( userf_frame1 ) - 2 ) );

		uint8_t userf_frame2[] = {0x20, 0xFE, 0x99, 0xF0};
		printf( "using frame: " );
		for ( k = 0; k < sizeof userf_frame2; k++ ) printf( "%x, ", userf_frame2[k] );
		printf( "\nCRC: 0x%x\n", modbusCRC( userf_frame2, sizeof( userf_frame2 ) - 2 ) );

		#ifdef LIGHTMODBUS_STATIC_MEM_SLAVE_REQUEST 
		if ( sizeof( userf_frame1 ) > LIGHTMODBUS_STATIC_MEM_SLAVE_REQUEST 
			|| sizeof( userf_frame2 ) > LIGHTMODBUS_STATIC_MEM_SLAVE_REQUEST )
		{
			printf( "not testing slave user functions cause my test frames are longer than specified buffer sizes\n" );
			test_slave = 0;
		}
		#endif

		#ifdef LIGHTMODBUS_STATIC_MEM_MASTER_RESPONSE 
		if ( sizeof( userf_frame1 ) > LIGHTMODBUS_STATIC_MEM_MASTER_RESPONSE
			|| sizeof( userf_frame2 ) > LIGHTMODBUS_STATIC_MEM_MASTER_RESPONSE )
		{
			printf( "not testing master user functions cause my test frames are longer than specified buffer sizes\n" );
			test_master = 0;
		}
		#endif

	#endif

	//SLAVE
	#ifdef LIGHTMODBUS_SLAVE_USER_FUNCTIONS
		if ( test_slave )
		{
			#ifdef LIGHTMODBUS_STATIC_MEM_SLAVE_REQUEST
				memcpy( sstatus.request.frame, userf_frame1, sizeof( userf_frame1 ) );
			#else
				sstatus.request.frame = userf_frame1;
			#endif
			sstatus.request.length = sizeof userf_frame1;
			printf( "PARSE RESULT 1: %d\n", modbusParseRequest( &sstatus ) );
		}
	#endif

	//MASTER
	#ifdef LIGHTMODBUS_MASTER_USER_FUNCTIONS
		if ( test_master )
		{
			#ifdef LIGHTMODBUS_STATIC_MEM_MASTER_RESPONSE
				memcpy( mstatus.response.frame, userf_frame1, sizeof( userf_frame1 ) );
			#else
				mstatus.response.frame = userf_frame1;
			#endif
			#ifdef LIGHTMODBUS_STATIC_MEM_MASTER_REQUEST
				memcpy( mstatus.request.frame, userf_frame1, sizeof( userf_frame1 ) );
			#else
				mstatus.request.frame = userf_frame1;
			#endif
			mstatus.response.length = sizeof userf_frame1;
			mstatus.request.length = sizeof userf_frame1;
			printf( "MASTER PARSE RESULT 1: %d\n", modbusParseResponse( &mstatus ) );
		}
	#endif

	//SLAVE
	#ifdef LIGHTMODBUS_SLAVE_USER_FUNCTIONS
		if ( test_slave )
		{
			#ifdef LIGHTMODBUS_STATIC_MEM_SLAVE_REQUEST
				memcpy( sstatus.request.frame, userf_frame2, sizeof( userf_frame2 ) );
			#else
				sstatus.request.frame = userf_frame2;
			#endif
			sstatus.request.length = sizeof userf_frame2;
			printf( "PARSE RESULT 2: %d\n", modbusParseRequest( &sstatus ) );
		}
	#endif

	//MASTER
	#ifdef LIGHTMODBUS_MASTER_USER_FUNCTIONS
		if ( test_master )
		{
			#ifdef LIGHTMODBUS_STATIC_MEM_MASTER_RESPONSE
				memcpy( mstatus.response.frame, userf_frame2, sizeof( userf_frame2 ) );
			#else
				mstatus.response.frame = userf_frame2;
			#endif
			#ifdef LIGHTMODBUS_STATIC_MEM_MASTER_REQUEST
				memcpy( mstatus.request.frame, userf_frame2, sizeof( userf_frame2 ) );
			#else
				mstatus.request.frame = userf_frame2;
			#endif
			mstatus.response.length = sizeof userf_frame2;
			mstatus.request.length = sizeof userf_frame2;
			printf( "MASTER PARSE RESULT 2: %d\n", modbusParseResponse( &mstatus ) );
		}
	#endif

	#if !defined(LIGHTMODBUS_SLAVE_USER_FUNCTIONS)
		printf( "\n\n-------\n\n slave user functions are disabled!\n" );
	#endif

	#if !defined(LIGHTMODBUS_MASTER_USER_FUNCTIONS)
		printf( "\n\n-------\n\n master user functions are disabled!\n" );
	#else
		#ifndef LIGHTMODBUS_STATIC_MEM_MASTER_REQUEST
			mstatus.request.frame = NULL; //To stop master from freeing it
		#endif
	#endif

}

int main( )
{
	printf( "\n\t\t======LIBLIGHTMODBUS LIBRARY COVERAGE TEST LOG======" );
	printf( "\n\t\t======LIBLIGHTMODBUS VERSION: %s\n\n\n", LIGHTMODBUS_VERSION );
	memset( TestValues2, 0xAA, 1024 );
	libinit( );
	userf_test( );
	MainTest( );
	#if !defined(LIGHTMODBUS_REGISTER_CALLBACK) && !defined(LIGHTMODBUS_COIL_CALLBACK)
		maxlentest( );
	#endif

	modbusSlaveEnd( &sstatus );
	modbusMasterEnd( &mstatus );

	//Reset terminal colors
	printf( "\t\t" );

	return 0;
}
