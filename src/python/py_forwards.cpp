/**
 * =============================================================================
 * Viper
 * Copyright (C) 2008-2009 Zach "theY4Kman" Kanzler
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file py_forwards.cpp
 * @brief Exposes the forwards system to Python
 */

#include <Python.h>
#include <structmember.h>
#include "viper_globals.h"
#include "IViperForwardSys.h"
#include "IViperPluginSys.h"
#include "PluginSys.h"
#include "ForwardSys.h"

typedef struct {
    PyObject_HEAD
    IViperForward *fwd;
    char const *fwd_name; /**< This is here so it can be exposed as a member */
} forwards__Forward;

static PyObject *
forwards__Forward__add_function(forwards__Forward *self, PyObject *args)
{
    PyObject *func = NULL;
    if (!PyArg_ParseTuple(args, "O", &func))
        return NULL;
    
    if (!PyCallable_Check(func))
    {
        PyErr_SetString(g_pViperException, "The function passed was not callable");
        return NULL;
    }
    
    GET_THREAD_PLUGIN();
    
    Py_INCREF(func);
    
    IViperPluginFunction *pFunc = CPluginFunction::CreatePluginFunction(func,
        pPlugin);
    
    self->fwd->AddFunction(pFunc);
    
    Py_RETURN_NONE;
}

static PyObject *
forwards__Forward__fire(forwards__Forward *self, PyObject *args)
{
    int result;
    PyObject *py_result = self->fwd->Execute(&result, args);
    
    switch (self->fwd->GetExecType())
    {
    case ET_Single:
        return py_result;
    
    default:
        return PyInt_FromLong(result);
    }
}

static PyObject *
forwards__Forward__remove_function(forwards__Forward *self, PyObject *args)
{
    PyObject *func = NULL;
    if (!PyArg_ParseTuple(args, "O", &func))
        return NULL;
    
    if (!PyCallable_Check(func))
    {
        PyErr_SetString(g_pViperException, "The function passed was not callable");
        return NULL;
    }
    
    GET_THREAD_PLUGIN();
    
    IViperPluginFunction *pFunc = CPluginFunction::CreatePluginFunction(func,
        pPlugin);
       
    bool removed = self->fwd->RemoveFunction(pFunc);
    
    if (removed)
    {
        Py_DECREF(func);
        delete pFunc;
        Py_RETURN_TRUE;
    }
    
    Py_RETURN_FALSE;
}

static PyMemberDef forwards__Forward__members[] = {
    {"name", T_STRING, offsetof(forwards__Forward, fwd_name), READONLY,
        "The name of the forward."},
    {NULL},
};

static PyMethodDef forwards__Forward__methods[] = {
    {"add_function", (PyCFunction)forwards__Forward__add_function, METH_VARARGS,
        "add_function(func)\n\n"
        "Adds a function the the forward's function list.\n"
        "@type  func: callable\n"
        "@param func: The function to add to the list. The function should be able\n"
        "   to handle all the arguments of the forward, though that is NOT checked\n"
        "   automatically."},
    {"fire", (PyCFunction)forwards__Forward__fire, METH_VARARGS,
        "fire([arg1[, ... ]]) -> int\n\n"
        "Fires the forward, passing all arguments passed to fire() to the callbacks\n"
        "@rtype: int\n"
        "@return: Depends on the ExecType of the forward."},
    {"remove_function", (PyCFunction)forwards__Forward__remove_function, METH_VARARGS,
        "remove_function(func) -> bool\n\n"
        "Removes the first instance of the function from the function list.\n"
        "@type  func: callable\n"
        "@param func: The function to remove\n"
        "@rtype: bool\n"
        "@return: True if the function was found and removed, False if the function\n"
        "   was not in the function list."},
    {NULL, NULL, 0, NULL},
};

PyTypeObject forwards__ForwardType = {
    PyObject_HEAD_INIT(NULL)
    0,                          /*ob_size*/
    "sourcemod.forwards.Forward",/*tp_name*/
    sizeof(forwards__Forward),/*tp_basicsize*/
    0,                          /*tp_itemsize*/
    0,                          /*tp_dealloc*/
    0,                          /*tp_print*/
    0,                          /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,                          /*tp_compare*/
    0,                          /*tp_repr*/
    0,                          /*tp_as_number*/
    0,                          /*tp_as_sequence*/
    0,                          /*tp_as_mapping*/
    0,                          /*tp_hash */
    0,                          /*tp_call*/
    0,                          /*tp_str*/
    0,                          /*tp_getattro*/
    0,                          /*tp_setattro*/
    0,                          /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,         /*tp_flags*/
    /* tp_doc */
    "Contains methods to manipulate this forward.",
    0,		                    /* tp_traverse */
    0,		                    /* tp_clear */
    0,		                    /* tp_richcompare */
    0,		                    /* tp_weaklistoffset */
    0,		                    /* tp_iter */
    0,		                    /* tp_iternext */
    forwards__Forward__methods, /* tp_methods */
    forwards__Forward__members, /* tp_members */
    0,                          /* tp_getset */
    0,                          /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    0,                          /* tp_dictoffset */
};

static PyObject *
forwards__register(PyObject *self, PyObject *args)
{
    char *name;
    PyObject *callback;
    
    if (!PyArg_ParseTuple(args, "sO", &name, &callback))
        return NULL;
    
    if (callback != Py_None && !PyCallable_Check(callback))
    {
        PyErr_SetString(g_pViperException, "The callback passed was not callable");
        return NULL;
    }
    
    IViperForward *fwd = g_Forwards.FindForward(name);
    if (fwd == NULL)
        Py_RETURN_FALSE;
    
    GET_THREAD_PLUGIN();
    
    Py_INCREF(callback);
    
    IViperPluginFunction *pFunc = CPluginFunction::CreatePluginFunction(callback,
        pPlugin);
    
    fwd->AddFunction(pFunc);
    
    Py_RETURN_TRUE;
}

static PyObject *
forwards__create(PyObject *self, PyObject *args)
{
    char *name;
    PyObject *callback;
    ViperExecType et;
    
    if (!PyArg_ParseTuple(args, "sOi|", &name, &callback, &et))
        return NULL;
    
    if (callback != Py_None && !PyCallable_Check(callback))
    {
        PyErr_SetString(g_pViperException, "The callback passed was not callable");
        return NULL;
    }
    
    /* Get all the arguments after et */
    PyObject *types = PyTuple_GetSlice(args, 3, PyTuple_Size(args)-1);
    
    // TODO: callback
    IViperForward *fwd = g_Forwards.CreateForward(name, et, types, NULL);
    
    forwards__Forward *py_fwd = PyObject_GC_New(forwards__Forward,
        &forwards__ForwardType);
    
    py_fwd->fwd = fwd;
    py_fwd->fwd_name = fwd->GetForwardName();
    
    return (PyObject*)py_fwd;
}

static PyMethodDef forwards__methods[] = {
    {"register", forwards__register, METH_VARARGS,
        "register(forward, callback) -> bool\n\n"
        "Registers a callback for the specified global forward.\n"
        "@type  forward: string\n"
        "@param forward: The name of the global forward\n"
        "@type  callback: callable\n"
        "@param callback: The function to be called when the forward is fired.\n"
        "@rtype: bool\n"
        "@return: True if successful, false if the specified forward could not be found,\n"
        "   or the forward name passed is blank."},
    {"create", forwards__create, METH_VARARGS,
        "create(name, callback, et) -> Forward object\n\n"
        "Creates a new forward.\n"
        "@note: Pass a blank forward name to create an anonymous forward.\n"
        "@type  name: string\n"
        "@param name: The name of the new forward\n"
        "@type  callback: callable\n"
        "@param callback: A function that will be called every time a function returns\n"
        "   when the forward is fired. The callable should have one parameter: the return\n"
        "   value of the callback. Pass None to ignore this parameter.\n"
        "@type  et: ET constant\n"
        "@param et: How return values of functions are handled when the forward is fired.\n"
        "   Use ET_Ignore, ET_Single, ET_Event, ET_Hook, or ET_LowEvent\n"
        "@rtype: sourcemod.forwards.Forward\n"
        "@return: A Forward object that may manipulate the forward."},
    {NULL, NULL, 0, NULL},
};

PyObject *
initforwards(void)
{
    if (PyType_Ready((PyTypeObject*)&forwards__ForwardType) < 0)
        return NULL;
    
    PyObject *forwards = Py_InitModule3("forwards", forwards__methods,
        "Objects and functions that manipulate the forwards system");
    
    PyModule_AddObject(forwards, "Forward", (PyObject*)&forwards__ForwardType);
    
    PyModule_AddIntMacro(forwards, ET_Ignore);
    PyModule_AddIntMacro(forwards, ET_Single);
    PyModule_AddIntMacro(forwards, ET_Event);
    PyModule_AddIntMacro(forwards, ET_Hook);
    PyModule_AddIntMacro(forwards, ET_LowEvent);
    
    return forwards;
}
