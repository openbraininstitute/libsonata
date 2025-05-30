from libsonata import (
    CompartmentLocation,
    CompartmentSets,
    CompartmentSet,
    SonataError,
    Selection
)

def inspect(v):
    print(v, type(v))
    for i in dir(v):
        # if not i.startswith('__'):
        print(f'  {i} = {getattr(v, i)}')

a = CompartmentSets('{ "CompartmentSet0": { "population": "pop0", "compartment_set": [[0, 10, 0.2], [3, 11, 0.2], [0, 10, 0.201], [1, 11, 0.2]] } }')

print("CompartmentSet" in a)
