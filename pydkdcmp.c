/*
 ============================================================================
 Name        : ss_cotton2_lz00.c
 Author      : nanashi
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

struct match {
		Py_ssize_t pos;
		Py_ssize_t match_len;
};

static uint16_t _get_szdecomp(const uint8_t* p_comp)
{
	uint8_t cpy_all = *p_comp;
	uint16_t length;

	if (cpy_all > 1)									// invalid flag, abort
		return 0;

	length = *((uint16_t*) (p_comp + 1));				// get decompressed length of data

	return length;
}

static uint16_t _decomp(const uint8_t* p_comp, uint8_t* p_decomp)
{
	uint8_t cpy_len;
	uint8_t* p_cpy;
	uint16_t length, bit_flags, temp, offset;
	uint8_t cpy_all = *p_comp;

	if (cpy_all > 1)									// invalid flag, abort
		return 0;

	length = *((uint16_t*) (p_comp + 1));				// get decompressed length of data

	if (cpy_all == 0){									// if the flag is null just copy all the data
		memcpy(p_decomp, p_comp, (size_t) length);

		return length+3;
	}

	const uint8_t* p_src = p_comp+3;
	uint8_t* p_dest = p_decomp;

	while ((p_dest - p_decomp) < length){				// while we haven't reached the end of the decompressed data

		bit_flags = *((uint16_t*) p_src);				// get the bit flags for the next 16 bit frame
		p_src += 2;

		for (int i=0; i<16; i++){						// if the bit flag is set
			if (bit_flags & 1){
				temp = *((uint16_t*) p_src);
				p_src += 2;
				offset = (temp & 0x7FF) + 1;			// get buffer offset
				cpy_len = (temp >> 11) + 3;				// get copy size

				p_cpy = p_dest - offset;

				for (int j=0; j<cpy_len; j++){			// copy data from the write buffer
					*p_dest++ = *p_cpy++;
				}

			} else {									// if the bit flag isn't set
				*p_dest++ = *p_src++;					// just copy the data
			}
			bit_flags >>= 1;

			if ((p_dest - p_decomp) >= length)
				break;
		}

	}

	return (uint16_t) (p_src-p_comp);
}

/**
 * @brief	Decompresses Drift King '97 images
 * @param	self			Python self object
 * @param	args			Python arguments
 * @return	PyBytes, int
 */
static PyObject* decomp(PyObject *self, PyObject *args)
{
	/* get arguments from python api */

	const uint8_t* p_comp;
	Py_ssize_t dlength;
    if (!PyArg_ParseTuple(args, "y#", &p_comp, &dlength))
        return NULL;

    uint16_t decomp_size = _get_szdecomp(p_comp);
    uint16_t comp_len;
    uint8_t* p_decomp = (uint8_t *) malloc(decomp_size);
    if (p_decomp == NULL)
        return PyErr_NoMemory();

    comp_len = _decomp(p_comp, p_decomp);

    PyObject* dcomp = PyBytes_FromStringAndSize((char*) p_decomp, sizeof(uint8_t)*decomp_size);
    free(p_decomp);

    return Py_BuildValue("Oi", dcomp, (int) comp_len);
}

static PyMethodDef pydkdcmp_methods[] = {
    {"decomp",  decomp, METH_VARARGS, "Decompresses Drift King '97 images."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef pydkdcmp_module = {
    PyModuleDef_HEAD_INIT,
    "pydkdcmp",   /* name of module */
    "De-/Compression toolset for Drift King '97", /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
	pydkdcmp_methods
};

PyMODINIT_FUNC
PyInit_pydkdcmp(void)
{
    return PyModule_Create(&pydkdcmp_module);
}
