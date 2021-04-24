import struct
from machine import UART
from machine import Pin
import time
import machine
import select

# function codes
READ_DISCRETE_INPUTS = 0x02

READ_COILS = 0x01
WRITE_SINGLE_COIL = 0x05
WRITE_MULTIPLE_COILS = 0x0F

READ_INPUT_REGISTER = 0x04
READ_HOLDING_REGISTERS = 0x03
WRITE_SINGLE_REGISTER = 0x06
WRITE_MULTIPLE_REGISTERS = 0x10
READ_WRITE_MULTIPLE_REGISTERS = 0x17
MASK_WRITE_REGISTER = 0x16
READ_FIFO_QUEUE = 0x18

READ_FILE_RECORD = 0x14
WRITE_FILE_RECORD = 0x15

READ_EXCEPTION_STATUS = 0x07
DIAGNOSTICS = 0x08
GET_COM_EVENT_COUNTER = 0x0B
GET_COM_EVENT_LOG = 0x0C
REPORT_SERVER_ID = 0x11
READ_DEVICE_IDENTIFICATION = 0x2B

# exception codes
ILLEGAL_FUNCTION = 0x01
ILLEGAL_DATA_ADDRESS = 0x02
ILLEGAL_DATA_VALUE = 0x03
SERVER_DEVICE_FAILURE = 0x04
ACKNOWLEDGE = 0x05
SERVER_DEVICE_BUSY = 0x06
MEMORY_PARITY_ERROR = 0x08
GATEWAY_PATH_UNAVAILABLE = 0x0A
DEVICE_FAILED_TO_RESPOND = 0x0B

# PDU constants
CRC_LENGTH = 0x02
ERROR_BIAS = 0x80
RESPONSE_HDR_LENGTH = 0x02
ERROR_RESP_LEN = 0x05
FIXED_RESP_LEN = 0x08
MBAP_HDR_LENGTH = 0x07

CRC16_TABLE = (
    0x0000,0xC0C1,0xC181,0x0140,0xC301,0x03C0,0x0280,0xC241,0xC601,
    0x06C0,0x0780,0xC741,0x0500,0xC5C1,0xC481,0x0440,0xCC01,0x0CC0,
    0x0D80,0xCD41,0x0F00,0xCFC1,0xCE81,0x0E40,0x0A00,0xCAC1,0xCB81,
    0x0B40,0xC901,0x09C0,0x0880,0xC841,0xD801,0x18C0,0x1980,0xD941,
    0x1B00,0xDBC1,0xDA81,0x1A40,0x1E00,0xDEC1,0xDF81,0x1F40,0xDD01,
    0x1DC0,0x1C80,0xDC41,0x1400,0xD4C1,0xD581,0x1540,0xD701,0x17C0,
    0x1680,0xD641,0xD201,0x12C0,0x1380,0xD341,0x1100,0xD1C1,0xD081,
    0x1040,0xF001,0x30C0,0x3180,0xF141,0x3300,0xF3C1,0xF281,0x3240,
    0x3600,0xF6C1,0xF781,0x3740,0xF501,0x35C0,0x3480,0xF441,0x3C00,
    0xFCC1,0xFD81,0x3D40,0xFF01,0x3FC0,0x3E80,0xFE41,0xFA01,0x3AC0,
    0x3B80,0xFB41,0x3900,0xF9C1,0xF881,0x3840,0x2800,0xE8C1,0xE981,
    0x2940,0xEB01,0x2BC0,0x2A80,0xEA41,0xEE01,0x2EC0,0x2F80,0xEF41,
    0x2D00,0xEDC1,0xEC81,0x2C40,0xE401,0x24C0,0x2580,0xE541,0x2700,
    0xE7C1,0xE681,0x2640,0x2200,0xE2C1,0xE381,0x2340,0xE101,0x21C0,
    0x2080,0xE041,0xA001,0x60C0,0x6180,0xA141,0x6300,0xA3C1,0xA281,
    0x6240,0x6600,0xA6C1,0xA781,0x6740,0xA501,0x65C0,0x6480,0xA441,
    0x6C00,0xACC1,0xAD81,0x6D40,0xAF01,0x6FC0,0x6E80,0xAE41,0xAA01,
    0x6AC0,0x6B80,0xAB41,0x6900,0xA9C1,0xA881,0x6840,0x7800,0xB8C1,
    0xB981,0x7940,0xBB01,0x7BC0,0x7A80,0xBA41,0xBE01,0x7EC0,0x7F80,
    0xBF41,0x7D00,0xBDC1,0xBC81,0x7C40,0xB401,0x74C0,0x7580,0xB541,
    0x7700,0xB7C1,0xB681,0x7640,0x7200,0xB2C1,0xB381,0x7340,0xB101,
    0x71C0,0x7080,0xB041,0x5000,0x90C1,0x9181,0x5140,0x9301,0x53C0,
    0x5280,0x9241,0x9601,0x56C0,0x5780,0x9741,0x5500,0x95C1,0x9481,
    0x5440,0x9C01,0x5CC0,0x5D80,0x9D41,0x5F00,0x9FC1,0x9E81,0x5E40,
    0x5A00,0x9AC1,0x9B81,0x5B40,0x9901,0x59C0,0x5880,0x9841,0x8801,
    0x48C0,0x4980,0x8941,0x4B00,0x8BC1,0x8A81,0x4A40,0x4E00,0x8EC1,
    0x8F81,0x4F40,0x8D01,0x4DC0,0x4C80,0x8C41,0x4400,0x84C1,0x8581,
    0x4540,0x8701,0x47C0,0x4680,0x8641,0x8201,0x42C0,0x4380,0x8341,
    0x4100,0x81C1,0x8081,0x4040
)

def read_coils(starting_address, quantity):
    if not (1 <= quantity <= 2000):
        raise ValueError('invalid number of coils')

    return struct.pack('>BHH', READ_COILS, starting_address, quantity)

def read_discrete_inputs(starting_address, quantity):
    if not (1 <= quantity <= 2000):
        raise ValueError('invalid number of discrete inputs')

    return struct.pack('>BHH', READ_DISCRETE_INPUTS, starting_address, quantity)

def read_holding_registers(starting_address, quantity):
    if not (1 <= quantity <= 125):
        raise ValueError('invalid number of holding registers')

    return struct.pack('>BHH', READ_HOLDING_REGISTERS, starting_address, quantity)

def read_input_registers(starting_address, quantity):
    if not (1 <= quantity <= 125):
        raise ValueError('invalid number of input registers')

    return struct.pack('>BHH', READ_INPUT_REGISTER, starting_address, quantity)

def write_single_coil(output_address, output_value):
    if output_value not in [0x0000, 0xFF00]:
        raise ValueError('Illegal coil value')

    return struct.pack('>BHH', WRITE_SINGLE_COIL, output_address, output_value)

def write_single_register(register_address, register_value, signed=True):
    fmt = 'h' if signed else 'H'

    return struct.pack('>BH' + fmt, WRITE_SINGLE_REGISTER, register_address, register_value)

def write_multiple_coils(starting_address, value_list):
    sectioned_list = [value_list[i:i + 8] for i in range(0, len(value_list), 8)]

    output_value=[]
    for index, byte in enumerate(sectioned_list):
        output = sum(v << i for i, v in enumerate(byte))
        output_value.append(output)

    fmt = 'B' * len(output_value)

    return struct.pack('>BHHB' + fmt, WRITE_MULTIPLE_COILS, starting_address,
                        len(value_list), (len(value_list) // 8) + 1, *output_value)

def write_multiple_registers(starting_address, register_values, signed=True):
    quantity = len(register_values)

    if not (1 <= quantity <= 123):
        raise ValueError('invalid number of registers')

    fmt = ('h' if signed else 'H') * quantity
    return struct.pack('>BHHB' + fmt, WRITE_MULTIPLE_REGISTERS, starting_address,
                        quantity, quantity * 2, *register_values)

def validate_resp_data(data, function_code, address, value=None, quantity=None, signed = True):
    if function_code in [WRITE_SINGLE_COIL, WRITE_SINGLE_REGISTER]:
        fmt = '>H' + ('h' if signed else 'H')
        resp_addr, resp_value = struct.unpack(fmt, data)

        if (address == resp_addr) and (value == resp_value):
            return True

    elif function_code in [WRITE_MULTIPLE_COILS, WRITE_MULTIPLE_REGISTERS]:
        resp_addr, resp_qty = struct.unpack('>HH', data)

        if (address == resp_addr) and (quantity == resp_qty):
            return True

    return False

class ModbusSerial:

    def __init__(self, uart_id, baudrate=9600, data_bits=8, stop_bits=1, parity=None, pins=None, ctrl_pin=None):
        self._uart = UART(uart_id, baudrate=baudrate, bits=data_bits, parity=parity, stop=stop_bits, timeout_char=10)

        if ctrl_pin is not None:
            self._ctrlPin = Pin(ctrl_pin, mode=Pin.OUT)
        else:
            self._ctrlPin = None
        self.char_time_ms = (1000 * (data_bits + stop_bits + 2)) // baudrate

    def _calculate_crc16(self, data):
        crc = 0xFFFF

        for char in data:
            crc = (crc >> 8) ^ CRC16_TABLE[((crc) ^ char) & 0xFF]

        return struct.pack('<H',crc)

    def _bytes_to_bool(self, byte_list):
        bool_list = []
        for index, byte in enumerate(byte_list):
            bool_list.extend([bool(byte & (1 << n)) for n in range(8)])

        return bool_list

    def _to_short(self, byte_array, signed=True):
        response_quantity = int(len(byte_array) / 2)
        fmt = '>' + (('h' if signed else 'H') * response_quantity)

        return struct.unpack(fmt, byte_array)

    def _exit_read(self, response):
        if len(response) > 0:
            if response[1] >= ERROR_BIAS and len(response) < ERROR_RESP_LEN:
                return False
        elif (READ_COILS <= response[1] <= READ_INPUT_REGISTER):
            expected_len = RESPONSE_HDR_LENGTH + 1 + response[2] + CRC_LENGTH
            if len(response) < expected_len:
                return False
        elif len(response) < FIXED_RESP_LEN:
            return False

        return True

    def _uart_read(self):
        response = bytearray()

        while self._uart.any() > 0:
            response.extend(self._uart.read(self._uart.any()))
            if self._exit_read(response):
                break
            time.sleep(0.05)

        return response

    def _send_receive(self, modbus_pdu, slave_addr, count):
        serial_pdu = bytearray()
        serial_pdu.append(slave_addr)
        serial_pdu.extend(modbus_pdu)

        crc = self._calculate_crc16(serial_pdu)
        serial_pdu.extend(crc)

        # flush the Rx FIFO
        flushed = bytearray()
        while self._uart.any() > 0:
            flushed.extend(self._uart.read())
        
        if len(flushed) > 0:
            print('flushed: ', flushed)
        
        if self._ctrlPin:
            self._ctrlPin(0)
        self._uart.write(serial_pdu)
        
        if self._ctrlPin:
            poll = select.poll()
            poll.register(self._uart, select.POLLIN)
            
            print ('Polling uart... ', poll)
            events = poll.poll()
            print('events =', events)

            self._ctrlPin(1)

        return self._validate_resp_hdr(self._uart_read(), slave_addr, modbus_pdu[0], count)

    def _validate_resp_hdr(self, response, slave_addr, function_code, count):

        if len(response) == 0:
            raise OSError('no data received from slave')

        resp_crc = response[-CRC_LENGTH:]
        expected_crc = self._calculate_crc16(response[0:len(response) - CRC_LENGTH])
        if (resp_crc[0] != expected_crc[0]) or (resp_crc[1] != expected_crc[1]):
            raise OSError('invalid response CRC')

        if (response[0] != slave_addr):
            raise ValueError('wrong slave address')

        if (response[1] == (function_code + ERROR_BIAS)):
            raise ValueError('slave returned exception code: {:d}'.format(response[2]))

        hdr_length = (RESPONSE_HDR_LENGTH + 1) if count else RESPONSE_HDR_LENGTH

        return response[hdr_length : len(response) - CRC_LENGTH]

    def read_coils(self, slave_addr, starting_addr, coil_qty):
        modbus_pdu = read_coils(starting_addr, coil_qty)

        resp_data = self._send_receive(modbus_pdu, slave_addr, True)
        status_pdu = self._bytes_to_bool(resp_data)

        return status_pdu

    def read_discrete_inputs(self, slave_addr, starting_addr, input_qty):
        modbus_pdu = read_discrete_inputs(starting_addr, input_qty)

        resp_data = self._send_receive(modbus_pdu, slave_addr, True)
        status_pdu = self._bytes_to_bool(resp_data)

        return status_pdu

    def read_holding_registers(self, slave_addr, starting_addr, register_qty, signed=True):
        modbus_pdu = read_holding_registers(starting_addr, register_qty)

        resp_data = self._send_receive(modbus_pdu, slave_addr, True)
        register_value = self._to_short(resp_data, signed)

        return register_value

    def read_input_registers(self, slave_addr, starting_address, register_quantity, signed=True):
        modbus_pdu = read_input_registers(starting_address, register_quantity)

        resp_data = self._send_receive(modbus_pdu, slave_addr, True)
        register_value = self._to_short(resp_data, signed)

        return register_value

    def write_single_coil(self, slave_addr, output_address, output_value):
        modbus_pdu = write_single_coil(output_address, output_value)

        resp_data = self._send_receive(modbus_pdu, slave_addr, False)
        operation_status = validate_resp_data(resp_data, WRITE_SINGLE_COIL,
                                                        output_address, value=output_value, signed=False)

        return operation_status

    def write_single_register(self, slave_addr, register_address, register_value, signed=True):
        modbus_pdu = write_single_register(register_address, register_value, signed)

        resp_data = self._send_receive(modbus_pdu, slave_addr, False)
        operation_status = validate_resp_data(resp_data, WRITE_SINGLE_REGISTER,
                                                        register_address, value=register_value, signed=signed)

        return operation_status

    def write_multiple_coils(self, slave_addr, starting_address, output_values):
        modbus_pdu = write_multiple_coils(starting_address, output_values)

        resp_data = self._send_receive(modbus_pdu, slave_addr, False)
        operation_status = validate_resp_data(resp_data, WRITE_MULTIPLE_COILS,
                                                        starting_address, quantity=len(output_values))

        return operation_status

    def write_multiple_registers(self, slave_addr, starting_address, register_values, signed=True):
        modbus_pdu = write_multiple_registers(starting_address, register_values, signed)

        resp_data = self._send_receive(modbus_pdu, slave_addr, False)
        operation_status = validate_resp_data(resp_data, WRITE_MULTIPLE_REGISTERS,
                                                        starting_address, quantity=len(register_values))

        return operation_status

    def close(self):
        if self._uart == None:
            return
        try:
            self._uart.deinit()
        except Exception:
            pass
