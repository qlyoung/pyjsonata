import os
import json
import types
import time

import pytest

from ..pyjsonata import jsonata, PyjsonataError

PATH_DATATREES = os.path.join(os.path.dirname(__file__), "inputs")


@pytest.fixture(scope="module")
def config_pairs():
    """
    Generate list of 2-lists. Each inner list has the format
    [
        "{ ... valid json ... }",
        "{ ... expected evaluation ...}",
    ]
    """
    input_jsons = [
        os.path.join(PATH_DATATREES, x)
        for x in os.listdir(PATH_DATATREES)
        if x.endswith(".input.json")
    ]
    outputs = [x.replace(".input.json", ".output") for x in input_jsons]
    expressions = [x.replace(".input.json", ".jsonata") for x in input_jsons]
    pairs = [list(x) for x in zip(input_jsons, outputs, expressions)]
    for pair in pairs:
        with open(pair[0]) as input_json_file:
            pair[0] = input_json_file.read().strip()
        with open(pair[1]) as output_file:
            pair[1] = output_file.read().strip()
        with open(pair[2]) as expression_file:
            pair[2] = expression_file.read().strip()
    return pairs


def is_json(text):
    try:
        json.loads(text)
        return True
    except JSONDecodeException:
        return False


def test_pyjsonata(config_pairs):
    """
    Evaluate a variety of JSON inputs with various expressions, and verify that
    the result matches the expected translation.
    """
    for pair in config_pairs:
        input_json = pair[0]
        output = pair[1]
        expression = pair[2]

        result = jsonata(expression, input_json)

        assert is_json(output) == is_json(result)

        if is_json(output):
            output = json.dumps(json.loads(output), sort_keys=True)
            result = json.dumps(json.loads(result), sort_keys=True)

        # check that the results match
        assert result == output


def test_pyjsonata_bad_json():
    """
    Run random bytes through the translator, and verify that an exception is
    raised.
    """
    with pytest.raises(PyjsonataError):
        garbage = b"{" + os.urandom(256) + b"\x13\x37\x00"
        jsonata("$", garbage)


def test_pyjsonata_no_json():
    """
    Run an empty string through the translator, and verify that we get
    'undefined'.
    """
    assert "undefined" == jsonata("$", "")


def test_pyjsonata_bad_expression():
    """
    Run a garbage expression through the translator, with valid JSON, and
    verify that an exception is raised.
    """
    with pytest.raises(PyjsonataError):
        garbage = b"\x13\x37" + os.urandom(256) + b"\x00"
        jsonata("{}", garbage)
