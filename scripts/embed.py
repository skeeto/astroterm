import sys
import os

# Check if correct number of arguments is provided
if len(sys.argv) != 3:
    print("Usage: python embed.py <input_file> <output_file>")
    sys.exit(1)

# Assign input and output arguments
input_file = sys.argv[1]
output_file = sys.argv[2]

# Extract the base filename without extension (this will be the array name)
array_name = os.path.splitext(os.path.basename(output_file))[0]

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
