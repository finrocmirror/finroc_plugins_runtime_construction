//
// You received this file as part of Finroc
// A framework for intelligent robot control
//
// Copyright (C) Finroc GbR (finroc.org)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
//----------------------------------------------------------------------
/*!\file    plugins/runtime_construction/tInterfaces.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2014-06-25
 *
 */
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tInterfaces.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "core/tRuntimeEnvironment.h"

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
typedef core::tFrameworkElement::tFlag tFlag;
typedef core::tFrameworkElement::tFlags tFlags;

//----------------------------------------------------------------------
// Const values
//----------------------------------------------------------------------
static const std::vector<tInterfaces::tStaticInterfaceInfo> cNO_STATIC_INTERFACE_INFO;

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------

tInterfaces::tInterfaces() :
  static_interface_info(cNO_STATIC_INTERFACE_INFO),
  interface_array(NULL),
  shared_interfaces(0)
{
  throw std::logic_error("This tEditableInterfaces constructor should never be called.");
}

tInterfaces::tInterfaces(const std::vector<tStaticInterfaceInfo>& static_interface_info, core::tPortGroup** interface_array,
                         std::bitset<cMAX_INTERFACE_COUNT> shared_interfaces) :
  static_interface_info(static_interface_info),
  interface_array(interface_array),
  shared_interfaces(shared_interfaces)
{
  assert(static_interface_info.size() <= cMAX_INTERFACE_COUNT && "Maximum number of interfaces exceeded");
}

core::tPortGroup& tInterfaces::CreateInterface(core::tFrameworkElement* parent, size_t index, bool initialize) const
{
  if (interface_array[index])
  {
    FINROC_LOG_PRINT(ERROR, "Interface already created.");
    return *(interface_array[index]);
  }
  if (index >= static_interface_info.size())
  {
    throw new std::runtime_error("tEditableInterfaces::CreateInterface - Invalid index.");
  }

  const tStaticInterfaceInfo& static_info = static_interface_info[index];
  interface_array[index] = new core::tPortGroup(parent, static_info.name, tFlag::INTERFACE | static_info.extra_interface_flags, GetDefaultPortFlags(index));
  if (initialize)
  {
    interface_array[index]->Init();
  }
  return *(interface_array[index]);
}

tFlags tInterfaces::GetDefaultPortFlags(size_t interface_index) const
{
  if (interface_index > static_interface_info.size())
  {
    throw new std::out_of_range("Index is out of bounds");
  }
  return static_interface_info[interface_index].default_port_flags | (shared_interfaces[interface_index] ? tFlags(tFlag::SHARED) : tFlags());
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
