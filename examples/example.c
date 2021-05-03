#include <stdio.h>
#include <stdint.h>

#include <lightmodbus/lightmodbus.h>
#include <lightmodbus/master.h>
#include <lightmodbus/slave.h>

//Configuration structures
ModbusMaster mstatus;
ModbusSlave sstatus;

//Registers and coils
uint8_t coils[2] = { 0 };
uint16_t regs[32] = { 0 };

//For storing exit codes
uint8_t sec, mec;


//Dump slave information
void slavedump( )
{
	int i;
	printf( "==SLAVE DUMP==\n" );

	printf( "Registers:" );
	for ( i = 0; i < sstatus.registerCount; i++ )
		printf( " %d", sstatus.registers[i] );
	printf( "\n" );

	printf( "Coils:" );
	for ( i = 0; i < sstatus.coilCount >> 3; i++ )
		printf( " %d", sstatus.coils[i] );
	printf( "\n" );

	printf( "Request:" );
	for ( i = 0; i < sstatus.request.length; i++ )
		printf( " %d", sstatus.request.frame[i] );
	printf( "\n" );

	printf( "Response:" );
	for ( i = 0; i < sstatus.response.length; i++ )
		printf( " %d", sstatus.response.frame[i] );
	printf( "\n" );

	printf( "Exit code: %d\n\n", sec );
}

//Dump master information
void masterdump( )
{
	int i;
	printf( "==MASTER DUMP==\n" );

	printf( "Received data: slave: %d, addr: %d, count: %d, type: %d\n",
		mstatus.data.address, mstatus.data.index, mstatus.data.count, mstatus.data.type );
	printf( "\t\tvalues:" );
	switch ( mstatus.data.type )
	{
		case MODBUS_HOLDING_REGISTER:
		case MODBUS_INPUT_REGISTER:
			for ( i = 0; i < mstatus.data.count; i++ )
				printf( " %d", mstatus.data.regs[i] );
			break;

		case MODBUS_COIL:
		case MODBUS_DISCRETE_INPUT:
			for ( i = 0; i < mstatus.data.count; i++ )
				printf( " %d", modbusMaskRead( mstatus.data.coils, mstatus.data.length, i ) );
			break;
	}
	printf( "\n" );

	printf( "Request:" );
	for ( i = 0; i < mstatus.request.length; i++ )
		printf( " %d", mstatus.request.frame[i] );
	printf( "\n" );

	printf( "Response:" );
	for ( i = 0; i < mstatus.response.length; i++ )
		printf( " %d", mstatus.response.frame[i] );
	printf( "\n" );

	printf( "Exit code: %d\n\n", mec );
}

int main( )
{
	//Init slave (input registers and discrete inputs work just the same)
	sstatus.address = 27;
	sstatus.registers = regs;
	sstatus.registerCount = 32;
	sstatus.coils = coils;
	sstatus.coilCount = 16;
	modbusSlaveInit( &sstatus );

	//Init master
	modbusMasterInit( &mstatus );

	//Dump status
	slavedump( );
	masterdump( );

	/* WRITE VALUE */

	//Build frame to write single register
	modbusBuildRequest06( &mstatus, 27, 03, 56 );

	//Pretend frame is being sent to slave
	sstatus.request.frame = mstatus.request.frame;
	sstatus.request.length = mstatus.request.length;

	//Let slave parse frame
	sec = modbusParseRequest( &sstatus );

	//Pretend frame is being sent to master
	mstatus.response.frame = sstatus.response.frame;
	mstatus.response.length = sstatus.response.length;

	//Let master parse the frame
	mec = modbusParseResponse( &mstatus );

	//Dump status again
	slavedump( );
	masterdump( );

	/* READ VALUE */

	//Build frame to read 4 registers
	modbusBuildRequest03( &mstatus, 27, 0, 4 );

	//Pretend frame is being sent to slave
	sstatus.request.frame = mstatus.request.frame;
	sstatus.request.length = mstatus.request.length;

	//Let slave parse frame
	sec = modbusParseRequest( &sstatus );

	//Pretend frame is being sent to master
	mstatus.response.frame = sstatus.response.frame;
	mstatus.response.length = sstatus.response.length;

	mec = modbusParseResponse( &mstatus );

	//Dump status again
	slavedump( );
	masterdump( );





	/* COILS */

	//Build frame to write single coil
	modbusBuildRequest05( &mstatus, 27, 07, 1 );

	//Pretend frame is being sent to slave
	sstatus.request.frame = mstatus.request.frame;
	sstatus.request.length = mstatus.request.length;

	//Let slave parse frame
	sec = modbusParseRequest( &sstatus );

	//Pretend frame is being sent to master
	mstatus.response.frame = sstatus.response.frame;
	mstatus.response.length = sstatus.response.length;

	mec = modbusParseResponse( &mstatus );

	//Dump status again
	slavedump( );
	masterdump( );

	/* READ VALUE */

	//Build frame to read 4 coils
	modbusBuildRequest01( &mstatus, 27, 0, 8 );

	//Pretend frame is being sent to slave
	sstatus.request.frame = mstatus.request.frame;
	sstatus.request.length = mstatus.request.length;

	//Let slave parse frame
	sec = modbusParseRequest( &sstatus );

	//Pretend frame is being sent to master
	mstatus.response.frame = sstatus.response.frame;
	mstatus.response.length = sstatus.response.length;

	mec = modbusParseResponse( &mstatus );

	//Dump status again
	slavedump( );
	masterdump( );

	return 0;
}
