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

#ifndef _VIPER_INCLUDE_CONCMDMANAGER_H_
#define _VIPER_INCLUDE_CONCMDMANAGER_H_

#include <sm_trie_tpl.h>
#include <sh_list.h>
#include "viper_globals.h"
#include "IViperPluginSys.h"
#include <convar.h>
#include <compat_wrappers.h>

enum CmdType
{
    Cmd_Server = 0,
    Cmd_Console,
    Cmd_Admin,
};

struct AdminCmdInfo
{
    AdminCmdInfo()
    {
        cmdGrpId = -1;
        flags = 0;
        eflags = 0;
    };

    int cmdGrpId;           /* index into cmdgroup string table */
    SourceMod::FlagBits flags;         /* default flags */
    SourceMod::FlagBits eflags;        /* effective flags */
};

struct CmdHook
{
    CmdHook() : pf(NULL), pAdmin(NULL){};
    
};

struct ConCmdInfo
{
    /// Server callbacks
    /// Console callbacks
    /// Was the ConCmd created by Viper?

typedef SourceHook::List<PlCmdInfo> CmdList;
typedef SourceHook::List<ConCommand*> ConCmdList;

class CConCmdManager :
    public SourceMod::IRootConsoleCommand,
    public ViperGlobalClass,
    public IViperPluginsListener
public: // ViperGlobalClass
    virtual void OnViperAllInitialized();
    virtual void OnViperShutdown();
public: // IViperPluginsListener
    virtual void OnPluginUnloaded(IViperPlugin *plugin);
        char const *name, char const *description, int flags);
        

private:
    /** Adds a new command or finds one that already exists */

#endif /* _VIPER_INCLUDE_CONCMDMANAGER_H_ */
