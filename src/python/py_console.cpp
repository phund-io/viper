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

#include <Python.h>
#include <structmember.h>
#include "viper_globals.h"
#include "systems/ConCmdManager.h"
#include "systems/ConVarManager.h"
#include "systems/PluginSys.h"
#include "python/py_console.h"
#include "IViperForwardSys.h"

static PyObject *
console__ConCommandReply__reply(console__ConCommandReply *self, PyObject *args)
{
    // :TODO: Reply to the user in the way they called the concmd
    char const *message = NULL;
    if (!PyArg_ParseTuple(args, "s", &message))
        return NULL;

	g_SMAPI->ConPrintf("%s\n", message);
    
    Py_RETURN_NONE;
}

static PyMethodDef console__ConCommandReply__methods[] = {
    {"reply", (PyCFunction)console__ConCommandReply__reply, METH_VARARGS,
        "reply(message)\n\n"
        "Replies to the client whom executed the ConCommand in the way they\n"
        "executed the ConCommand (console or chat).\n"
        "@type  message: string\n"
        "@param message: The message to send"},
    {NULL, NULL, 0, NULL},
};

static PyMemberDef console__ConCommandReply__members[] = {
    {"args", T_OBJECT_EX, offsetof(console__ConCommandReply, args), READONLY,
        "The arguments passed when the ConCommand was executed"},
    {"name", T_STRING, offsetof(console__ConCommandReply, name), READONLY,
        "The name of the ConCommand executed."},
    {"client", T_INT, offsetof(console__ConCommandReply, client), READONLY,
        "The client whom executed the ConCommand"},
    {"argstring", T_STRING, offsetof(console__ConCommandReply, argstring), READONLY,
        "The full argument string sent to the server. This includes any quotes sent."},
    {NULL},
};

PyTypeObject console__ConCommandReplyType = {
    PyObject_HEAD_INIT(NULL)
    0,                          /*ob_size*/
    "sourcemod.console.ConCommandReply",/*tp_name*/
    sizeof(console__ConCommandReply),/*tp_basicsize*/
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
    "Stores information about a ConCommand when it is executed.",
    0,		                    /* tp_traverse */
    0,		                    /* tp_clear */
    0,		                    /* tp_richcompare */
    0,		                    /* tp_weaklistoffset */
    0,		                    /* tp_iter */
    0,		                    /* tp_iternext */
    console__ConCommandReply__methods,/* tp_methods */
    console__ConCommandReply__members,/* tp_members */
    0,                          /* tp_getset */
    0,                          /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    0,                          /* tp_dictoffset */
};

static PyObject *
console__ConVar__hook_change(console__ConVar *self, PyObject *args)
{
    PyObject *callback;
    if (!PyArg_ParseTuple(args, "O", &callback))
        return NULL;
    
    if (!PyCallable_Check(callback))
    {
        PyErr_SetString(g_pViperException, "The callback passed was not callable");
        return NULL;
    }
    
    GET_THREAD_PLUGIN();
    
    IViperPluginFunction *pFunc = CPluginFunction::CreatePluginFunction(callback,
        pPlugin);
    
    g_ConVarManager.HookConVarChange(self->pVar, pFunc);
    
    Py_RETURN_NONE;
}

static PyObject *
console__ConVar__unhook_change(console__ConVar *self, PyObject *args)
{
    PyObject *callback;
    if (!PyArg_ParseTuple(args, "O", &callback))
        return NULL;
    
    if (!PyCallable_Check(callback))
    {
        PyErr_SetString(g_pViperException, "The callback passed was not callable");
        return NULL;
    }
    
    GET_THREAD_PLUGIN();
    
    IViperPluginFunction *pFunc = CPluginFunction::CreatePluginFunction(callback,
        pPlugin);
    
    g_ConVarManager.UnhookConVarChange(self->pVar, pFunc);
    
    if (PyErr_Occurred())
        return NULL;
    
    Py_RETURN_NONE;
}

static PyMethodDef console__ConVar__methods[] = {
    {"hook_change", (PyCFunction)console__ConVar__hook_change, METH_VARARGS,
        "hook_change(callback)\n\n"
        "Creates a hook for when a console variable's value is changed.\n"
        "@type  callback: callable\n"
        "@param callback: The function to call when the ConVar is changed.\n"
        "   The callback should have the prototype callback(cvar, oldvalue,\n"
        "   newvalue), where cvar is the ConVar object representing the ConVar\n"
        "   that was changed, newvalue is the value being assigned to the Convar\n"
        "   as a string, and oldvalue is the previous value of the ConVar as a string.\n"},
    {"unhook_change", (PyCFunction)console__ConVar__unhook_change, METH_VARARGS,
        "unhook_change(callback)\n\n"
        "Removes a ConVar change hook.\n"
        "@type  callback: callable\n"
        "@param callback: The hook to remove.\n"
        "@throw ViperException: No active hook on the ConVar, or an invalid\n"
        "   or unregistered callback supplied."},
    {NULL, NULL, 0, NULL},
};

static PyMemberDef console__ConVar__members[] = {
    {"name", T_STRING, offsetof(console__ConVar, name), READONLY,
        "The name of the ConVar."},
    {"value", T_STRING, offsetof(console__ConVar, value), 0, /* RW access */
        "The value of the ConVar."},
    {NULL},
};

PyTypeObject console__ConVarType = {
    PyObject_HEAD_INIT(NULL)
    0,                          /*ob_size*/
    "sourcemod.console.ConVar", /*tp_name*/
    sizeof(console__ConVar),    /*tp_basicsize*/
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
    "Represents a ConVar.",     /* tp_doc */
    0,		                    /* tp_traverse */
    0,		                    /* tp_clear */
    0,		                    /* tp_richcompare */
    0,		                    /* tp_weaklistoffset */
    0,		                    /* tp_iter */
    0,		                    /* tp_iternext */
    console__ConVar__methods,   /* tp_methods */
    console__ConVar__members,   /* tp_members */
    0,                          /* tp_getset */
    0,                          /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    0,                          /* tp_dictoffset */
};

static PyObject *
console__create_convar(PyObject *self, PyObject *args, PyObject *keywds)
{
    char const *sName = NULL;
    char const *sValue = NULL;
    char const *sDescription = "";
    int flags = 0;
    PyObject *pymin = Py_None;
    PyObject *pymax = Py_None;
    
    static char *kwdlist[] = {"name", "value", "description", "flags", "min",
                              "max", NULL};
    
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "ss|siOO", kwdlist,
            &sName, &sValue, &sDescription, &flags, &pymin, &pymax))
        return NULL;
    
    if (!IS_STR_FILLED(sName))
    {
        PyErr_SetString(g_pViperException, "The name passed was blank");
        return NULL;
    }
    
    if (!IS_STR_FILLED(sValue))
    {
        PyErr_SetString(g_pViperException, "The value passed was blank");
        return NULL;
    }
    
    float max = 0.0f;
    bool hasMax = false;
    float min = 0.0f;
    bool hasMin = false;
    
    if (pymin != Py_None)
    {
        if (!PyFloat_Check(pymin))
        {
            PyErr_SetString(g_pViperException, "The min value passed was not "
                "a float.");
            return NULL;
        }
        
        hasMin = true;
        min = PyFloat_AsDouble(pymin);
    }
    
    if (pymax != Py_None)
    {
        if (!PyFloat_Check(pymax))
        {
            PyErr_SetString(g_pViperException, "The min value passed was not "
                "a float.");
            return NULL;
        }
        
        hasMax = true;
        max = PyFloat_AsDouble(pymax);
    }
    
    GET_THREAD_PLUGIN();
    
    console__ConVar *handle = g_ConVarManager.CreateConVar(pPlugin, sName,
        sValue, sDescription, flags, hasMin, min, hasMax, max);
    
    if (handle == NULL)
    {
        PyErr_SetString(g_pViperException, "ConVar name conflicts with an "
            "existing ConCommand.");
        return NULL;
    }
    
    return (PyObject*)handle;
}

static PyObject *
console__print_to_server(PyObject *self, PyObject *args)
{
	char *message;

	if(!PyArg_ParseTuple(args, "s", &message))
		return NULL;

	g_SMAPI->ConPrintf("%s\n", message);
	
	Py_RETURN_NONE;
}

static PyObject *
console__reg_concmd(PyObject *self, PyObject *args, PyObject *keywds)
{
    char const *sName = NULL;
    char const *sDescription = "";
    int flags = 0;
    PyFunction *pFunc;
    
    static char *kwdlist[] = {"name", "callback", "description", "flags", NULL};
    
    if (!PyArg_ParseTupleAndKeywords(args, keywds, "sO|si", kwdlist,
            &sName, &pFunc, &sDescription, &flags))
        return NULL;
    
    if (!PyCallable_Check(pFunc))
    {
        PyErr_SetString(g_pViperException, "The callback passed was not callable");
        return NULL;
    }
    
    if (!IS_STR_FILLED(sName))
    {
        PyErr_SetString(g_pViperException, "The name passed was blank");
        return NULL;
    }
    
    Py_INCREF(pFunc);
    
    GET_THREAD_PLUGIN();
    
    if (!g_VCmds.AddCommand(pPlugin, pFunc, Cmd_Console, sName, sDescription, flags))
        Py_RETURN_FALSE;
    
    Py_RETURN_TRUE;
}

static PyObject *
console__server_command(PyObject *self, PyObject *args)
{
	char *command;

	if(!PyArg_ParseTuple(args, "s", &command))
		return NULL;

	engine->ServerCommand(command);
	
	Py_RETURN_NONE;
}

static PyMethodDef console__methods[] = {
    {"create_convar", (PyCFunction)console__create_convar, METH_VARARGS|METH_KEYWORDS,
        "create_convar(name, value[, description[, flags[, min[, max]]]]) -> ConVar object\n\n"
        "Creates a new console variable.\n"
        "@type  name: string\n"
        "@param name: Name of the ConVar\n"
        "@type  value: string\n"
        "@param value: The value of the ConVar. As the internal representation\n"
        "   of a ConVar's value is a string, stringization is left to the user.\n"
        "@type  description: string\n"
        "@param description: (Optional) Description of the ConVar\n"
        "@type  flags: FCVAR constants\n"
        "@param flags: (Optional) Flags that change how a ConVar is handled.\n"
        "   Use FCVAR constants, such as FCVAR_CHEAT, etc.\n"
        "@type  min: float\n"
        "@param min: The lowest number this ConVar can be set to. Pass None to\n"
        "   ignore this field."
        "@type  max: float\n"
        "@param max: The highest number this ConVar can be set to. Pass None to\n"
        "   ignore this field."},
	{"print_to_server", console__print_to_server, METH_VARARGS,
	    "print_to_server(message)\n\n"
	    "Sends a message to the server console.\n"
	    "@type  message: string\n"
	    "@param message: The message to print"},
    {"reg_concmd", (PyCFunction)console__reg_concmd, METH_VARARGS|METH_KEYWORDS,
        "reg_concmd(name, callback[, description[, flags]]) -> bool\n\n"
        "Registers a new console command or hooks an existing one.\n"
        "@type  name: string\n"
        "@param name: Name of the ConCommand\n"
        "@type  callback: callable\n"
        "@param callback: A function to call when the ConCommand is executed.\n"
        "   The function should receive one argument: a sourcemod.console.ConCommand object.\n"
        "@type  description: string\n"
        "@param description: (Optional) Description of the ConCommand\n"
        "@type  flags: FCVAR constants\n"
        "@param flags: (Optional) Flags that change how a ConCommand is handled.\n"
        "   Use FCVAR constants, such as FCVAR_CHEAT, etc."},
	{"server_command", console__server_command, METH_VARARGS,
	    "server_command(command)\n\n"
	    "Executes a command as if it were on the server console\n"
	    "@type  command: string\n"
	    "@param command: Command to execute"},
    {NULL, NULL, 0, NULL},
};

PyObject *
initconsole(void)
{
    if (PyType_Ready((PyTypeObject*)&console__ConCommandReplyType) < 0)
        return NULL;
    if (PyType_Ready((PyTypeObject*)&console__ConVarType) < 0)
        return NULL;
    
    PyObject *console = Py_InitModule3("console", console__methods,
        "Contains functions and objects pertaining to console interaction.");
    
    PyModule_AddObject(console, "ConCommandReply", (PyObject*)&console__ConCommandReplyType);
    PyModule_AddObject(console, "ConVar", (PyObject*)&console__ConCommandReplyType);
    
    PyModule_AddIntConstant(console, "Plugin_Continue", Pl_Continue);
    PyModule_AddIntConstant(console, "Plugin_Stop", Pl_Stop);
    PyModule_AddIntConstant(console, "Plugin_Handled", Pl_Handled);
    
    PyModule_AddIntMacro(console, FCVAR_NONE);
    PyModule_AddIntMacro(console, FCVAR_UNREGISTERED);
    PyModule_AddIntMacro(console, FCVAR_GAMEDLL);
    PyModule_AddIntMacro(console, FCVAR_CLIENTDLL);
    PyModule_AddIntMacro(console, FCVAR_PROTECTED);
    PyModule_AddIntMacro(console, FCVAR_SPONLY);
    PyModule_AddIntMacro(console, FCVAR_ARCHIVE);
    PyModule_AddIntMacro(console, FCVAR_NOTIFY);
    PyModule_AddIntMacro(console, FCVAR_USERINFO);
    PyModule_AddIntMacro(console, FCVAR_PRINTABLEONLY);
    PyModule_AddIntMacro(console, FCVAR_UNLOGGED);
    PyModule_AddIntMacro(console, FCVAR_NEVER_AS_STRING);
    PyModule_AddIntMacro(console, FCVAR_REPLICATED);
    PyModule_AddIntMacro(console, FCVAR_CHEAT);
    PyModule_AddIntMacro(console, FCVAR_DEMO);
    PyModule_AddIntMacro(console, FCVAR_DONTRECORD);
    PyModule_AddIntMacro(console, FCVAR_NOT_CONNECTED);
    PyModule_AddIntMacro(console, FCVAR_ARCHIVE_XBOX);
    
    /* Manually define the next ones, as they are not defined in OB */
    PyModule_AddIntConstant(console, "FCVAR_LAUNCHER",          (1<<1));
    PyModule_AddIntConstant(console, "FCVAR_MATERIAL_SYSTEM",   (1<<4));
    PyModule_AddIntConstant(console, "FCVAR_STUDIORENDER",      (1<<15));
    PyModule_AddIntConstant(console, "FCVAR_PLUGIN",            (1<<18));
    PyModule_AddIntConstant(console, "FCVAR_DATACACHE",         (1<<19));
    PyModule_AddIntConstant(console, "FCVAR_TOOLSYSTEM",        (1<<20));
    PyModule_AddIntConstant(console, "FCVAR_FILESYSTEM",        (1<<21));
    PyModule_AddIntConstant(console, "FCVAR_SOUNDSYSTEM",       (1<<23));
    PyModule_AddIntConstant(console, "FCVAR_INPUTSYSTEM",       (1<<25));
    PyModule_AddIntConstant(console, "FCVAR_NETWORKSYSTEM",     (1<<26));
    PyModule_AddIntConstant(console, "FCVAR_VPHYSICS",          (1<<27));
    
    return console;
}