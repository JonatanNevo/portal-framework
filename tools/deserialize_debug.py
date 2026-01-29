#!/usr/bin/env python3
"""
Portal Framework Binary Deserializer Debug Tool

Parses hex dump input in the format:
    0000: 50 53 01 80 ...
    0020: ...

And prints the deserialized properties.

String ID Map:
    Provide a mapping file with lines like:
        0x0db85f495cb4e4b0 = "SomeTypeName"
    Or:
        994927812739401904 = "SomeTypeName"
"""

import struct
import sys
import re
import argparse
from enum import IntEnum
from typing import Optional


class PropertyType(IntEnum):
    BINARY = 0
    INTEGER8 = 1
    INTEGER16 = 2
    INTEGER32 = 3
    INTEGER64 = 4
    INTEGER128 = 5
    FLOATING32 = 6
    FLOATING64 = 7
    CHARACTER = 8
    BOOLEAN = 9
    OBJECT = 10
    NULL_TERM_STRING = 11
    STRING = 12
    INVALID = 255


class PropertyContainerType(IntEnum):
    INVALID = 0
    SCALAR = 1
    ARRAY = 2
    STRING = 3
    NULL_TERM_STRING = 4
    VECTOR = 5
    MATRIX = 6
    OBJECT = 7


TYPE_SIZES = {
    PropertyType.BINARY: 1,
    PropertyType.INTEGER8: 1,
    PropertyType.INTEGER16: 2,
    PropertyType.INTEGER32: 4,
    PropertyType.INTEGER64: 8,
    PropertyType.INTEGER128: 16,
    PropertyType.FLOATING32: 4,
    PropertyType.FLOATING64: 8,
    PropertyType.CHARACTER: 1,
    PropertyType.BOOLEAN: 1,
    PropertyType.OBJECT: 0,
    PropertyType.NULL_TERM_STRING: 0,
    PropertyType.STRING: 0,
    PropertyType.INVALID: 0,
}


# Global string ID map: uint64 -> string
STRING_ID_MAP: dict[int, str] = {}


def load_string_id_map(filepath: str) -> dict[int, str]:
    """
    Load string ID map from file.

    Supported formats:
        0x0db85f495cb4e4b0 = "SomeTypeName"
        0x0db85f495cb4e4b0 = SomeTypeName
        994927812739401904 = "SomeTypeName"
        0x0db85f495cb4e4b0: SomeTypeName
        0x0db85f495cb4e4b0 SomeTypeName
    """
    id_map = {}
    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#') or line.startswith('//'):
                continue

            # Try different formats
            # Format: key = "value" or key = value or key: value or key value
            match = re.match(r'(0x[0-9a-fA-F]+|\d+)\s*[=:,\s]\s*"?([^"]+)"?', line)
            if match:
                key_str, value = match.groups()
                if key_str.startswith('0x'):
                    key = int(key_str, 16)
                else:
                    key = int(key_str)
                id_map[key] = value.strip()

    return id_map


def lookup_string_id(value: int) -> Optional[str]:
    """Look up a string ID in the global map."""
    return STRING_ID_MAP.get(value)


def parse_hex_dump(hex_input: str) -> bytes:
    """Parse hex dump format into bytes."""
    result = bytearray()
    for line in hex_input.strip().split('\n'):
        line = line.strip()
        if not line:
            continue
        # Remove offset prefix (e.g., "0000: ")
        if ':' in line:
            line = line.split(':', 1)[1]
        # Parse hex bytes
        hex_bytes = line.split()
        for hb in hex_bytes:
            hb = hb.strip()
            if len(hb) == 2:
                try:
                    result.append(int(hb, 16))
                except ValueError:
                    pass
    return bytes(result)


def format_value(prop_type: PropertyType, data: bytes, elements: int) -> str:
    """Format the value for display based on property type."""
    if not data:
        return "<empty>"

    element_size = TYPE_SIZES.get(prop_type, 0)

    if prop_type == PropertyType.INTEGER8:
        if elements == 1:
            return str(struct.unpack('<b', data)[0])
        return str([struct.unpack('<b', data[i:i+1])[0] for i in range(elements)])

    elif prop_type == PropertyType.INTEGER16:
        if elements == 1:
            return str(struct.unpack('<h', data)[0])
        return str([struct.unpack('<h', data[i*2:(i+1)*2])[0] for i in range(elements)])

    elif prop_type == PropertyType.INTEGER32:
        if elements == 1:
            return str(struct.unpack('<i', data)[0])
        return str([struct.unpack('<i', data[i*4:(i+1)*4])[0] for i in range(elements)])

    elif prop_type == PropertyType.INTEGER64:
        if elements == 1:
            val = struct.unpack('<Q', data)[0]  # Use unsigned for string ID lookup
            string_name = lookup_string_id(val)
            if string_name:
                return f'"{string_name}" (0x{val:016x})'
            return f"0x{val:016x}"
        else:
            values = []
            for i in range(elements):
                val = struct.unpack('<Q', data[i*8:(i+1)*8])[0]
                string_name = lookup_string_id(val)
                if string_name:
                    values.append(f'"{string_name}"')
                else:
                    values.append(f"0x{val:016x}")
            return "[" + ", ".join(values) + "]"

    elif prop_type == PropertyType.INTEGER128:
        # Display as two 64-bit values or hex
        if elements == 1:
            low = struct.unpack('<Q', data[0:8])[0]
            high = struct.unpack('<Q', data[8:16])[0]
            return f"0x{high:016x}{low:016x}"
        values = []
        for i in range(elements):
            low = struct.unpack('<Q', data[i*16:i*16+8])[0]
            high = struct.unpack('<Q', data[i*16+8:i*16+16])[0]
            values.append(f"0x{high:016x}{low:016x}")
        return str(values)

    elif prop_type == PropertyType.FLOATING32:
        if elements == 1:
            return f"{struct.unpack('<f', data)[0]:.6g}"
        return "[" + ", ".join(f"{struct.unpack('<f', data[i*4:(i+1)*4])[0]:.6g}" for i in range(elements)) + "]"

    elif prop_type == PropertyType.FLOATING64:
        if elements == 1:
            return f"{struct.unpack('<d', data)[0]:.10g}"
        return str([struct.unpack('<d', data[i*8:(i+1)*8])[0] for i in range(elements)])

    elif prop_type == PropertyType.BOOLEAN:
        if elements == 1:
            return str(bool(data[0]))
        return str([bool(data[i]) for i in range(elements)])

    elif prop_type == PropertyType.CHARACTER:
        try:
            return repr(data.decode('utf-8'))
        except:
            return repr(data)

    elif prop_type == PropertyType.NULL_TERM_STRING:
        try:
            # Find null terminator
            null_idx = data.find(b'\x00')
            if null_idx >= 0:
                return repr(data[:null_idx].decode('utf-8'))
            return repr(data.decode('utf-8'))
        except:
            return repr(data)

    elif prop_type == PropertyType.STRING:
        try:
            return repr(data.decode('utf-8'))
        except:
            return repr(data)

    elif prop_type == PropertyType.BINARY:
        return data.hex()

    return data.hex()


class BinaryDeserializer:
    def __init__(self, data: bytes):
        self.data = data
        self.pos = 0
        self.large_element_size = False
        self.pack_elements = False
        self.property_index = 0

    def read(self, n: int) -> bytes:
        if self.pos + n > len(self.data):
            raise EOFError(f"Attempted to read {n} bytes at position {self.pos}, but only {len(self.data) - self.pos} bytes remaining")
        result = self.data[self.pos:self.pos + n]
        self.pos += n
        return result

    def read_header(self) -> dict:
        """Read and validate the 4-byte header."""
        header_bytes = self.read(4)
        magic = header_bytes[0:2]
        version = header_bytes[2]
        params = header_bytes[3]

        if magic != b'PS':
            raise ValueError(f"Invalid magic: {magic!r}, expected b'PS'")
        if version != 1:
            raise ValueError(f"Invalid version: {version}, expected 1")

        self.large_element_size = bool(params & 0x01)
        encode_header = bool(params & 0x80)

        return {
            'magic': magic.decode('ascii'),
            'version': version,
            'encode_header': encode_header,
            'large_element_size': self.large_element_size,
        }

    def should_encode_element_number(self, container_type: PropertyContainerType) -> bool:
        if container_type == PropertyContainerType.SCALAR:
            return False
        if container_type in (PropertyContainerType.VECTOR, PropertyContainerType.MATRIX):
            return not self.pack_elements
        return True

    def read_property(self) -> dict:
        """Read a single property."""
        start_pos = self.pos

        container_type_byte = self.read(1)[0]
        type_byte = self.read(1)[0]

        try:
            container_type = PropertyContainerType(container_type_byte)
        except ValueError:
            container_type = PropertyContainerType.INVALID

        try:
            prop_type = PropertyType(type_byte)
        except ValueError:
            prop_type = PropertyType.INVALID

        element_size = TYPE_SIZES.get(prop_type, 0)
        elements_number = 1

        if container_type != PropertyContainerType.SCALAR:
            if self.should_encode_element_number(container_type):
                if self.large_element_size:
                    elements_number = struct.unpack('<Q', self.read(8))[0]
                else:
                    elements_number = struct.unpack('<H', self.read(2))[0]

        # Calculate data size
        if prop_type == PropertyType.NULL_TERM_STRING:
            # Read until null terminator
            data_start = self.pos
            while self.pos < len(self.data) and self.data[self.pos] != 0:
                self.pos += 1
            if self.pos < len(self.data):
                self.pos += 1  # Skip null terminator
            data = self.data[data_start:self.pos]
        elif element_size > 0:
            data_size = elements_number * element_size
            data = self.read(data_size)
        else:
            data = b''

        self.property_index += 1

        return {
            'index': self.property_index - 1,
            'offset': start_pos,
            'container_type': container_type,
            'type': prop_type,
            'elements': elements_number,
            'data': data,
            'value': format_value(prop_type, data, elements_number),
        }

    def deserialize_all(self) -> list:
        """Deserialize all properties from the buffer."""
        header = self.read_header()
        print(f"Header: magic={header['magic']!r}, version={header['version']}, "
              f"large_element_size={header['large_element_size']}")
        print("-" * 80)

        properties = []
        while self.pos < len(self.data):
            # Check for padding (0xCD bytes often used as uninitialized memory marker)
            if self.data[self.pos] == 0xCD:
                # Check if rest is all padding
                remaining = self.data[self.pos:]
                if all(b == 0xCD for b in remaining):
                    print(f"\n[Offset 0x{self.pos:04X}] Padding: {len(remaining)} bytes of 0xCD")
                    break

            try:
                prop = self.read_property()
                properties.append(prop)
            except EOFError as e:
                print(f"\nEnd of data at offset 0x{self.pos:04X}: {e}")
                break
            except Exception as e:
                print(f"\nError at offset 0x{self.pos:04X}: {e}")
                break

        return properties


def main():
    parser = argparse.ArgumentParser(description='Portal Framework Binary Deserializer Debug Tool')
    parser.add_argument('hexfile', nargs='?', help='File containing hex dump (reads from stdin if not provided)')
    parser.add_argument('-m', '--map', '--string-map', dest='string_map',
                        help='File containing string ID map (uint64 -> string)')
    parser.add_argument('-i', '--inline-map', dest='inline_map', action='append', default=[],
                        help='Inline string ID mapping, e.g., "0x1234=MyString" (can be repeated)')
    args = parser.parse_args()

    # Load string ID map
    global STRING_ID_MAP
    if args.string_map:
        STRING_ID_MAP = load_string_id_map(args.string_map)
        print(f"Loaded {len(STRING_ID_MAP)} string ID mappings from {args.string_map}")

    # Add inline mappings
    for mapping in args.inline_map:
        match = re.match(r'(0x[0-9a-fA-F]+|\d+)\s*[=:]\s*"?([^"]+)"?', mapping)
        if match:
            key_str, value = match.groups()
            if key_str.startswith('0x'):
                key = int(key_str, 16)
            else:
                key = int(key_str)
            STRING_ID_MAP[key] = value.strip()

    if STRING_ID_MAP:
        print(f"Total string ID mappings: {len(STRING_ID_MAP)}")
        print()

    # Read input
    if args.hexfile:
        with open(args.hexfile, 'r') as f:
            hex_input = f.read()
    else:
        print("Paste hex dump (press Ctrl+D or Ctrl+Z when done):")
        hex_input = sys.stdin.read()

    # Parse hex dump to bytes
    data = parse_hex_dump(hex_input)
    print(f"\nParsed {len(data)} bytes\n")

    # Deserialize
    deserializer = BinaryDeserializer(data)
    properties = deserializer.deserialize_all()

    # Print properties
    print(f"\nFound {len(properties)} properties:\n")
    for prop in properties:
        container_name = prop['container_type'].name
        type_name = prop['type'].name

        # Shorten common patterns for readability
        if prop['container_type'] == PropertyContainerType.SCALAR:
            container_str = "scalar"
        elif prop['container_type'] == PropertyContainerType.VECTOR:
            container_str = f"vec{prop['elements']}"
        elif prop['container_type'] == PropertyContainerType.ARRAY:
            container_str = f"array[{prop['elements']}]"
        elif prop['container_type'] == PropertyContainerType.NULL_TERM_STRING:
            container_str = "cstring"
        else:
            container_str = f"{container_name}[{prop['elements']}]"

        type_str = type_name.lower().replace('integer', 'i').replace('floating', 'f')

        print(f"[{prop['index']:3d}] 0x{prop['offset']:04X}: {container_str:15} {type_str:12} = {prop['value']}")


if __name__ == '__main__':
    main()