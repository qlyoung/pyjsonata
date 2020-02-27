# pyjsonata

Python bindings for [JSONata](https://jsonata.org).


## Building

Required build tools:
* npm
* clang
* make

If you want to run tests:
* pytest


To build:

```
make
```

To strip symbols (saves about 60k):

```
make strip
```

Or to build in release mode, with optimizations and security flags enabled:

```
make release
```

## Testing

### Pytest

From the repository root:

```
python3 -m pip install pytest
python3 -m pytest
```

### Fuzzing

The Duktape + jsonata.js binary can be built as an executable in-process
fuzzing binary, which allows fuzzing JSONata expressions and input JSON.

To build a target for fuzzing input JSON:

```
make FUZZTARGET=JSON fuzz
```

To build a target for fuzzing JSONata expressions:

```
make FUZZTARGET=EXPRESSION fuzz
```

Omitting `FUZZTARGET` will build the `JSON` target.

By default, these targets are built with ASAN enabled. You can override this
with `CFLAGS=-fno-sanitize=address`, or use others such as UBSan with
`CFLAGS=-fsanitize=address,undefined`.

The usual libFuzzer parameters are accepted. You must also provide, as the last
argument, the path to a file containing either a JSONata expression or an input
JSON, depending on the target. If you built the expression target, provide an
in put JSON for generated expression to process. If you built the JSON target,
provide an expression to use. For example, for the expression target:

```
./translate -rss_limit_mb=0 -jobs=4 -workers=4 ./tests/inputs/test_1.input.json
```

## Usage

```python
from pyjsonata import jsonata

my_expression = "$"
my_json = "{'foo': 'bar'}"

# "{'foo': 'bar'}"
result = jsonata(my_expression, my_json)
```

With exception handling:

```python
from pyjsonata import jsonata, PyjsonataError

my_expression = "$"
my_json = "{'foo': 'bar'}"

# "{'foo': 'bar'}"

try:
    result = jsonata(my_expression, my_json)
except PyjsonataError as e:
    print("Error: ", e)
```


That's it! Return values are always strings.
