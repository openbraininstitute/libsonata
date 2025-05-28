import json
import os
import unittest

from libsonata import (
    CompartmentSets,
    SonataError,
)

PATH = os.path.join(os.path.dirname(os.path.realpath(__file__)),
                    '../../tests/data')
class TestCompartmentSetsFailure(unittest.TestCase):
    def test_CorrectStructure(self):
        # Top level must be an object
        self.assertRaises(SonataError, CompartmentSets, "1")
        self.assertRaises(SonataError, CompartmentSets, '["array"]')

        # Each CompartmentSet must be an object with population and compartment_set keys
        self.assertRaises(SonataError, CompartmentSets, '{ "CompartmentSet0": 1 }')
        self.assertRaises(SonataError, CompartmentSets, '{ "CompartmentSet0": "string" }')
        self.assertRaises(SonataError, CompartmentSets, '{ "CompartmentSet0": null }')
        self.assertRaises(SonataError, CompartmentSets, '{ "CompartmentSet0": true }')

    def test_MissingPopulationOrCompartmentSet(self):
        # Missing population key
        self.assertRaises(SonataError, CompartmentSets, '{ "CompartmentSet0": { "compartment_set": [] } }')
        # Missing compartment_set key
        self.assertRaises(SonataError, CompartmentSets, '{ "CompartmentSet0": { "population": "pop" } }')

    def test_InvalidPopulationType(self):
        self.assertRaises(SonataError, CompartmentSets, '{ "CompartmentSet0": { "population": 123, "compartment_set": [] } }')
        self.assertRaises(SonataError, CompartmentSets, '{ "CompartmentSet0": { "population": null, "compartment_set": [] } }')

    def test_InvalidCompartmentSetType(self):
        self.assertRaises(SonataError, CompartmentSets, '{ "CompartmentSet0": { "population": "pop", "compartment_set": "not an array" } }')
        self.assertRaises(SonataError, CompartmentSets, '{ "CompartmentSet0": { "population": "pop", "compartment_set": 123 } }')

    def test_InvalidCompartmentSetElementStructure(self):
        # Each compartment_set element must be an array of 4 elements [gid, section_name, section_index, location]

        # Not an array
        self.assertRaises(SonataError, CompartmentSets, '''
            {
                "CompartmentSet0": {
                    "population": "pop",
                    "compartment_set": [ 1 ]
                }
            }
        ''')

        # Array with wrong size
        self.assertRaises(SonataError, CompartmentSets, '''
            {
                "CompartmentSet0": {
                    "population": "pop",
                    "compartment_set": [ [1, "sec", 0] ]
                }
            }
        ''')

        # Wrong types inside element
        self.assertRaises(SonataError, CompartmentSets, '''
            {
                "CompartmentSet0": {
                    "population": "pop",
                    "compartment_set": [ ["not uint64", "sec", 0, 0.5] ]
                }
            }
        ''')

        self.assertRaises(SonataError, CompartmentSets, '''
            {
                "CompartmentSet0": {
                    "population": "pop",
                    "compartment_set": [ [1, 123, 0, 0.5] ]
                }
            }
        ''')

        self.assertRaises(SonataError, CompartmentSets, '''
            {
                "CompartmentSet0": {
                    "population": "pop",
                    "compartment_set": [ [1, "sec", "not uint64", 0.5] ]
                }
            }
        ''')

        self.assertRaises(SonataError, CompartmentSets, '''
            {
                "CompartmentSet0": {
                    "population": "pop",
                    "compartment_set": [ [1, "sec", 0, "not a number"] ]
                }
            }
        ''')

        # Location out of bounds
        self.assertRaises(SonataError, CompartmentSets, '''
            {
                "CompartmentSet0": {
                    "population": "pop",
                    "compartment_set": [ [1, "sec", 0, -0.1] ]
                }
            }
        ''')

        self.assertRaises(SonataError, CompartmentSets, '''
            {
                "CompartmentSet0": {
                    "population": "pop",
                    "compartment_set": [ [1, "sec", 0, 1.1] ]
                }
            }
        ''')

    def test_MissingFile(self):
        self.assertRaises(SonataError, CompartmentSets.from_file, 'this/file/does/not/exist')


class TestCompartmentSet(unittest.TestCase):
    def setUp(self):
        self.compartment_sets = CompartmentSets.from_file(os.path.join(PATH, "compartment_sets.json"))

    def test_BasicCompartmentIdSelection(self):
        pass
        # sel = self.compartment_sets.materialize("CompartmentSet0", self.storage)
        # self.assertEqual(sel, Selection(((0, 3),)))  # Adapt expected selection

    # def test_InvalidSetName(self):
    #     with self.assertRaises(KeyError):
    #         self.compartment_sets.materialize("UnknownSet", self.storage)

    # def test_EmptySet(self):
    #     sel = self.compartment_sets.materialize("EmptySet", self.storage)
    #     self.assertEqual(sel, Selection([]))

    # def test_CompoundSet(self):
    #     j = {
    #         "SetA": { "compartment_id": [1, 2] },
    #         "SetB": { "compartment_id": [3] },
    #         "CompoundSet": ["SetA", "SetB"]
    #     }
    #     cs = CompartmentSets(json.dumps(j))
    #     sel = cs.materialize("CompoundSet", self.storage)
    #     self.assertEqual(sel, Selection(((1, 2), (3, 4))))  # Adjust to real ranges

    # def test_toJSON_roundtrip(self):
    #     j = json.dumps({
    #         "SetA": { "compartment_id": [0, 1] },
    #         "SetB": { "compartment_id": [2] },
    #         "CompoundSet": ["SetA", "SetB"]
    #     })
    #     cs = CompartmentSets(j)
    #     new = cs.toJSON()
    #     self.assertEqual(cs.toJSON(), CompartmentSets(new).toJSON())

    # def test_update_sets(self):
    #     cs1 = CompartmentSets(json.dumps({"SetA": { "compartment_id": [1] }}))
    #     cs2 = CompartmentSets(json.dumps({"SetB": { "compartment_id": [2] }}))
    #     dup = cs1.update(cs2)
    #     self.assertEqual(dup, set())
    #     self.assertEqual(cs1.names, {"SetA", "SetB"})

    #     cs3 = CompartmentSets(json.dumps({"SetA": { "compartment_id": [1] }}))
    #     dup = cs1.update(cs3)
    #     self.assertEqual(dup, {"SetA"})