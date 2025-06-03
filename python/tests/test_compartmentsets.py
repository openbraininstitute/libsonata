import json
import os
import unittest

from libsonata import (
    CompartmentLocation,
#     CompartmentSet,
#     CompartmentSets,
#     SonataError,
)

PATH = os.path.join(os.path.dirname(os.path.realpath(__file__)),
                    '../../tests/data')

class TestCompartmentLocation(unittest.TestCase):
    def test_constructor_from_values(self):
        loc = CompartmentLocation(4, 40, 0.9)
        self.assertEqual(loc.gid, 4)
        self.assertEqual(loc.section_idx, 40)
        self.assertAlmostEqual(loc.offset, 0.9)

    def test_constructor_from_string(self):
        loc = CompartmentLocation("[4, 40, 0.9]")
        self.assertEqual(loc.gid, 4)
        self.assertEqual(loc.section_idx, 40)
        self.assertAlmostEqual(loc.offset, 0.9)

    def test_toJSON(self):
        loc = CompartmentLocation(4, 40, 0.9)
        self.assertEqual(loc.toJSON(), "[4,40,0.9]")

    def test_equality(self):
        loc1 = CompartmentLocation(4, 40, 0.9)
        loc2 = CompartmentLocation("[4, 40, 0.9]")
        loc3 = CompartmentLocation(5, 40, 0.9)
        self.assertEqual(loc1, loc2)
        self.assertNotEqual(loc1, loc3)

    def test_repr_and_str(self):
        loc = CompartmentLocation(4, 40, 0.9)
        expected = "CompartmentLocation(4, 40, 0.9)"
        self.assertEqual(repr(loc), expected)
        self.assertEqual(str(loc), repr(loc))  # str should delegate to repr

    def test_iterable(self):
        loc = CompartmentLocation(4, 40, 0.9)
        gid, section_idx, offset = loc
        self.assertEqual(gid, 4)
        self.assertEqual(section_idx, 40)
        self.assertAlmostEqual(offset, 0.9)

    def test_assignment_creates_copy(self):
        loc1 = CompartmentLocation(1, 2, 0.3)
        loc2 = loc1  # This is a reference assignment in Python
        self.assertEqual(loc1, loc2)

        # Now mutate loc1 and check if loc2 is affected — which it will be, unless toJSON etc. are implemented with deep semantics
        self.assertIs(loc1, loc2)  # They reference the same object

    def test_explicit_copy(self):
        import copy
        loc1 = CompartmentLocation(1, 2, 0.3)
        loc2 = copy.copy(loc1)
        self.assertEqual(loc1, loc2)
        self.assertIsNot(loc1, loc2)  # Ensure they’re distinct objects

    def test_deepcopy(self):
        import copy
        loc1 = CompartmentLocation(1, 2, 0.3)
        loc2 = copy.deepcopy(loc1)
        self.assertEqual(loc1, loc2)
        self.assertIsNot(loc1, loc2)


# class TestCompartmentSetsFailure(unittest.TestCase):
#     def test_CorrectStructure(self):
#         # Top level must be an object
#         self.assertRaises(SonataError, CompartmentSets, "1")
#         self.assertRaises(SonataError, CompartmentSets, '["array"]')

#         # Each CompartmentSet must be an object with population and compartment_set keys
#         self.assertRaises(SonataError, CompartmentSets, '{ "CompartmentSet0": 1 }')
#         self.assertRaises(SonataError, CompartmentSets, '{ "CompartmentSet0": "string" }')
#         self.assertRaises(SonataError, CompartmentSets, '{ "CompartmentSet0": null }')
#         self.assertRaises(SonataError, CompartmentSets, '{ "CompartmentSet0": true }')

#     def test_MissingPopulationOrCompartmentSet(self):
#         # Missing population key
#         self.assertRaises(SonataError, CompartmentSets, '{ "CompartmentSet0": { "compartment_set": [] } }')
#         # Missing compartment_set key
#         self.assertRaises(SonataError, CompartmentSets, '{ "CompartmentSet0": { "population": "pop" } }')

#     def test_InvalidPopulationType(self):
#         self.assertRaises(SonataError, CompartmentSets, '{ "CompartmentSet0": { "population": 123, "compartment_set": [] } }')
#         self.assertRaises(SonataError, CompartmentSets, '{ "CompartmentSet0": { "population": null, "compartment_set": [] } }')

#     def test_InvalidCompartmentSetType(self):
#         self.assertRaises(SonataError, CompartmentSets, '{ "CompartmentSet0": { "population": "pop", "compartment_set": "not an array" } }')
#         self.assertRaises(SonataError, CompartmentSets, '{ "CompartmentSet0": { "population": "pop", "compartment_set": 123 } }')

#     def test_InvalidCompartmentLocationStructure(self):
#         # Each compartment_set element must be an array of 3 elements [gid, section_idx, location]

#         # Not an array
#         self.assertRaises(SonataError, CompartmentSets, '''
#             {
#                 "CompartmentSet0": {
#                     "population": "pop",
#                     "compartment_set": [ 1 ]
#                 }
#             }
#         ''')

#         # Array with wrong size
#         self.assertRaises(SonataError, CompartmentSets, '''
#             {
#                 "CompartmentSet0": {
#                     "population": "pop",
#                     "compartment_set": [ [1, 0] ]
#                 }
#             }
#         ''')

#         # Wrong types inside element
#         self.assertRaises(SonataError, CompartmentSets, '''
#             {
#                 "CompartmentSet0": {
#                     "population": "pop",
#                     "compartment_set": [ ["not uint64", 0, 0.5] ]
#                 }
#             }
#         ''')


#         self.assertRaises(SonataError, CompartmentSets, '''
#             {
#                 "CompartmentSet0": {
#                     "population": "pop",
#                     "compartment_set": [ [1, "not uint64", 0.5] ]
#                 }
#             }
#         ''')

#         self.assertRaises(SonataError, CompartmentSets, '''
#             {
#                 "CompartmentSet0": {
#                     "population": "pop",
#                     "compartment_set": [ [1, 0, "not a number"] ]
#                 }
#             }
#         ''')

#         # Location out of bounds
#         self.assertRaises(SonataError, CompartmentSets, '''
#             {
#                 "CompartmentSet0": {
#                     "population": "pop",
#                     "compartment_set": [ [1, 0, -0.1] ]
#                 }
#             }
#         ''')

#         self.assertRaises(SonataError, CompartmentSets, '''
#             {
#                 "CompartmentSet0": {
#                     "population": "pop",
#                     "compartment_set": [ [1, 0, 1.1] ]
#                 }
#             }
#         ''')

#     def test_MissingFile(self):
#         self.assertRaises(SonataError, CompartmentSets.from_file, 'this/file/does/not/exist')


# class TestLoadValidCompartmentSets(unittest.TestCase):
#     def test_valid_compartment_sets_file(self):
#         # Load a valid compartment sets file
#         file_path = os.path.join(PATH, 'compartment_sets.json')
#         cs = CompartmentSets.from_file(file_path)

#         self.assertIsInstance(cs, CompartmentSets)
#         self.assertEqual(len(cs), 2)
#         self.assertEqual(['cs0', 'cs1'], cs.keys())
#         self.assertIn('cs0', cs)
#         self.assertFalse('cs2' in cs)
#         self.assertIsInstance(cs['cs0'], CompartmentSet)
#         self.assertEqual(cs['cs0'].population, 'pop0')
#         self.assertEqual(len(cs.values()), 2)
#         self.assertEqual(len(cs.items()), 2)

# class TestCompartmentLocation(unittest.TestCase):
#     def test_construction_and_json_round_trip(self):
#         # Construct using (gid, section_idx, offset)
#         loc = CompartmentLocation(42, 3, 0.75)
#         self.assertEqual(loc.gid, 42)
#         self.assertEqual(loc.section_idx, 3)
#         self.assertAlmostEqual(loc.offset, 0.75)

#         # Convert to JSON string
#         json_str = loc.toJSON()

#         # Construct from JSON string
#         parsed = CompartmentLocation(json_str)
#         self.assertEqual(parsed, loc)

#         # Construct directly from JSON list string representation
#         loc0 = CompartmentLocation("[42, 3, 0.75]")
#         self.assertEqual(loc0, loc)