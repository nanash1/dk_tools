from distutils.core import setup, Extension

module1 = Extension('pydkdcmp',sources = ['pydkdcmp.c'])

setup(
	name='dkdcmp_tools',
	version='1.2',
	packages=['dkdcmp'],
	ext_modules = [module1])