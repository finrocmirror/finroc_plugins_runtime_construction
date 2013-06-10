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
/*!\file    plugins/runtime_construction/tEditableInterfaces.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2013-06-04
 *
 */
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tEditableInterfaces.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "core/tRuntimeEnvironment.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tPortCreationList.h"

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
static rrlib::rtti::tDataType<tEditableInterfaces> cTYPE;
static const std::vector<tEditableInterfaces::tStaticInterfaceInfo> cNO_STATIC_INTERFACE_INFO;

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------

tEditableInterfaces::tEditableInterfaces() :
  static_interface_info(cNO_STATIC_INTERFACE_INFO),
  interface_array(NULL),
  shared_interfaces(0)
{
  throw std::runtime_error("This tEditableInterfaces constructor should never be called.");
}

tEditableInterfaces::tEditableInterfaces(const std::vector<tStaticInterfaceInfo>& static_interface_info, core::tPortGroup** interface_array,
    std::bitset<cMAX_INTERFACE_COUNT> shared_interfaces) :
  static_interface_info(static_interface_info),
  interface_array(interface_array),
  shared_interfaces(shared_interfaces)
{
  assert(static_interface_info.size() <= cMAX_INTERFACE_COUNT && "Maximum number of interfaces exceeded");
}

rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tEditableInterfaces& interfaces)
{
  rrlib::thread::tLock lock(core::tRuntimeEnvironment::GetInstance().GetStructureMutex()); // no one should interfere
  core::tPortGroup** current_interface_array = interfaces.interface_array;

  stream.WriteByte(static_cast<uint8_t>(interfaces.static_interface_info.size())); // number of interfaces
  for (size_t i = 0; i < interfaces.static_interface_info.size(); i++)
  {
    stream.WriteBoolean(*current_interface_array);
    if (*current_interface_array)
    {
      stream.WriteString(interfaces.static_interface_info[i].name);
      tPortCreationList port_creation_list(**current_interface_array, interfaces.static_interface_info[i].default_port_flags,
                                           interfaces.static_interface_info[i].show_output_port_selection);
      stream << port_creation_list;
    }
    current_interface_array++;
  }
  return stream;
}

rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tEditableInterfaces& interfaces)
{
  rrlib::thread::tLock lock(core::tRuntimeEnvironment::GetInstance().GetStructureMutex()); // no one should interfere
  core::tPortGroup** current_interface_array = interfaces.interface_array;

  size_t size = stream.ReadByte();
  if (size != interfaces.static_interface_info.size())
  {
    throw std::runtime_error("Error deserializing tEditableInterfaces: Wrong number of interfaces");
  }

  for (size_t i = 0; i < interfaces.static_interface_info.size(); i++)
  {
    bool interface_has_ports = stream.ReadBoolean();
    if (!interface_has_ports)
    {
      for (auto it = (*current_interface_array)->ChildPortsBegin(); it != (*current_interface_array)->ChildPortsEnd(); ++it)
      {
        if (it->GetFlag(core::tFrameworkElement::tFlag::FINSTRUCTED))
        {
          it->ManagedDelete();
        }
      }
    }
    else
    {
      std::string name = stream.ReadString();
      if (name.compare(interfaces.static_interface_info[i].name) != 0)
      {
        FINROC_LOG_PRINT(WARNING, "Deserialized string ", name, " does not match expected ", interfaces.static_interface_info[i].name);
      }
      tPortCreationList port_creation_list(**current_interface_array, interfaces.static_interface_info[i].default_port_flags,
                                           interfaces.static_interface_info[i].show_output_port_selection);
      stream >> port_creation_list;
    }
    current_interface_array++;
  }
  return stream;
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
