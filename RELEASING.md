# Publishing to PyPI

```bash
make build
.venv/bin/python -m pip install twine
.venv/bin/python -m twine check --strict dist/*.tar.gz dist/*.whl
```

Install the wheel into a fresh environment and run the workspace's
`../interop/check.py` against JavaScript and the rebuilt King extension.
Also test installation from the source distribution in a fresh environment.
The extension currently builds interpreter-specific wheels. A local
`linux_x86_64` wheel is not accepted by PyPI: on Linux, repair and inspect it:

```bash
.venv/bin/python -m pip install auditwheel patchelf
mkdir -p dist/release/2.0.0a2
PATH="$PWD/.venv/bin:$PATH" .venv/bin/python -m auditwheel repair dist/iibin-2.0.0a2-cp312-cp312-linux_x86_64.whl -w dist/release/2.0.0a2
cp dist/iibin-2.0.0a2.tar.gz dist/release/2.0.0a2/
.venv/bin/python -m twine check --strict dist/release/2.0.0a2/*
```

The example uses CPython 3.12 on Linux x86_64; adapt the wheel name for other
builds. Produce other platform/interpreter wheels in their matching build
environments. Other users can build from source with a C compiler and Python
development headers.

Create a PyPI API token and store it locally in the OS keyring:

```bash
.venv/bin/python -m keyring set https://upload.pypi.org/legacy/ __token__
```

Upload only the checked, installation-tested release files:

```bash
.venv/bin/python -m twine upload --username __token__ dist/release/2.0.0a2/*
```

Do not upload the raw local Linux wheel. The version is a prerelease; install
with `pip install --pre iibin`. PyPI release filenames cannot be reused;
increase the version for the next release.
