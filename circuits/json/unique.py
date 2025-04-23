import json

def find_unique_types(data, unique_types=None):
    if unique_types is None:
        unique_types = set()

    if isinstance(data, dict):
        for key, value in data.items():
            if key == "type" and isinstance(value, str):
                unique_types.add(value)
            else:
                find_unique_types(value, unique_types)
    elif isinstance(data, list):
        for item in data:
            find_unique_types(item, unique_types)

    return unique_types

# Example usage:
with open("parallel-comp-arch/parallel-comp-arch/parallel-sta/circuits/json/largecircuit_easy.json") as f:
    json_data = json.load(f)

unique_types = find_unique_types(json_data)
print("Unique 'type' values found:")
for t in sorted(unique_types):
    print(t)


