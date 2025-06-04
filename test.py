from libsonata import (
    CompartmentLocation,
    CompartmentSet,
    CompartmentSets
)

def inspect(v):
    print(v, type(v))
    for i in dir(v):
        # if not i.startswith('__'):
        print(f'  {i} = {getattr(v, i)}')


json = '''{
            "population": "pop0",
            "compartment_set": [
                [1, 10, 0.5],
                [2, 20, 0.25],
                [3, 30, 0.75],
                [2, 20, 0.25]
            ]
        }'''
cs = CompartmentSet(json)
print(cs)

# a = CompartmentSets('{ "CompartmentSet0": { "population": "pop0", "compartment_set": [[0, 10, 0.2], [3, 11, 0.2], [0, 10, 0.201], [1, 11, 0.2]] },  "CompartmentSet1": { "population": "pop1", "compartment_set": [[0, 10, 0.2], [3, 11, 0.2], [0, 10, 0.201], [1, 11, 0.2]] } }')
# b = CompartmentLocation(1, 2, 0.3)
# a = CompartmentLocation(4, 5, 0.6)
# b = a

# print(id(b), b)

# print(id(a), a)

# a = CompartmentSet('''{
#             "population": "pop0",
#             "compartment_set": [
#                 [1, 10, 0.5],
#                 [2, 20, 0.25],
#                 [3, 30, 0.75],
#                 [2, 20, 0.25]
#             ]
#         }''')
print(a)
