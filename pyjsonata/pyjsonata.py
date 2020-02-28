#!/usr/bin/env python3

import os
import json

from ctypes import c_char_p, byref, CDLL, POINTER

THIS_MODULE_DIR = os.path.dirname(__file__)
TRANSLATE_SO_PATH = os.path.join(THIS_MODULE_DIR, [ x for x in os.listdir(THIS_MODULE_DIR) if x.endswith(".so") ][0])

_clib_jsonata = CDLL(TRANSLATE_SO_PATH)

_clib_jsonata.jsonata.argtypes = (
    c_char_p,
    c_char_p,
    POINTER(c_char_p),
    POINTER(c_char_p),
)
_clib_jsonata.free_result.argtypes = (c_char_p,)


class PyjsonataError(Exception):
    pass


def jsonata(expression, input):
    if type(expression) is not bytes:
        expression = bytes(expression, encoding="utf-8")
    if type(input) is not bytes:
        input = bytes(input, encoding="utf-8")

    result_p = (c_char_p)()
    error_p = (c_char_p)()

    return_code = int(
        _clib_jsonata.jsonata(expression, input, byref(result_p), byref(error_p))
    )

    if return_code != 0:
        raise PyjsonataError(error_p.value.decode("utf8"))
    else:
        result = str(result_p.value.decode("utf8"))

    _clib_jsonata.free_result(result_p)

    return result
