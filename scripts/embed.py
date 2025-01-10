import sys
import os
import argparse

def main():
    # Argument parsing
    parser = argparse.ArgumentParser(description="Generate a C header file with embedded binary data.")
    parser.add_argument('--source', required=True, help="Path to the input binary file")
    parser.add_argument('--header', required=True, help="Path to the output header file")
    parser.add_argument('--array-name', required=True, help="Name of the C array")

    args = parser.parse_args()

    input_file = args.source
    output_file = args.header
    array_name = args.array_name

    # Read the binary data from the input file
    try:
        with open(input_file, "rb") as f:
            binary_data = f.read()
    except FileNotFoundError:
        print(f"Error: Input file '{input_file}' not found.")
        sys.exit(1)

    # Generate the C array data (convert bytes to hex)
    hex_data = ', '.join(f'0x{byte:02x}' for byte in binary_data)

    # Get the length of the array
    array_length = len(binary_data)

    # Generate the C code content
    c_code = f"""// Array length for {array_name}
unsigned int {array_name}_len = {array_length};

unsigned char {array_name}[] = {{
    {hex_data}
}};
"""

    # Write the C code to the output file
    try:
        with open(output_file, "w") as f:
            f.write(c_code)
        print(f"Successfully generated {output_file} with array name '{array_name}' and length '{array_name}_len'")
    except IOError as e:
        print(f"Error writing to output file '{output_file}': {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
