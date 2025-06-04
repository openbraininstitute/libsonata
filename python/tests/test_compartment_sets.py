import json
import os
import unittest

from libsonata import (
    CompartmentLocation,
    CompartmentSet,
    Selection
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

class TestCompartmentSet(unittest.TestCase):
    def setUp(self):
        self.json = '''{
            "population": "pop0",
            "compartment_set": [
                [1, 10, 0.5],
                [2, 20, 0.25],
                [3, 30, 0.75],
                [2, 20, 0.25]
            ]
        }'''
        self.cs = CompartmentSet(self.json)

    def test_population_property(self):
        self.assertIsInstance(self.cs.population, str)
        self.assertEqual(self.cs.population, "pop0")
    
    def test_size(self):
        self.assertEqual(self.cs.size(), 4)
        self.assertEqual(self.cs.size([1, 2]), 3)
        self.assertEqual(self.cs.size(Selection([[2, 3]])), 2)

    def test_len_dunder(self):
        self.assertEqual(len(self.cs), 4)

    def test_getitem(self):
        loc = self.cs[0]
        self.assertEqual((loc.gid, loc.section_idx, loc.offset), (1, 10, 0.5))

    def test_getitem_negative_index(self):
        loc = self.cs[-1]
        self.assertEqual((loc.gid, loc.section_idx, loc.offset), (2, 20, 0.25))

    def test_getitem_out_of_bounds_raises(self):
        with self.assertRaises(IndexError):
            _ = self.cs[10]
        with self.assertRaises(IndexError):
            _ = self.cs[-10]

    def test_iterators(self):
        gids = [loc.gid for loc in self.cs]
        self.assertEqual(gids, [1, 2, 3, 2])
        gids = [loc.gid for loc in self.cs.filtered_iter([2, 3])]
        self.assertEqual(gids, [2, 3, 2])
        conv_to_list = list(self.cs.filtered_iter([2, 3]))
        self.assertTrue(all(isinstance(i, CompartmentLocation) for i in conv_to_list))

    def test_gids(self):
        gids = self.cs.gids()
        self.assertEqual(gids, Selection([1, 2, 3]))

    def test_filter_identity(self):
        filtered = self.cs.filter()
        self.assertEqual(filtered.size(), 4)
        filtered = self.cs.filter(Selection([1, 2]))
        self.assertEqual(filtered.size(), 3)

    def test_toJSON_roundtrip(self):
        json_out = self.cs.toJSON()
        cs2 = CompartmentSet(json_out)
        self.assertEqual(len(cs2), 4)
        self.assertEqual(cs2.population, self.cs.population)
        self.assertEqual([tuple(loc) for loc in cs2], [tuple(loc) for loc in self.cs])


