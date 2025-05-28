from libsonata import (
    CompartmentSetElement,
    CompartmentSets,
    CompartmentSet,
    SonataError,
)

def inspect(v):
    print(v, type(v))
    for i in dir(v):
        # if not i.startswith('__'):
        print(f'  {i} = {getattr(v, i)}')

a = CompartmentSets('{ "CompartmentSet0": { "population": "pop0", "compartment_set": [[0, "dend", 10, 0.2], [2, "dend", 11, 0.2], [0, "dend", 11, 0.2], [1, "dend", 11, 0.2]] } }')
print(a.compartment_set("CompartmentSet0").gids())