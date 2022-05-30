from distutils.core import setup, Extension

module1 = Extension('pydkdcmp',sources = ['pydkdcmp.c'])

setup(
    name='dkdcmp_tools',
    version='1.6',
    python_requires='>3.4',
	packages=['dkdcmp'],
	ext_modules = [module1])