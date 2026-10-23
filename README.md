# IIBIN Python package

The existing King IIBIN C implementation and its codec/registry fragments are stored under `native/king`, preserving the original paths. They are the source material for the Python binding.

These sources still depend on PHP/Zend. Extracting the portable C codec, adding Python bindings, and packaging wheels are pending; this directory is not yet an installable Python package.
