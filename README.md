# IIBIN Python

> **IIBIN 2.0 compatibility notice:** this package reads and writes schema-defined protobuf wire messages, shared with JavaScript 2.0's `Schema` API and King's schema codec. **It does not decode the older JavaScript 1.x formats.** Existing JS clients and stored payloads need an explicit migration.

**This is an alpha release.** Install with `pip install --pre iibin`.
Version `2.0.0a2` adds explicit migration documentation; its codec and wire
format are unchanged from `2.0.0a1`, the first release of this Python package
on PyPI. The breaking-change warning concerns the older JavaScript 1.x
formats, not an earlier Python 1.x release.

Native C protobuf wire codec with a CPython extension and a small schema registration API. The portable C wire buffer/varint implementation is in `native/core`; `native/python.c` binds Python values. Original PHP/Zend sources under `native/king` are reference only and are not compiled or imported.

```python
from iibin import Schema
message = Schema({'text': {'id': 1, 'type': 'string'}})
message.decode(message.encode({'text': 'Hallo'}))
```

## Connecting existing applications

1. Use `@intelligentintern/iibin@alpha` and its `Schema` API on the JavaScript side. Agree on identical field IDs and types, repeated fields and nested schemas. PHP uses `tag` where Python/JavaScript use `id`.
2. Decode old JavaScript data using its matching codec from `@intelligentintern/iibin/legacy`; map the application fields and re-encode with the new schema. The Python package does not contain a legacy JS decoder.
3. Select the format through endpoint negotiation or stored metadata. Schema messages do not embed a protocol-version header or schema identifier and do not auto-detect legacy data. Do not select a format just by trying decoders until one succeeds.
4. Deploy the accompanying native King extension update for full unsigned 64-bit values and aligned validation. Installing `iibin` with pip does not update PHP's `king.so`. King's schema codec already uses protobuf wire encoding.

The same 64-bit wire value is a Python `int`, a JavaScript `bigint`, or a PHP
integer/decimal string for unsigned values above `PHP_INT_MAX`. Both peers
must preserve it without conversion to a floating-point value. Keep exchanged
messages within 16 MiB and nesting depth 32. King-only schema APIs such as
maps, oneof and enum registries are not exposed by these Python/JS bindings.

Install the 2.0 prerelease with `pip install --pre iibin`. `pip install .` builds the extension from source. A C compiler and Python development headers are needed when a wheel is unavailable for your interpreter/platform. `make build` creates a source distribution and a local wheel; see [RELEASING.md](RELEASING.md) for PyPI uploads.

Supported field types: string, bytes, bool, int32/64, uint32/64, sint32/64, fixed32/64, sfixed32/64, float, double, message. Fields may be repeated; nested schemas use `schema`. Numeric repeated fields decode packed and unpacked; encoding is unpacked. Unknown incoming fields are skipped. Absent fields remain absent. Invalid tags, wire mismatches, invalid UTF-8, numeric range violations, non-finite values, nesting above 32 and frames larger than 16 MiB are rejected. Python integers preserve the full 64-bit range.

The JS package has the same wire contract. King enum registries, map/oneof declarations and recursive named schema references are not exposed by these bindings yet. This is not complete API parity with the PHP extension. MIT license.

Source: [iibin-py](https://github.com/Intelligent-Intern/iibin-py). TypeScript codec: [iibin-js](https://github.com/Intelligent-Intern/iibin-js). Lunar Charge Control and Shopfloor demo: [iibin-demo](https://github.com/Intelligent-Intern/iibin-demo).
