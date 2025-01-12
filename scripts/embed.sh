#!/bin/sh

# Enhanced script with parameters: --source, --header, and --array-name

usage() {
    echo "Usage: $0 --source <input_file> --header <output_header> --array-name <array_name>"
    exit 1
}

# Ensure xxd is available
if ! command -v xxd > /dev/null 2>&1; then
    echo "Error: xxd is not installed or not found in PATH."
    exit 1
fi

# Parse arguments
SOURCE=""
HEADER=""
ARRAY_NAME=""

while [ "$#" -gt 0 ]; do
    case "$1" in
        --source)
            SOURCE="$2"
            shift 2
            ;;
        --header)
            HEADER="$2"
            shift 2
            ;;
        --array-name)
            ARRAY_NAME="$2"
            shift 2
            ;;
        *)
            echo "Unknown parameter: $1"
            usage
            ;;
    esac
done

# Validate required arguments
if [ -z "$SOURCE" ] || [ -z "$HEADER" ] || [ -z "$ARRAY_NAME" ]; then
    echo "Error: Missing required arguments."
    usage
fi

# Generate the header file
xxd -i "$SOURCE" | \
    sed "s/unsigned char [^ ]*/unsigned char ${ARRAY_NAME}[]/" | \
    sed "s/unsigned int [^ ]*/unsigned int ${ARRAY_NAME}_len/" > "$HEADER"

# Check for success
if [ $? -eq 0 ]; then
    echo "Successfully generated header '$HEADER' with array name '${ARRAY_NAME}'"
else
    echo "Error generating header '$HEADER'"
    exit 1
fi
