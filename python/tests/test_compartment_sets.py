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
    def setUp(self):
        self.json = '''{
            "population": "pop0",
            "compartment_set": [
                [4, 40, 0.9],
                [4, 41, 0.9],
                [5, 30, 0.75]
            ]
        }'''
        self.cs = CompartmentSet(self.json)
    def test_constructor_from_values(self):
        loc = self.cs[0]
        self.assertEqual(loc.node_id, 4)
        self.assertEqual(loc.section_id, 40)
        self.assertAlmostEqual(loc.offset, 0.9)

    def test_comparison_operators(self):
        cs2 = CompartmentSet(self.json)

        # For == and != use self.cs and cs2
        self.assertTrue(self.cs[0] == cs2[0])
        self.assertFalse(self.cs[0] != cs2[0])
        self.assertFalse(self.cs[0] == cs2[1])
        self.assertTrue(self.cs[0] != cs2[1])

        # For other comparisons use self.cs only
        loc1 = self.cs[0]
        loc2 = self.cs[1]
        loc3 = self.cs[0]

        # Less than
        self.assertTrue(loc1 < loc2 or loc2 < loc1)
        self.assertFalse(loc1 < loc3)

        # Less than or equal
        self.assertTrue(loc1 <= loc3)
        self.assertTrue(loc1 <= loc2 or loc2 <= loc1)

        # Greater than
        self.assertTrue(loc2 > loc1 or loc1 > loc2)
        self.assertFalse(loc1 > loc3)

        # Greater than or equal
        self.assertTrue(loc1 >= loc3)
        self.assertTrue(loc1 >= loc2 or loc2 >= loc1)

        # Consistency check for self.cs pairs
        for a, b in [(loc1, loc2), (loc1, loc3)]:
            self.assertTrue((a < b) + (a == b) + (a > b) == 1)

    def test_repr_and_str(self):
        loc = self.cs[0]
        expected = "CompartmentLocation(4, 40, 0.9)"
        self.assertEqual(repr(loc), expected)
        self.assertEqual(str(loc), repr(loc))  # str should delegate to repr


class TestCompartmentSet(unittest.TestCase):
    def setUp(self):
        self.json = '''{
            "population": "pop0",
            "compartment_set": [
                [1, 10, 0.5],
                [2, 20, 0.25],
                [2, 20, 0.26],
                [4, 20, 0.25]
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
        self.assertEqual((loc.node_id, loc.section_id, loc.offset), (1, 10, 0.5))

    def test_getitem_negative_index(self):
        loc = self.cs[-1]
        self.assertEqual((loc.node_id, loc.section_id, loc.offset), (4, 20, 0.25))

    def test_getitem_out_of_bounds_raises(self):
        with self.assertRaises(IndexError):
            _ = self.cs[10]
        with self.assertRaises(IndexError):
            _ = self.cs[-10]

    def test_iterators(self):
        node_ids = [loc.node_id for loc in self.cs]
        self.assertEqual(node_ids, [1, 2, 2, 4])
        node_ids = [loc.node_id for loc in self.cs.filtered_iter([2, 3])]
        self.assertEqual(node_ids, [2, 2])
        conv_to_list = list(self.cs.filtered_iter([2, 3]))
        self.assertTrue(all(isinstance(i, CompartmentLocation) for i in conv_to_list))

    def test_node_ids(self):
        node_ids = self.cs.node_ids()
        self.assertEqual(node_ids, Selection([1, 2, 4]))

    def test_filter_identity(self):
        filtered = self.cs.filter()
        self.assertEqual(filtered.size(), 4)
        filtered = self.cs.filter(Selection([1, 2]))
        self.assertEqual(filtered.size(), 3)

    def test_toJSON_roundtrip(self):
        json_out = self.cs.toJSON()
        cs2 = CompartmentSet(json_out)
        self.assertEqual(cs2, self.cs)

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
        keys = self.cs.names()
        for key in keys:
            self.assertIn(key, self.cs)
        self.assertNotIn('non_existing_key', self.cs)

    def test_getitem(self):
        keys = self.cs.names()
        if keys:
            key = keys[0]
            val = self.cs[key]
            self.assertIsInstance(val, CompartmentSet)

    def test_keys_values_items(self):
        keys = self.cs.names()
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
        cs_file = CompartmentSets.from_file(os.path.join(PATH, 'compartment_sets.json'))
        self.assertEqual(cs_file, self.cs)

    def test_repr_and_str(self):
        r = repr(self.cs)
        s = str(self.cs)
        self.assertTrue(r.startswith("CompartmentSets({"))
        self.assertEqual(s, r)  # str delegates to repr
        # repr should contain keys from the dict
        for key in self.cs.names():
            self.assertIn(str(key), r)
