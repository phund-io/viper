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

PyObject *
initusermessages(void)
{
    PyObject *usermessages = Py_InitModule3("usermessages", NULL,
        "Module for manipulating usermessages.");
    
    return usermessages;
}

