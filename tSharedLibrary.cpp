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
/*!\file    plugins/runtime_construction/tSharedLibrary.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2013-10-20
 *
 */
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tSharedLibrary.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Debugging
//----------------------------------------------------------------------
#include <cassert>

//----------------------------------------------------------------------
// Namespace usage
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Namespace declaration
//----------------------------------------------------------------------
namespace finroc
{
namespace runtime_construction
{

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Const values
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------

tSharedLibrary::tSharedLibrary() :
  name(),
  path()
{}

tSharedLibrary::tSharedLibrary(const std::string& library_name) :
  name(library_name),
  path()
{
  if (name.rfind('/') != std::string::npos)
  {
    path = name.substr(0, name.rfind('/'));
    name = name.substr(name.rfind('/') + 1);
  }

  if (name.length() > 6 && name.substr(0, 3) == "lib" && name.substr(name.length() - 3) == ".so")
  {
    name = name.substr(3, name.length() - 6);
  }
}

std::string tSharedLibrary::ToString(bool platform_dependent) const
{
  if (platform_dependent)
  {
    return "lib" + name + ".so";
  }
  return name;
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
