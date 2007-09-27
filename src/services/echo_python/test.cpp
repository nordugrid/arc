#include <Python.h>

class A {
    protected:
        int b;
    public:
        A(void) { b = 10; };
        void show(void) { printf ("%d\n", b); };
};

int main(void)
{
    // int *ize = (int *)calloc(10, sizeof(int));
    A *ize = new A();
    PyObject *obj;
    PyObject *args;
    PyObject *argv;

    long v = 0;
    
    Py_Initialize();

    printf("%p (%ld)\n", ize, (long int)ize);
    obj = PyLong_FromLong((long int)ize);
    printf("%p\n", obj);
    args = Py_BuildValue("(l)", (long int)ize);
    argv = PyTuple_GET_ITEM(args, 0);
    printf("%p\n", argv);
    v = PyLong_AsLong(argv);
    if (!PyErr_Occurred()) {
        printf("%p (%ld)\n", (int *)v, v);
    } else {
        PyErr_Print();
    }
    return 0;
}
