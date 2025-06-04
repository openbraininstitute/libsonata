import json
import os
import unittest

from libsonata import (
    CompartmentLocation,
    CompartmentSet,
    CompartmentSets,
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
    
    def test_equality(self):
        cs1 = CompartmentSet(self.json)
        cs2 = CompartmentSet(self.json)
        self.assertEqual(cs1, cs2)
        self.assertFalse(cs1 != cs2)

        # Slightly modify JSON to create a different object
        json_diff = '''{
            "population": "pop0",
            "compartment_set": [
                [1, 10, 0.5],
                [2, 20, 0.25],
                [3, 30, 0.75]
            ]
        }'''
        cs3 = CompartmentSet(json_diff)
        self.assertNotEqual(cs1, cs3)
        self.assertFalse(cs1 == cs3)

    def test_repr_and_str(self):
        r = repr(self.cs)
        s = str(self.cs)
        print(r)
        self.assertTrue(r.startswith("CompartmentSet(population"))
        self.assertEqual(s, r)

class TestCompartmentSets(unittest.TestCase):
    def setUp(self):
        # Load valid json string from file
        with open(os.path.join(PATH, 'compartment_sets.json'), 'r') as f:
            self.json_str = f.read()
        self.cs = CompartmentSets(self.json_str)

    def test_init_from_string(self):
        self.assertIsInstance(self.cs, CompartmentSets)
        self.assertGreater(len(self.cs), 0)


    def test_contains(self):
        keys = self.cs.keys()
        for key in keys:
            self.assertIn(key, self.cs)
        self.assertNotIn('non_existing_key', self.cs)

    def test_getitem(self):
        keys = self.cs.keys()
        if keys:
            key = keys[0]
            val = self.cs[key]
            self.assertIsInstance(val, CompartmentSet)

    def test_keys_values_items(self):
        keys = self.cs.keys()
        values = self.cs.values()
        items = self.cs.items()
        self.assertEqual(len(keys), len(values))
        self.assertEqual(len(keys), len(items))
        for k, v in items:
            self.assertIn(k, keys)
            self.assertIn(v, values)
            
    def test_equality(self):
        cs1 = CompartmentSets(self.json_str)
        cs2 = CompartmentSets(self.json_str)
        self.assertEqual(cs1, cs2)
        self.assertFalse(cs1 != cs2)

        # Modify JSON to create different object
        altered = json.loads(self.json_str)
        if altered:
            # Remove one key if possible
            some_key = list(altered.keys())[0]
            altered.pop(some_key)
            altered_json = json.dumps(altered)
            cs3 = CompartmentSets(altered_json)
            self.assertNotEqual(cs1, cs3)
            self.assertFalse(cs1 == cs3)

    def test_toJSON_roundtrip(self):
        json_out = self.cs.toJSON()
        cs2 = CompartmentSets(json_out)
        self.assertEqual(self.cs, cs2)

    def test_static_fromFile(self):
        cs_file = CompartmentSets.fromFile(os.path.join(PATH, 'compartment_sets.json'))
        self.assertEqual(cs_file, self.cs)

    def test_repr_and_str(self):
        r = repr(self.cs)
        s = str(self.cs)
        self.assertTrue(r.startswith("CompartmentSets({"))
        self.assertEqual(s, r)  # str delegates to repr
        # repr should contain keys from the dict
        for key in self.cs.keys():
            self.assertIn(str(key), r)