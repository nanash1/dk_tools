/*
 ============================================================================
 Author      : nanashi
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

/*****************************************************************************
 * Compression
 ****************************************************************************/

struct match {
		int pos;
		int match_len;
};

static int _find_match(
		const uint8_t* labuff,
		int labuff_len,
		const uint8_t* sbuff,
		int sbuff_start,
		int sbuff_len)
{
	int match;
	int i = 0;

	/* check if there is a match */
	for (i = 0; i < fmin(labuff_len, sbuff_len-sbuff_start); i++){
		if (labuff[i] != sbuff[sbuff_start+i]) break;
	}
	match = i;
	int j = sbuff_start;

	/* check if the found match repeats in the look ahead buffer */
	if ((sbuff_start+i) == sbuff_len){
		while (match < labuff_len){
			if (labuff[match] == sbuff[j]){
				match++;
				j++;
			} else {
				break;
			}

			if (j == sbuff_len) j = sbuff_start;
		}
	}

	return match;
}

static struct match _find_best_match(const uint8_t* labuff, int labuff_len, const uint8_t* sbuff, int sbuff_len)
{
    /* initialize variables */
    struct match ret;
    ret.match_len = 0;
    ret.pos = 0;
    int pos = 0;
    int max_match_len = 0;
    int match_len;

    /* go through the search buffer and look for matches */
    while (pos < sbuff_len){

		match_len = _find_match(labuff, labuff_len, sbuff,
				sbuff_len-1-pos, sbuff_len);

		if (match_len > max_match_len){
			ret.pos = pos+1;
			ret.match_len = match_len,
			max_match_len = match_len;

			if (match_len == labuff_len)
				break;

		}

    	pos++;
    }

    return ret;
}

static uint16_t _comp(const uint8_t* p_data, int size, uint8_t* p_comp)
{
	uint8_t* p_comp_start = p_comp;
	uint16_t symbol;
	struct match match;
	const uint8_t* sbuff = p_data;
	uint16_t* p_flags = (uint16_t*) p_comp;
	p_comp += 2;
	const uint8_t* labuff = p_data;
	int labuff_len = fmin(size, 34);
	int sbuff_len = 0;
	int cntr = 0;
	uint16_t flags = 0;

	while (size){

		match = _find_best_match(labuff, labuff_len, sbuff, sbuff_len);

		if (match.match_len < 3){
			*p_comp++ = *labuff++;
			sbuff_len++;
			--size;
		} else {
			symbol = (match.pos-1) | ((match.match_len-3) << 11);
			*((uint16_t*) p_comp) = symbol;
			p_comp += 2;

			sbuff_len += match.match_len;
			labuff += match.match_len;
			size -= match.match_len;
			flags |= 0x8000;
		}

		labuff_len = fmin(size, 34);
		if (sbuff_len > 2048){
			sbuff += (sbuff_len-2048);
			sbuff_len = 2048;
		}

		if (++cntr > 15){
			cntr = 0;
			*p_flags = flags;
			p_flags = (uint16_t*) p_comp;
			p_comp += 2;
		}
		flags = (flags >> 1);

	}

	if (cntr == 0){
		p_comp -= 2;
	} else {
		*p_flags = flags >> (15 - cntr);
	}

	return p_comp - p_comp_start;
}

/*****************************************************************************
 * Decompression
 ****************************************************************************/

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

/*****************************************************************************
 * Python wrappers
 ****************************************************************************/

/**
 * @brief	Compresses Drift King '97 images
 * @param	self			Python self object
 * @param	args			Python arguments
 * @return	PyBytes, int
 */
static PyObject* comp(PyObject *self, PyObject *args)
{
	/* get arguments from python api */

	const uint8_t* p_data;
	Py_ssize_t dlength;
	uint16_t comp_len;
    if (!PyArg_ParseTuple(args, "y#", &p_data, &dlength))
        return NULL;

    uint8_t* p_comp = (uint8_t *) malloc(fmax(1024, dlength));
    if (p_comp == NULL)
        return PyErr_NoMemory();

    comp_len = _comp(p_data, dlength, p_comp);

    PyObject* dcomp = PyBytes_FromStringAndSize((char*) p_comp, sizeof(uint8_t)*(comp_len));
    free(p_comp);

    return Py_BuildValue("O", dcomp);
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
	{"comp",  comp, METH_VARARGS, "comp(data)\n--\n\nCompresses Drift King '97 images."},
    {"decomp",  decomp, METH_VARARGS, "decomp(data)\n--\n\nDecompresses Drift King '97 images."},
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
