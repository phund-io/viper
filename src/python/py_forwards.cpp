/**
 * =============================================================================
 * Viper
 * Copyright (C) 2012 PimpinJuice
 * Copyright (C) 2007-2012 Zach "theY4Kman" Kanzler
 * Copyright (C) 2004-2007 AlliedModders LLC.
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
#include "systems/ViperPluginSys.h"
#include "ForwardSys.h"
#include <sh_list.h>

using SourceHook::List;

namespace Viper {
	namespace Python {
		typedef struct {
			PyObject_HEAD
			IViperForward *fwd;
			char const *fwd_name; /**< This is here so it can be exposed as a member */
			IViperPluginFunction *callback;
		} forwards__Forward;

		static List<forwards__Forward*> g_ForwardCallbacks;

		void
		AddToForwardCallbacks(forwards__Forward *fwd)
		{
			g_ForwardCallbacks.push_back(fwd);
		}

		void
		RemoveFromForwardCallbacks(forwards__Forward *fwd)
		{
			g_ForwardCallbacks.remove(fwd);
		}

		ViperResultType
		ForwardCallback(IViperForward *fwd, PyObject *result, IViperPluginFunction *func)
		{
			List<forwards__Forward*>::iterator iter;
			for (iter=g_ForwardCallbacks.begin(); iter!=g_ForwardCallbacks.end(); iter++)
			{
				if ((*iter)->fwd == fwd)
					return (*iter)->callback->ForwardCallback(fwd, result, func);
			}
    
			return Pl_Continue;
		}

		static PyObject *
		forwards__Forward__add_function(forwards__Forward *self, PyObject *args)
		{
			PyObject *func = NULL;
			if (!PyArg_ParseTuple(args, "O", &func))
				return NULL;
    
			if (!PyCallable_Check(func))
			{
				/* sawce would pass a non-callable object. */
				PyErr_SetString(g_pViperException, "The function passed was not callable");
				return NULL;
			}
    
			GET_THREAD_PLUGIN();
    
			IViperPluginFunction *pFunc = CPluginFunction::CreatePluginFunction(func,
				pPlugin);
    
			self->fwd->AddFunction(pFunc);
			delete pFunc;
    
			Py_RETURN_NONE;
		}

		static PyObject *
		forwards__Forward__fire(forwards__Forward *self, PyObject *args)
		{
			int result;
			PyObject *py_result = self->fwd->Execute(&result, args);
    
			if (py_result == NULL && PyErr_Occurred())
				return NULL;
    
			switch (self->fwd->GetExecType())
			{
			case ET_Single:
			case ET_Ignore:
				Py_RETURN_NONE;
    
			default:
				return PyInt_FromLong(result);
			}
		}

		static PyObject *
		forwards__Forward__get_function_count(forwards__Forward *self)
		{
			return PyInt_FromLong(self->fwd->GetFunctionCount());
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
			delete pFunc;
    
			return PyBool_FromLong(removed);
		}

		static void
		forwards__Forward__del__(forwards__Forward *self)
		{
			RemoveFromForwardCallbacks(self);
			delete self->callback;
    
			((PyObject *)self)->ob_type->tp_free((PyObject *)self);
		}

		static int
		forwards__Forward__contains__(forwards__Forward *self, PyObject *value)
		{
			/* Forwards only contain functions, so it's pointless to iterate through
			 * all the forward's functions to find what's not going to be there
			 */
			if (!PyCallable_Check(value))
				return 0;
    
			for (int i=0, length=self->fwd->GetFunctionCount(); i<length; i++)
			{
				IViperPluginFunction *pfunc = self->fwd->GetFunction(i);
				assert(pfunc != NULL);
        
				PyObject *pyfunc = pfunc->GetFunction();
				assert(pyfunc != NULL);
        
				if (pyfunc == value)
					return 1;
			}
    
			return 0;
		}

		static PyObject *
		forwards__Forward__item__(forwards__Forward *self, Py_ssize_t i)
		{
			/* Damn compiler complaining about signedness comparison */
			if ((unsigned int)(i < 0 ? 0 : i) >= self->fwd->GetFunctionCount())
			{
				PyErr_SetString(_PyExc_IndexError, "list index out of range");
				return NULL;
			}
    
			IViperPluginFunction *pfunc = self->fwd->GetFunction(i);
			assert(pfunc != NULL);
    
			PyObject *pyfunc = pfunc->GetFunction();
			assert(pyfunc != NULL);
    
			Py_INCREF(pyfunc);
    
			return pyfunc;
		}

		static Py_ssize_t
		forwards__Forward__len__(forwards__Forward *self)
		{
			return self->fwd->GetFunctionCount();
		}

		static PyObject *
		forwards__Forward__str__(forwards__Forward *self)
		{
			if (!IS_STR_FILLED(self->fwd_name))
				return PyString_FromFormat("<anonymous Forward: %p>", self);
			else
				return PyString_FromFormat("<Forward \"%s\": %p>", self->fwd_name, self);
		}

		static PyMemberDef forwards__Forward__members[] = {
			{"name", T_STRING, offsetof(forwards__Forward, fwd_name), READONLY,
				"The name of the forward."},
			{NULL},
		};

		static PyMethodDef forwards__Forward__methods[] = {
			{"add_function", (PyCFunction)forwards__Forward__add_function, METH_VARARGS,
				"add_function(func)\n\n"
				"Adds a function to the the forward's function list.\n"
				"@type  func: callable\n"
				"@param func: The function to add to the list. The function should be able\n"
				"   to handle all the arguments of the forward, though that is NOT checked\n"
				"   automatically."},
			{"fire", (PyCFunction)forwards__Forward__fire, METH_VARARGS,
				"fire([arg1[, ... ]]) -> int\n\n"
				"Fires the forward, passing all arguments passed to fire() to the callbacks\n"
				"@rtype: int\n"
				"@return: Depends on the ExecType of the forward."},
			{"get_function_count", (PyCFunction)forwards__Forward__get_function_count, METH_NOARGS,
				"Returns the number of callbacks registered on this forward\n"
				"@rtype: int\n"
				"@return: Length of the callbacks list"},
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

		static PySequenceMethods forwards__ForwardSequenceType = {
			(lenfunc)forwards__Forward__len__, /*sq_length*/
			0,                          /*sq_concat*/
			0,                          /*sq_repeat*/
			(ssizeargfunc)forwards__Forward__item__,/*sq_item*/
			0,                          /*sq_slice*/
			0,                          /*sq_ass_item*/
			0,                          /*sq_ass_slice*/
			(objobjproc)forwards__Forward__contains__,/*sq_contains*/
			0,                          /*sq_inplace_concat*/
			0,                          /*sq_inplace_repeat*/
		};

		PyTypeObject forwards__ForwardType = {
			PyObject_HEAD_INIT(NULL)
			0,                          /*ob_size*/
			"sourcemod.forwards.Forward",/*tp_name*/
			sizeof(forwards__Forward),  /*tp_basicsize*/
			0,                          /*tp_itemsize*/
			(destructor)forwards__Forward__del__,/*tp_dealloc*/
			0,                          /*tp_print*/
			0,                          /*tp_getattr*/
			0,                          /*tp_setattr*/
			0,                          /*tp_compare*/
			0,                          /*tp_repr*/
			0,                          /*tp_as_number*/
			&forwards__ForwardSequenceType,/*tp_as_sequence*/
			0,                          /*tp_as_mapping*/
			0,                          /*tp_hash */
			0,                          /*tp_call*/
			(reprfunc)forwards__Forward__str__,/*tp_str*/
			0,                          /*tp_getattro*/
			0,                          /*tp_setattro*/
			0,                          /*tp_as_buffer*/
			Py_TPFLAGS_DEFAULT,         /*tp_flags*/
			/* tp_doc */
			"Contains methods to manipulate a forward.",
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
				return PyErr_Format(g_pViperException, "Forward '%s' does not exist", name);
    
			GET_THREAD_PLUGIN();
    
			IViperPluginFunction *pFunc = CPluginFunction::CreatePluginFunction(callback,
				pPlugin);
    
			fwd->AddFunction(pFunc);
			delete pFunc;
    
			Py_RETURN_TRUE;
		}

		static PyObject *
		forwards__create(PyObject *self, PyObject *args)
		{
			char *name;
			PyObject *callback;
    
			if (PyTuple_Size(args) < 3)
			{
				PyErr_SetString(_PyExc_TypeError, "forwards.create takes at least 3 arguments");
				return NULL;
			}
    
			PyObject *pyname = PyTuple_GetItem(args, 0);
			if (!PyString_Check(pyname))
				return PyErr_Format(_PyExc_TypeError, "argument 1 must be string, not %s",
					pyname->ob_type->tp_name);
			name = PyString_AS_STRING(pyname);
    
			callback = PyTuple_GetItem(args, 1);
			if (!PyCallable_Check(callback) && callback != Py_None)
				return PyErr_Format(_PyExc_TypeError, "argument 2 must be callable");
    
			PyObject *pyet = PyTuple_GetItem(args, 2);
			if (!PyInt_Check(pyet))
				return PyErr_Format(_PyExc_TypeError, "argument 3 must be int, not %s",
					pyet->ob_type->tp_name);
			int int_et = PyInt_AS_LONG(pyet);
    
			if (int_et < ET_Ignore || int_et > ET_LowEvent)
				return PyErr_Format(_PyExc_TypeError, "argument 3 must be a valid ExecType");
    
			/* Get all the arguments after et */
			PyObject *types = PyTuple_GetSlice(args, 3, PyTuple_Size(args));
    
			IViperPluginFunction *pFunc = NULL;
			IViperForwardCallback fwd_callback = NULL;
			if (callback != Py_None)
			{
				GET_THREAD_PLUGIN();
				pFunc = CPluginFunction::CreatePluginFunction(callback, pPlugin);
				fwd_callback = ForwardCallback;
			}
    
			IViperForward *fwd = g_Forwards.CreateForward(name, (ViperExecType)int_et, types, fwd_callback);
			assert(fwd != NULL);
    
			/* Create a new Python Forward object */
			forwards__Forward *py_fwd = PyObject_New(forwards__Forward,
				&forwards__ForwardType);
			assert(py_fwd != NULL);
    
			py_fwd->fwd = fwd;
			py_fwd->fwd_name = fwd->GetForwardName();
			py_fwd->callback = pFunc;
    
			if (callback != Py_None)
				AddToForwardCallbacks(py_fwd);
    
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
				"create(name, callback, et, ...) -> Forward object\n\n"
				"Creates a new forward. All the arguments after |et| are the types of the objects\n"
				"that will be passed to the forward's hooks. For example:\n\n"
				"   >>> myforward = create(\"\", None, ET_Ignore, int, int, str)\n"
				"<anonymous Forward: 0x819c12d>\n"
				"   >>> def myhook(num1, num2, name):\n"
				"   >>>     print num1, num2, name"
				"   >>>     return Plugin_Continue\n"
				"\n"
				"   >>> myforward.add_function(myhook)\n"
				"   >>> myforward.fire(3, 1337, \"elite\")\n"
				"3 1337 elite\n\n"
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
				"@type  ...: type\n"
				"@param ...: The types of the parameters that will be passed to all the hooks\n"
				"   when the forward is fired."
				"@rtype: sourcemod.forwards.Forward\n"
				"@return: A Forward object that may manipulate the forward."},
			{NULL, NULL, 0, NULL},
		};

		PyObject *
		initforwards(void)
		{
			Py_INCREF(_PyType_Type);
			forwards__ForwardType.ob_type = _PyType_Type;
			if (PyType_Ready((PyTypeObject*)&forwards__ForwardType) < 0)
				return NULL;
    
			PyObject *forwards = Py_InitModule3("forwards", forwards__methods,
				"Objects and functions that manipulate the forwards system");
    
			Py_INCREF((PyObject*)&forwards__ForwardType);
			PyModule_AddObject(forwards, "Forward", (PyObject*)&forwards__ForwardType);
    
			PyModule_AddIntMacro(forwards, ET_Ignore);
			PyModule_AddIntMacro(forwards, ET_Single);
			PyModule_AddIntMacro(forwards, ET_Event);
			PyModule_AddIntMacro(forwards, ET_Hook);
			PyModule_AddIntMacro(forwards, ET_LowEvent);
    
			return forwards;
		}
	}
}