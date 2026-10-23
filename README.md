# IIBIN Python

Native C protobuf wire codec with a CPython extension and a small schema registration API. The portable C wire buffer/varint implementation is in `native/core`; `native/python.c` binds Python values. Original PHP/Zend sources under `native/king` are reference only and are not compiled or imported.

```python
from iibin import Schema
message = Schema({'text': {'id': 1, 'type': 'string'}})
message.decode(message.encode({'text': 'Hallo'}))
```

`pip install .` builds the extension. `make build` creates a source distribution and a wheel for the local Python/platform. No PyPI release has been made.

Supported field types: string, bytes, bool, int32/64, uint32/64, sint32/64, fixed32/64, sfixed32/64, float, double, message. Fields may be repeated; nested schemas use `schema`. Numeric repeated fields decode packed and unpacked; encoding is unpacked. Unknown incoming fields are skipped. Absent fields remain absent. Invalid tags, wire mismatches, invalid UTF-8, numeric range violations, non-finite values, nesting above 32 and frames larger than 16 MiB are rejected. Python integers preserve the full 64-bit range.

The JS package has the same wire contract. King enum registries, map/oneof declarations and recursive named schema references are not exposed by these bindings yet. This is not complete API parity with the PHP extension. MIT license.
