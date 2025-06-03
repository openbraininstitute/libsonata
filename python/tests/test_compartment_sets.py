import json
import os
import unittest

from libsonata import (
    CompartmentLocation,
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
