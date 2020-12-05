
__all__ = ["CTRL", "CMD", "TAGS", "PACKETS"]

class CTRL():
    # Control-key characters
    CTRL_A = '\x01'
    CTRL_B = '\x02'
    CTRL_F = '\x06'
    CTRL_H = '\x08'
    CTRL_R = '\x12'
    CTRL_T = '\x14'
    CTRL_Y = '\x19'
    CTRL_P = '\x10'
    CTRL_X = '\x18'
    CTRL_L = '\x0c'
    CTRL_RBRACKET = '\x1d'  # Ctrl+]
    SEND_DATA_KEY = 'w'
    SAVE_DATA_KEY = 's'

class CMD():
    # Command parsed from console inputs
    CMD_STOP = 1
    CMD_RESET = 2
    CMD_MAKE = 3
    CMD_APP_FLASH = 4
    CMD_OUTPUT_TOGGLE = 5
    CMD_TOGGLE_LOGGING = 6
    CMD_ENTER_BOOT = 7
    CMD_SEND_DATA = 8
    CMD_SAVE_DATA = 9

__version__ = "1.1"

# Tags for tuples in queues
class TAGS():
    TAG_KEY = 0
    TAG_SERIAL = 1
    TAG_SERIAL_FLUSH = 2
    TAG_CMD = 3

class PACKETS():
    PACKET_DATA_SIZE = 256
    DATA_HEADER = b'DATA\x00'
    DATA_FOOTER = b'END\n'
    PACKET_SIZE = 256

    # Algorithm Choices
    ALGO_WHITE_V1_0 = b'ALGO_WHITE_V1_0' 
    ALGO_TEST = b'ALGO_TEST'

    # Data Types (bytes-like)
    ONE_BYTE_UNSIGNED = b'U1\0'
    TWO_BYTE_UNSIGNED = b'U2\0'
    UNSIGNED_BYTE_TYPES = [0, ONE_BYTE_UNSIGNED, TWO_BYTE_UNSIGNED]

    ALGORITHM_SELECT = b'AS\0'

class SerialStopException(Exception):
    """
    This exception is used for stopping the IDF monitor in testing mode.
    """
    pass