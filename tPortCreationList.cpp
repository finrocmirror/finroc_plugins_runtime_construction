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
/*!\file    plugins/runtime_construction/tPortCreationList.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-02
 *
 */
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tPortCreationList.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "core/port/tPortFactory.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tFinstructable.h"

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

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------

static const core::tFrameworkElement::tFlags cRELEVANT_FLAGS = tFlag::SHARED | tFlag::VOLATILE;
static rrlib::rtti::tDataType<tPortCreationList> cTYPE;

inline tPortCreateOptions ToPortCreateOptions(tFlags flags, tPortCreateOptions selectable_create_options)
{
  tPortCreateOptions result;
  if (selectable_create_options.Get(tPortCreateOption::SHARED) && flags.Get(tFlag::SHARED))
  {
    result |= tPortCreateOption::SHARED;
  }
  if (selectable_create_options.Get(tPortCreateOption::OUTPUT) && flags.Get(tFlag::OUTPUT_PORT))
  {
    result |= tPortCreateOption::OUTPUT;
  }
  return result;
}

inline tFlags ToFlags(tPortCreateOptions create_options, tPortCreateOptions selectable_create_options)
{
  tFlags result;
  if (selectable_create_options.Get(tPortCreateOption::SHARED) && create_options.Get(tPortCreateOption::SHARED))
  {
    result |= tFlag::SHARED;
  }
  if (selectable_create_options.Get(tPortCreateOption::OUTPUT) && create_options.Get(tPortCreateOption::OUTPUT))
  {
    result |= tFlag::OUTPUT_PORT;
  }
  return result;
}


tPortCreationList::tPortCreationList() :
  selectable_create_options(),
  list(),
  io_vector(NULL),
  flags(tFlags()),
  ports_flagged_finstructed(true)
{}

tPortCreationList::tPortCreationList(core::tFrameworkElement& port_group, tFlags flags, const tPortCreateOptions& selectable_create_options, bool ports_flagged_finstructed) :
  selectable_create_options(),
  list(),
  io_vector(&port_group),
  flags(flags | (ports_flagged_finstructed ? tFlag::FINSTRUCTED : tFlag::PORT)),
  ports_flagged_finstructed(ports_flagged_finstructed)
{
  if (!flags.Get(tFlag::SHARED) && selectable_create_options.Get(tPortCreateOption::SHARED))
  {
    this->selectable_create_options |= tPortCreateOption::SHARED;
  }
  if (!flags.Get(tFlag::OUTPUT_PORT) && selectable_create_options.Get(tPortCreateOption::OUTPUT))
  {
    this->selectable_create_options |= tPortCreateOption::OUTPUT;
  }
}

void tPortCreationList::Add(const std::string& name, rrlib::rtti::tType dt, const tPortCreateOptions& create_options)
{
  rrlib::thread::tLock lock(io_vector->GetStructureMutex());
  CheckPort(NULL, *io_vector, flags, name, dt, create_options, NULL);
}

void tPortCreationList::ApplyChanges(core::tFrameworkElement& io_vector_, tFlags flags_)
{
  rrlib::thread::tLock lock(io_vector->GetStructureMutex());
  std::vector<core::tAbstractPort*> ports1;
  GetPorts(*this->io_vector, ports1, ports_flagged_finstructed);
  std::vector<core::tAbstractPort*> ports2;
  GetPorts(io_vector_, ports2, ports_flagged_finstructed);

  for (size_t i = 0u; i < ports1.size(); i++)
  {
    core::tAbstractPort* ap1 = ports1[i];
    core::tAbstractPort* ap2 = i < ports2.size() ? ports2[i] : NULL;
    CheckPort(ap2, io_vector_, flags_, ap1->GetName(), ap1->GetDataType(), ToPortCreateOptions(ap1->GetAllFlags(), selectable_create_options), ap1);
  }
  for (size_t i = ports1.size(); i < ports2.size(); i++)
  {
    ports2[i]->ManagedDelete();
  }
}

void tPortCreationList::CheckPort(core::tAbstractPort* existing_port, core::tFrameworkElement& io_vector, tFlags flags,
                                  const std::string& name, rrlib::rtti::tType type, const tPortCreateOptions& create_options, core::tAbstractPort* prototype)
{
  if (existing_port && existing_port->GetName() == name && existing_port->GetDataType() == type &&
      existing_port->GetFlag(tFlag::VOLATILE) == flags.Get(tFlag::VOLATILE))
  {
    bool create_output_port = create_options.Get(tPortCreateOption::OUTPUT) || flags.Get(tFlag::OUTPUT_PORT);
    bool create_shared_port = create_options.Get(tPortCreateOption::SHARED) || flags.Get(tFlag::SHARED);
    if (((!selectable_create_options.Get(tPortCreateOption::OUTPUT)) || (existing_port->GetFlag(tFlag::OUTPUT_PORT) == create_output_port)) &&
        ((!selectable_create_options.Get(tPortCreateOption::SHARED)) || (existing_port->GetFlag(tFlag::SHARED) == create_shared_port)))
    {
      // port is as it should be
      return;
    }
  }
  if (existing_port)
  {
    existing_port->ManagedDelete();
  }

  // compute flags to use
  flags |= tFlag::ACCEPTS_DATA | tFlag::EMITS_DATA; // proxy port
  flags |= ToFlags(create_options, selectable_create_options);
  if (ports_flagged_finstructed)
  {
    flags |= tFlag::FINSTRUCTED;
  }

  FINROC_LOG_PRINT_TO(port_creation_list, DEBUG_VERBOSE_1, "Creating port ", name, " in IOVector ", io_vector);
  core::tAbstractPort* created_port = core::tPortFactory::CreatePort(name, io_vector, type, flags);
  if (created_port != NULL)
  {
    created_port->Init();
  }
//  if (ap != NULL && listener != NULL)
//  {
//    listener->PortCreated(ap, prototype);
//  }
}

void tPortCreationList::GetPorts(const core::tFrameworkElement& elem, std::vector<core::tAbstractPort*>& result, bool finstructed_ports_only)
{
  result.clear();
  for (auto it = elem.ChildPortsBegin(); it != elem.ChildPortsEnd(); ++it)
  {
    if ((!finstructed_ports_only) || it->GetFlag(tFlag::FINSTRUCTED))
    {
      result.push_back(&(*it));
    }
  }
}

int tPortCreationList::GetSize() const
{
  if (!io_vector)
  {
    return list.size();
  }
  int count = 0;
  for (auto it = io_vector->ChildrenBegin(); it != io_vector->ChildrenEnd(); ++it)
  {
    count++;
  }
  return count;
}

void tPortCreationList::InitialSetup(core::tFrameworkElement& managed_io_vector, tFlags port_creation_flags, const tPortCreateOptions& selectable_create_options)
{
  assert((io_vector == NULL || io_vector == &managed_io_vector) && list.empty());
  io_vector = &managed_io_vector;
  flags = port_creation_flags;
  this->selectable_create_options = selectable_create_options;
}

tPortCreationList::tEntry::tEntry(const std::string& name, const std::string& type, const tPortCreateOptions& create_options) :
  name(name),
  type(),
  create_options(create_options)
{
  rrlib::serialization::tStringInputStream sis(type);
  sis >> this->type;
  assert(this->type.Get());
}

rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tPortCreationList& list)
{
  stream.WriteByte(list.selectable_create_options.Raw());
  if (list.io_vector == NULL)
  {
    int size = list.list.size();
    stream.WriteInt(size);
    for (int i = 0; i < size; i++)
    {
      const tPortCreationList::tEntry& e = list.list[i];
      stream.WriteString(e.name);
      stream.WriteString(e.type.Get().GetName());
      stream.WriteByte(e.create_options.Raw());
    }
  }
  else
  {
    rrlib::thread::tLock lock(list.io_vector->GetStructureMutex());
    std::vector<core::tAbstractPort*> ports;
    list.GetPorts(*list.io_vector, ports, list.ports_flagged_finstructed);
    int size = ports.size();
    stream.WriteInt(size);
    for (int i = 0; i < size; i++)
    {
      core::tAbstractPort* p = ports[i];
      stream.WriteString(p->GetName());
      stream.WriteString(p->GetDataType().GetName());
      stream.WriteByte(ToPortCreateOptions(p->GetAllFlags(), list.selectable_create_options).Raw());
    }
  }
  return stream;
}

rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tPortCreationList& list)
{
  if (list.io_vector == NULL)
  {
    list.selectable_create_options = tPortCreateOptions(stream.ReadByte());
    size_t size = stream.ReadInt();
    list.list.clear();
    for (size_t i = 0u; i < size; i++)
    {
      std::string name = stream.ReadString();
      std::string type = stream.ReadString();
      list.list.emplace_back(name, type, tPortCreateOptions(stream.ReadByte()));
    }
  }
  else
  {
    rrlib::thread::tLock lock(list.io_vector->GetStructureMutex());
    stream.ReadByte(); // skip selectable create options, as this is not defined by finstruct
    size_t size = stream.ReadInt();
    std::vector<core::tAbstractPort*> existing_ports;
    list.GetPorts(*list.io_vector, existing_ports, list.ports_flagged_finstructed);
    for (size_t i = 0u; i < size; i++)
    {
      std::string name = stream.ReadString();
      std::string type_name = stream.ReadString();
      rrlib::rtti::tType type = rrlib::rtti::tType::FindType(type_name);
      if (!type)
      {
        FINROC_LOG_PRINT_STATIC(ERROR, "Error checking port from port creation deserialization: Type " + type_name + " not available");
        throw std::runtime_error("Error checking port from port creation list deserialization: Type " + type_name + " not available");
      }
      tPortCreateOptions create_options(stream.ReadByte());

      core::tAbstractPort* existing_port_with_this_name = NULL;
      for (auto it = existing_ports.begin(); it != existing_ports.end(); ++it)
      {
        if ((*it)->GetName() == name)
        {
          existing_port_with_this_name = *it;
          existing_ports.erase(it);
          break;
        }
      }
      list.CheckPort(existing_port_with_this_name, *list.io_vector, list.flags, name, type, create_options, NULL);
    }

    // delete any remaining ports
    for (size_t i = 0; i < existing_ports.size(); i++)
    {
      existing_ports[i]->ManagedDelete();
    }
  }
  return stream;
}

rrlib::xml::tNode& operator << (rrlib::xml::tNode& node, const tPortCreationList& list)
{
  if (!list.io_vector)
  {
    throw std::runtime_error("Only available on local systems");
  }

  rrlib::thread::tLock lock(list.io_vector->GetStructureMutex());
  if (!list.ports_flagged_finstructed)
  {
    node.SetAttribute("showOutputSelection", list.selectable_create_options.Get(tPortCreateOption::OUTPUT));
  }
  std::vector<core::tAbstractPort*> ports;
  list.GetPorts(*list.io_vector, ports, list.ports_flagged_finstructed);
  int size = ports.size();
  for (int i = 0; i < size; i++)
  {
    core::tAbstractPort* p = ports[i];
    rrlib::xml::tNode& child = node.AddChildNode("port");
    child.SetAttribute("name", p->GetName());
    child.SetAttribute("type", p->GetDataType().GetName());
    tFinstructable::AddDependency(p->GetDataType());
    if (list.selectable_create_options.Get(tPortCreateOption::OUTPUT))
    {
      child.SetAttribute("output", p->IsOutputPort());
    }
    if (list.selectable_create_options.Get(tPortCreateOption::SHARED) && p->GetFlag(tFlag::SHARED))
    {
      child.SetAttribute("shared", true);
    }
  }
  return node;
}

const rrlib::xml::tNode& operator >> (const rrlib::xml::tNode& node, tPortCreationList& list)
{
  if (!list.io_vector)
  {
    throw std::runtime_error("Only available on local systems");
  }

  rrlib::thread::tLock lock(list.io_vector->GetStructureMutex());
  if (!list.ports_flagged_finstructed)
  {
    list.selectable_create_options.Set(tPortCreateOption::OUTPUT, node.GetBoolAttribute("showOutputSelection"));
  }
  std::vector<core::tAbstractPort*> ports;
  list.GetPorts(*list.io_vector, ports, list.ports_flagged_finstructed);
  size_t i = 0u;
  for (rrlib::xml::tNode::const_iterator port = node.ChildrenBegin(); port != node.ChildrenEnd(); ++port, ++i)
  {
    core::tAbstractPort* ap = i < ports.size() ? ports[i] : NULL;
    std::string port_name = port->Name();
    assert(port_name.compare("port") == 0);
    tPortCreateOptions create_options;
    if (list.selectable_create_options.Get(tPortCreateOption::OUTPUT) && port->HasAttribute("output") && port->GetBoolAttribute("output"))
    {
      create_options |= tPortCreateOption::OUTPUT;
    }
    if (list.selectable_create_options.Get(tPortCreateOption::SHARED) && port->HasAttribute("shared") && port->GetBoolAttribute("shared"))
    {
      create_options |= tPortCreateOption::SHARED;
    }
    std::string dt_name = port->GetStringAttribute("type");
    rrlib::rtti::tType dt = rrlib::rtti::tType::FindType(dt_name);
    if (!dt)
    {
      FINROC_LOG_PRINT_STATIC(ERROR, "Error checking port from port creation deserialization: Type " + dt_name + " not available");
      throw std::runtime_error("Error checking port from port creation list deserialization: Type " + dt_name + " not available");
    }
    list.CheckPort(ap, *list.io_vector, list.flags, port->GetStringAttribute("name"), dt, create_options, NULL);
  }
  for (; i < ports.size(); i++)
  {
    ports[i]->ManagedDelete();
  }

  return node;
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
