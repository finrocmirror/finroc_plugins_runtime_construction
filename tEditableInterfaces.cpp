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
#include "core/tFrameworkElementTags.h"

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
static const char* cEDITABLE_INTERFACE_TAG = "edit";

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------

void tEditableInterfaces::AddInterface(core::tPortGroup& interface, tPortCreateOptions port_create_options, bool at_front)
{
  // Mark interface as editable
  if (interface.IsReady())
  {
    FINROC_LOG_PRINT(WARNING, "Interface was already initialized before tagging it as editable");
  }
  core::tFrameworkElementTags::AddTag(interface, cEDITABLE_INTERFACE_TAG);

  // Find parent component
  core::tFrameworkElement* parent = &interface;
  do
  {
    parent = parent->GetParent();
    if (!parent)
    {
      throw std::runtime_error("Interface has no parent component");
    }
  }
  while (!(core::tFrameworkElementTags::IsTagged(*parent, "module") || core::tFrameworkElementTags::IsTagged(*parent, "group")));

  // Get or create annotation
  tEditableInterfaces* annotation = parent->GetAnnotation<tEditableInterfaces>();
  if (!annotation)
  {
    annotation = &parent->EmplaceAnnotation<tEditableInterfaces>();
  }

  // Add interface to list
  if (at_front)
  {
    annotation->editable_interfaces.insert(annotation->editable_interfaces.begin(), tEditableInterfaceEntry(&interface, port_create_options));
  }
  else
  {
    annotation->editable_interfaces.emplace_back(&interface, port_create_options);
  }
}

core::tPortGroup& tEditableInterfaces::LoadInterfacePorts(const rrlib::xml::tNode& node)
{
  std::string name = node.GetStringAttribute("name");
  for (auto & interface : editable_interfaces)
  {
    if (name == interface.first->GetName())
    {
      tPortCreationList port_creation_list(*interface.first, interface.first->GetDefaultPortFlags(), interface.second);
      node >> port_creation_list;
      return *interface.first;
    }
  }
  throw new std::runtime_error("There is no editable interface called '" + name + "'");
}

void tEditableInterfaces::SaveAllNonEmptyInterfaces(rrlib::xml::tNode& parent_node)
{
  for (auto & interface : editable_interfaces)
  {
    for (auto it = interface.first->ChildPortsBegin(); it != interface.first->ChildPortsEnd(); ++it)
    {
      if (it->GetFlag(tFlag::FINSTRUCTED))
      {
        rrlib::xml::tNode& interface_node = parent_node.AddChildNode("interface");
        SaveInterfacePorts(interface_node, interface);
        break;
      }
    }
  }
}

void tEditableInterfaces::SaveInterfacePorts(rrlib::xml::tNode& node, tEditableInterfaceEntry& entry)
{
  node.SetAttribute("name", entry.first->GetName());
  tPortCreationList port_creation_list(*entry.first, entry.first->GetDefaultPortFlags(), entry.second);
  node << port_creation_list;
}

rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tEditableInterfaces& interfaces)
{
  rrlib::thread::tLock lock(core::tRuntimeEnvironment::GetInstance().GetStructureMutex()); // no one should interfere
  stream.WriteByte(static_cast<uint8_t>(interfaces.editable_interfaces.size())); // number of interfaces
  for (auto & interface : interfaces.editable_interfaces)
  {
    stream.WriteString(interface.first->GetName());
    bool contains_ports = interface.first->ChildCount();
    stream.WriteBoolean(contains_ports);
    if (contains_ports)
    {
      tPortCreationList port_creation_list(*interface.first, interface.first->GetDefaultPortFlags(), interface.second);
      stream << port_creation_list;
    }
    else
    {
      tPortCreateOptions selectable = interface.second;
      stream << selectable.Raw();
    }
  }
  return stream;
}

rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tEditableInterfaces& interfaces)
{
  rrlib::thread::tLock lock(core::tRuntimeEnvironment::GetInstance().GetStructureMutex()); // no one should interfere

  size_t size = stream.ReadByte();
  if (size != interfaces.editable_interfaces.size())
  {
    throw std::runtime_error("Error deserializing tEditableInterfaces: Wrong number of interfaces");
  }

  for (auto & interface : interfaces.editable_interfaces)
  {
    std::string name = stream.ReadString();
    if (name != interface.first->GetName())
    {
      FINROC_LOG_PRINT_STATIC(WARNING, "Deserialized string ", name, " does not match expected ", interface.first->GetName());
    }

    bool interface_has_ports = stream.ReadBoolean();
    if (!interface_has_ports)
    {
      stream.ReadByte(); // selectable_create_options
      for (auto it = interface.first->ChildPortsBegin(); it != interface.first->ChildPortsEnd(); ++it)
      {
        if (it->GetFlag(tFlag::FINSTRUCTED))
        {
          it->ManagedDelete();
        }
      }
    }
    else
    {
      tPortCreationList port_creation_list(*interface.first, interface.first->GetDefaultPortFlags(), interface.second);
      stream >> port_creation_list;
    }
  }

  if (interfaces.GetListener())
  {
    interfaces.GetListener()->OnEditableInterfacesChange();
  }

  return stream;
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
