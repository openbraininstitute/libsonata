import json
import os
import unittest

from libsonata import (
    CompartmentSets,
    SonataError,
)

class TestCompartmentSetsFailure(unittest.TestCase):
    def test_CorrectStructure(self):
        self.assertRaises(SonataError, CompartmentSets, "1")