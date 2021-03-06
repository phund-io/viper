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

#ifndef _INCLUDE_VIPER_IVIPER_H_
#define _INCLUDE_VIPER_IVIPER_H_

#include <Python.h>

class CCommand;

namespace Viper {
	class IBaseViper {
	public:
		virtual void PushCommandStack(const CCommand *cmd) =0;
		virtual const CCommand *PeekCommandStack() =0;
		virtual void PopCommandStack() =0;
		virtual char const *CurrentCommandName() =0;
	};
}


#endif /* _INCLUDE_VIPER_IVIPER_H_ */
