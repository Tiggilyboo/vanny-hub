#!/usr/bin/env python
"""
Pymodbus Synchronous Server Example
--------------------------------------------------------------------------

The synchronous server is implemented in pure python without any third
party libraries (unless you need to use the serial protocols which require
pyserial). This is helpful in constrained or old environments where using
twisted is just not feasible. What follows is an example of its use:
"""
# --------------------------------------------------------------------------- #
# import the various server implementations
# --------------------------------------------------------------------------- #
from pymodbus.version import version
from pymodbus.server.sync import StartTcpServer
from pymodbus.server.sync import StartTlsServer
from pymodbus.server.sync import StartUdpServer
from pymodbus.server.sync import StartSerialServer

from pymodbus.device import ModbusDeviceIdentification
from pymodbus.datastore import ModbusSequentialDataBlock, ModbusSparseDataBlock
from pymodbus.datastore import ModbusSlaveContext, ModbusServerContext

from pymodbus.transaction import ModbusRtuFramer, ModbusBinaryFramer
# --------------------------------------------------------------------------- #
# configure the service logging
# --------------------------------------------------------------------------- #
import logging
FORMAT = ('%(asctime)-15s %(threadName)-15s'
          ' %(levelname)-8s %(module)-15s:%(lineno)-8s %(message)s')
logging.basicConfig(format=FORMAT)
log = logging.getLogger()
log.setLevel(logging.DEBUG)


def run_server():
    # ----------------------------------------------------------------------- #
    # initialize your data store
    # ----------------------------------------------------------------------- #
    # The datastores only respond to the addresses that they are initialized to
    # Therefore, if you initialize a DataBlock to addresses of 0x00 to 0xFF, a
    # request to 0x100 will respond with an invalid address exception. This is
    # because many devices exhibit this kind of behavior (but not all)::
    #
    #     block = ModbusSequentialDataBlock(0x00, [0]*0xff)
    #
    # Continuing, you can choose to use a sequential or a sparse DataBlock in
    # your data context.  The difference is that the sequential has no gaps in
    # the data while the sparse can. Once again, there are devices that exhibit
    # both forms of behavior::
    #
    block = ModbusSparseDataBlock({
        # aux soc
        0x101: 0b01011111,
        # aux v
        0x102: 141, # 0.1V
        # aux a
        0x103: 3309, # 0.01A
        # controller temperature
        0x104: 0b0001010100001111, # H: ctrl temp, L: battery temp sensor
        # altenator voltage
        0x105: 146, # 0.1V
        # altenator amps
        0x106: 4000, #0.01A
        # altenator watts
        0x107: 496,
        # solar v
        0x108: 304,
        # solar a
        0x109: 20,
        # solar w
        0x10A: 6684,
        0x10B: 0,
        # lowest battery V in day
        0x10C: 108,     # 0.1
        # max battery V in day
        0x10D: 144,
        # max battery A in day
        0x10E: 5000, #0.01A

        0x10F: 0,
        0x110: 0,
        0x111: 0,
        0x112: 0,
        0x113: 0,
        0x114: 0,
        0x115: 0,

        # running day count
        0x116: 1,

        0x117: 0,
        # number of times battery is full in day
        0x118: 3,

        0x119: 0,
        0x11A: 0,
        0x11B: 0,
        0x11C: 0,
        0x11D: 0,
        0x11E: 0,
        0x11F: 0,
        0x120: 0,

        # charge state (L)
        0x121: 0b0000000010001010,

        0x122: 0,
        0x123: 0,
    })
    #     block = ModbusSequentialDataBlock(0x00, [0]*5)
    #
    # Alternately, you can use the factory methods to initialize the DataBlocks
    # or simply do not pass them to have them initialized to 0x00 on the full
    # address range::
    #
    #     store = ModbusSlaveContext(di = ModbusSequentialDataBlock.create())
    #     store = ModbusSlaveContext()
    #
    # Finally, you are allowed to use the same DataBlock reference for every
    # table or you may use a separate DataBlock for each table.
    # This depends if you would like functions to be able to access and modify
    # the same data or not::
    #
    #     block = ModbusSequentialDataBlock(0x00, [0]*0xff)
    #     store = ModbusSlaveContext(di=block, co=block, hr=block, ir=block)
    #
    # The server then makes use of a server context that allows the server to
    # respond with different slave contexts for different unit ids. By default
    # it will return the same context for every unit id supplied (broadcast
    # mode).
    # However, this can be overloaded by setting the single flag to False and
    # then supplying a dictionary of unit id to context mapping::
    #
    store = ModbusSlaveContext(hr=block, ir=block)
    context = ModbusServerContext(slaves=store, single=True)

    # ----------------------------------------------------------------------- #
    # initialize the server information
    # ----------------------------------------------------------------------- #
    # If you don't set this or any fields, they are defaulted to empty strings.
    # ----------------------------------------------------------------------- #
    identity = ModbusDeviceIdentification()
    identity.VendorName = 'Pymodbus'
    identity.ProductCode = 'PM'
    identity.VendorUrl = 'http://github.com/riptideio/pymodbus/'
    identity.ProductName = 'Pymodbus Server'
    identity.ModelName = 'Pymodbus Server'
    identity.MajorMinorRevision = version.short()
    
    StartSerialServer(
        context, 
        framer=ModbusRtuFramer, 
        identity=identity,
        port='/dev/ttyUSB0', 
        timeout=.05, 
        baudrate=9600,
        stopbits=2,
        bytesize=8)


if __name__ == "__main__":
    run_server()

