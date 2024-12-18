#!/bin/sh

# Since some implementations of xxd don't have the -n option to specify the name
# of the data array, we must take it upon ourselves to rename it ourselves
# FIXME: this is very hacky :(. Not sure how portable it is
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <input_file> <output_file>"
    exit 1
fi

INPUT="$1"
OUTPUT="$2"

# Extract the base filename without extension (this will be the array name)
ARRAY_NAME=$(basename "$OUTPUT" | sed 's/\(.*\)\..*/\1/')

xxd -i "$INPUT" | \
    sed "s/unsigned char [^ ]*/unsigned char ${ARRAY_NAME}[]/" | \
    sed "s/unsigned int [^ ]*/unsigned int ${ARRAY_NAME}_len/" > "$OUTPUT"

# Check for success
if [ $? -eq 0 ]; then
    echo "Successfully generated $OUTPUT with array name '${ARRAY_NAME}_array'"
else
    echo "Error generating $OUTPUT"
    exit 1
fi
