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
typedef core::tFrameworkElement::tFlag tFlag;
typedef core::tFrameworkElement::tFlags tFlags;

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
  throw std::logic_error("This tEditableInterfaces constructor should never be called.");
}

tEditableInterfaces::tEditableInterfaces(const std::vector<tStaticInterfaceInfo>& static_interface_info, core::tPortGroup** interface_array,
    std::bitset<cMAX_INTERFACE_COUNT> shared_interfaces) :
  static_interface_info(static_interface_info),
  interface_array(interface_array),
  shared_interfaces(shared_interfaces)
{
  assert(static_interface_info.size() <= cMAX_INTERFACE_COUNT && "Maximum number of interfaces exceeded");
}

core::tPortGroup& tEditableInterfaces::CreateInterface(core::tFrameworkElement* parent, size_t index, bool initialize) const
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

tFlags tEditableInterfaces::GetDefaultPortFlags(size_t interface_index) const
{
  if (interface_index > static_interface_info.size())
  {
    throw new std::out_of_range("Index is out of bounds");
  }
  return static_interface_info[interface_index].default_port_flags | (shared_interfaces[interface_index] ? tFlags(tFlag::SHARED) : tFlags());
}

void tEditableInterfaces::LoadInterfacePorts(const rrlib::xml::tNode& node)
{
  std::string name = node.GetStringAttribute("name");
  for (size_t i = 0; i < static_interface_info.size(); i++)
  {
    if (name.compare(static_interface_info[i].name) == 0)
    {
      core::tPortGroup* port_group = interface_array[i];

      // possibly create interface
      if (!port_group)
      {
        CreateInterface(GetAnnotated<core::tFrameworkElement>(), i, GetAnnotated<core::tFrameworkElement>()->IsReady());
      }

      tPortCreationList port_creation_list(*interface_array[i], GetDefaultPortFlags(i), static_interface_info[i].selectable_create_options);
      node >> port_creation_list;
      return;
    }
  }
  throw new std::runtime_error("There is no editable interface called '" + name + "'");
}

void tEditableInterfaces::SaveAllNonEmptyInterfaces(rrlib::xml::tNode& parent_node)
{
  for (size_t i = 0; i < static_interface_info.size(); i++)
  {
    core::tPortGroup* port_group = interface_array[i];
    if (port_group)
    {
      for (auto it = port_group->ChildPortsBegin(); it != port_group->ChildPortsEnd(); ++it)
      {
        if (it->GetFlag(tFlag::FINSTRUCTED))
        {
          rrlib::xml::tNode& interface_node = parent_node.AddChildNode("interface");
          SaveInterfacePorts(interface_node, i);
          break;
        }
      }
    }
  }
}

void tEditableInterfaces::SaveInterfacePorts(rrlib::xml::tNode& node, size_t index)
{
  const tStaticInterfaceInfo& static_info = static_interface_info[index];
  core::tPortGroup* port_group = interface_array[index];

  node.SetAttribute("name", static_info.name);
  if (port_group)
  {
    tPortCreationList port_creation_list(*port_group, GetDefaultPortFlags(index), static_info.selectable_create_options);
    node << port_creation_list;
  }
}

rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tEditableInterfaces& interfaces)
{
  rrlib::thread::tLock lock(core::tRuntimeEnvironment::GetInstance().GetStructureMutex()); // no one should interfere
  core::tPortGroup** current_interface_array = interfaces.interface_array;

  stream.WriteByte(static_cast<uint8_t>(interfaces.static_interface_info.size())); // number of interfaces
  for (size_t i = 0; i < interfaces.static_interface_info.size(); i++)
  {
    stream.WriteString(interfaces.static_interface_info[i].name);
    stream.WriteBoolean(*current_interface_array);
    if (*current_interface_array)
    {
      tPortCreationList port_creation_list(**current_interface_array, interfaces.GetDefaultPortFlags(i),
                                           interfaces.static_interface_info[i].selectable_create_options);
      stream << port_creation_list;
    }
    else
    {
      tPortCreateOptions selectable = interfaces.static_interface_info[i].selectable_create_options;
      if (interfaces.shared_interfaces[i])
      {
        selectable.Set(tPortCreateOption::SHARED, false);
      }
      stream << selectable.Raw();
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
    std::string name = stream.ReadString();
    if (name.compare(interfaces.static_interface_info[i].name) != 0)
    {
      FINROC_LOG_PRINT(WARNING, "Deserialized string ", name, " does not match expected ", interfaces.static_interface_info[i].name);
    }

    bool interface_has_ports = stream.ReadBoolean();
    if (!interface_has_ports)
    {
      stream.ReadByte(); // selectable_create_options
      if ((*current_interface_array))
      {
        for (auto it = (*current_interface_array)->ChildPortsBegin(); it != (*current_interface_array)->ChildPortsEnd(); ++it)
        {
          if (it->GetFlag(tFlag::FINSTRUCTED))
          {
            it->ManagedDelete();
          }
        }
      }
    }
    else
    {
      // possibly create interface
      if (!(*current_interface_array))
      {
        interfaces.CreateInterface(interfaces.GetAnnotated<core::tFrameworkElement>(), i, interfaces.GetAnnotated<core::tFrameworkElement>()->IsReady());
      }

      tPortCreationList port_creation_list(**current_interface_array, interfaces.GetDefaultPortFlags(i),
                                           interfaces.static_interface_info[i].selectable_create_options);
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
