//
// You received this file as part of Finroc
// A Framework for intelligent robot control
//
// Copyright (C) Finroc GbR (finroc.org)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
//----------------------------------------------------------------------
/*!\file    plugins/runtime_construction/internal/dynamic_loading.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-02
 *
 * Contains utility/convenience methods for dynamic loading -
 * finroc libraries in particular.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__runtime_construction__internal__dynamic_loading_h__
#define __plugins__runtime_construction__internal__dynamic_loading_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <set>

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tCreateFrameworkElementAction.h"

//----------------------------------------------------------------------
// Namespace declaration
//----------------------------------------------------------------------
namespace finroc
{
namespace runtime_construction
{
namespace internal
{

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Function declarations
//----------------------------------------------------------------------

/*!
 * dlopen specified library
 * (also takes care of closing library again on program shutdown)
 *
 * \param open Shared library to open (.so file)
 * \return Returns true if successful
 */
bool DLOpen(const char* open);

/*!
 * \return Returns vector with all finroc libraries available on hard disk.
 */
std::set<std::string> GetAvailableFinrocLibraries();

/*!
 * \return Returns .so file in which address provided as argument is found by dladdr.
 */
std::string GetBinary(void* addr);

/*!
 * \return Returns vector with all libfinroc*.so and librrlib*.so files loaded by current process.
 */
std::set<std::string> GetLoadedFinrocLibraries();

/*!
 * \return Returns vector with all available finroc libraries that haven't been loaded yet.
 */
std::vector<std::string> GetLoadableFinrocLibraries();

/*!
 * Returns/loads CreateFrameworkElementAction with specified name and specified .so file.
 * (doesn't do any dynamic loading, if .so is already present)
 *
 * \param group Group (.jar or .so)
 * \param name Module type name
 * \return CreateFrameworkElementAction - null if it could not be found
 */
tCreateFrameworkElementAction* LoadModuleType(const std::string& group, const std::string& name);

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
