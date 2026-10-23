from setuptools import setup, Extension
setup(ext_modules=[Extension('iibin._native', ['native/python.c', 'native/core/wire.c'], include_dirs=['native/core'], extra_compile_args=['-std=c11','-Wall','-Wextra'])])
