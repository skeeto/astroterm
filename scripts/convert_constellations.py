import csv
import sys

# Script to convert Stellarium constellations (https://github.com/Stellarium/stellarium/blob/3c8d3c448f82848e9d8c1af307ec4cad20f2a9c0/skycultures/modern/constellationship.fab#L6) from Hipparchus catalogue indices to BSC5 indices using the HYG catalogue (https://www.astronexus.com/projects/hyg)

# 1. Download hygdata_vXX.csv.gz from https://www.astronexus.com/projects/hyg,
# where XX is the downloaded versions
# 2. Unzip via `gzip -d hygdata_vXX.csv.gz`
# 3. Download constellationship.fab from https://github.com/Stellarium/stellarium/blob/3c8d3c448f82848e9d8c1af307ec4cad20f2a9c0/skycultures/modern/constellationship.fab#L6
# 4. python convert_constellations.py constellationship.fab converted.txt hygdata_vXX.csv

# Load the HIP-to-BSC5 mapping from the CSV file
def load_mapping(csv_path):
    hip_to_bsc5 = {}
    with open(csv_path, newline='') as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            hip = row.get('hip')
            hr = row.get('hr')
            if hip and hr:
                hip_to_bsc5[int(hip)] = int(hr)
    return hip_to_bsc5

# Convert a single constellation line
def convert_constellation_line(line, hip_to_bsc5):
    parts = line.split()
    constellation_name = parts[0]
    segment_count = parts[1]
    indices = [int(x) for x in parts[2:]]

    converted_indices = []
    for index in indices:
        if index in hip_to_bsc5:
            converted_indices.append(str(hip_to_bsc5[index]))
        else:
            raise KeyError(f"HIP index {index} not found in the mapping.")

    return f"{constellation_name} {segment_count} {' '.join(converted_indices)}"

# Process the input constellations file
def process_constellations(input_file, output_file, mapping_csv):
    hip_to_bsc5 = load_mapping(mapping_csv)
    with open(input_file, 'r') as infile, open(output_file, 'w') as outfile:
        for line in infile:
            line = line.strip()
            if not line:
                continue  # Skip empty lines
            try:
                converted_line = convert_constellation_line(line, hip_to_bsc5)
                # NOTE: the final line must be a new line
                outfile.write(converted_line + '\n')
            except KeyError as e:
                print(f"Skipping line due to missing mapping: {line}")
                print(f"Error: {e}")

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python convert_constellations.py <input_file> <output_file> <mapping_csv>")
        sys.exit(1)

    input_constellations_file = sys.argv[1]
    output_constellations_file = sys.argv[2]
    mapping_csv_file = sys.argv[3]

    process_constellations(input_constellations_file, output_constellations_file, mapping_csv_file)
    print(f"Converted constellations written to {output_constellations_file}")
