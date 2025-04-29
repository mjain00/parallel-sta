import subprocess

# List of circuit names
circuits = [
    'simple',
    'adder',
    'longpath',
    'balanced',
    'mult',
    'bigadder',
    'bigcircuit'
]

# Loop over circuit
# Open one output file
with open("all_out.txt", "w") as outfile:
    for circuit in circuits:
        print(f"Running for {circuit}...")
        outfile.write(f"=== {circuit} ===\n")  # Write header for clarity
        subprocess.run(["./sta.o", f"circuits/json/{circuit}.json"], stdout=outfile)
        outfile.write("\n\n")  # Separate outputs
