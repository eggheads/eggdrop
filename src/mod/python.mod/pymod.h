#define APIDEF_METHOD(symbol)	static PyObject * api_##symbol(PyObject *self, PyObject *args)
#define APIDEF_KWMETHOD(symbol)	static PyObject * api_##symbol(PyObject *self, PyObject *args, PyObject *kw)

#undef putserv
