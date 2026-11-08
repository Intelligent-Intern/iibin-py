"""Schema-defined protobuf-wire messages. No JSON envelope or content digests."""
from ._native import encode as _encode, decode as _decode

_TYPES = {'string','bytes','bool','int32','uint32','sint32','int64','uint64','sint64','fixed32','sfixed32','fixed64','sfixed64','float','double','message'}

def _validate(schema, depth=0):
    if depth > 32 or not isinstance(schema, dict):
        raise ValueError('Invalid or excessively nested schema')
    result, ids = {}, set()
    for name, original in schema.items():
        f = dict(original)
        i, t = f.get('id'), f.get('type')
        if not isinstance(name,str) or not name or type(i) is not int or not 0<i<2**29 or 19000<=i<=19999 or i in ids or t not in _TYPES:
            raise ValueError(f'Invalid field {name!r}')
        ids.add(i)
        if t == 'message': f['schema'] = _validate(f.get('schema'), depth+1)
        result[name] = f
    return dict(sorted(result.items(), key=lambda item: item[1]['id']))

class Schema:
    def __init__(self, fields):
        self.fields = _validate(fields)
    def encode(self, message):
        return _encode(self.fields, message)
    def decode(self, data):
        return _decode(self.fields, data)

__all__ = ['Schema']
