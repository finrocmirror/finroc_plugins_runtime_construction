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
#include "plugins/runtime_construction/tFinstructableGroup.h"

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

//----------------------------------------------------------------------
// Const values
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------

static const core::tFrameworkElement::tFlags cRELEVANT_FLAGS = tFlag::SHARED | tFlag::VOLATILE;
static rrlib::rtti::tDataType<tPortCreationList> cTYPE;

tPortCreationList::tPortCreationList() :
  show_output_port_selection(false),
  list(),
  io_vector(NULL),
  flags(tFlags()),
  ports_flagged_finstructed()
{}

tPortCreationList::tPortCreationList(core::tFrameworkElement& port_group, tFlags flags, bool show_output_port_selection, bool ports_flagged_finstructed) :
  show_output_port_selection(show_output_port_selection),
  list(),
  io_vector(&port_group),
  flags(flags | (ports_flagged_finstructed ? tFlag::FINSTRUCTED : tFlag::PORT)),
  ports_flagged_finstructed(ports_flagged_finstructed)
{}

void tPortCreationList::Add(const std::string& name, rrlib::rtti::tType dt, bool output)
{
  rrlib::thread::tLock lock(io_vector->GetStructureMutex());
  CheckPort(NULL, *io_vector, flags, name, dt, output, NULL);
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
    CheckPort(ap2, io_vector_, flags_, ap1->GetName(), ap1->GetDataType(), ap1->IsOutputPort(), ap1);
  }
  for (size_t i = ports1.size(); i < ports2.size(); i++)
  {
    ports2[i]->ManagedDelete();
  }
}

void tPortCreationList::CheckPort(core::tAbstractPort* existing_port, core::tFrameworkElement& io_vector, tFlags flags,
                                  const std::string& name, rrlib::rtti::tType type, bool output, core::tAbstractPort* prototype)
{
  if (existing_port && existing_port->NameEquals(name) && existing_port->GetDataType() == type &&
      existing_port->GetFlag(tFlag::SHARED) == flags.Get(tFlag::SHARED) &&
      existing_port->GetFlag(tFlag::VOLATILE) == flags.Get(tFlag::VOLATILE))
  {
    if ((!show_output_port_selection) || (output == existing_port->IsOutputPort()))
    {
      return;
    }
  }
  if (existing_port)
  {
    existing_port->ManagedDelete();
  }

  // compute flags to use
  tFlags tmp = tFlag::ACCEPTS_DATA | tFlag::EMITS_DATA; // proxy port
  if (show_output_port_selection && output)
  {
    tmp |= tFlag::OUTPUT_PORT;
  }
  if (ports_flagged_finstructed)
  {
    tmp |= tFlag::FINSTRUCTED;
  }
  flags |= tmp;

  FINROC_LOG_PRINT_TO(port_creation_list, DEBUG_VERBOSE_1, "Creating port ", name, " in IOVector ", io_vector.GetQualifiedLink());
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

void tPortCreationList::InitialSetup(core::tFrameworkElement& managed_io_vector, tFlags port_creation_flags, bool show_output_port_selection)
{
  assert((io_vector == NULL || io_vector == &managed_io_vector) && list.empty());
  io_vector = &managed_io_vector;
  flags = port_creation_flags;
  this->show_output_port_selection = show_output_port_selection;
}

tPortCreationList::tEntry::tEntry(const std::string& name, const std::string& type, bool output_port) :
  name(name),
  type(),
  output_port(output_port)
{
  rrlib::serialization::tStringInputStream sis(type);
  sis >> this->type;
  assert(this->type.Get() != NULL);
}

rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tPortCreationList& list)
{
  stream.WriteBoolean(list.show_output_port_selection);
  if (list.io_vector == NULL)
  {
    int size = list.list.size();
    stream.WriteInt(size);
    for (int i = 0; i < size; i++)
    {
      const tPortCreationList::tEntry& e = list.list[i];
      stream.WriteString(e.name);
      stream.WriteString(e.type.Get().GetName());
      stream.WriteBoolean(e.output_port);
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
      stream.WriteString(p->GetCName());
      stream.WriteString(p->GetDataType().GetName());
      stream.WriteBoolean(p->IsOutputPort());
    }
  }
  return stream;
}

rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tPortCreationList& list)
{
  if (list.io_vector == NULL)
  {
    list.show_output_port_selection = stream.ReadBoolean();
    size_t size = stream.ReadInt();
    list.list.clear();
    for (size_t i = 0u; i < size; i++)
    {
      std::string name = stream.ReadString();
      std::string type = stream.ReadString();
      list.list.emplace_back(name, type, stream.ReadBoolean());
    }
  }
  else
  {
    rrlib::thread::tLock lock(list.io_vector->GetStructureMutex());
    list.show_output_port_selection = stream.ReadBoolean();
    size_t size = stream.ReadInt();
    std::vector<core::tAbstractPort*> existing_ports;
    list.GetPorts(*list.io_vector, existing_ports, list.ports_flagged_finstructed);
    for (size_t i = 0u; i < size; i++)
    {
      std::string name = stream.ReadString();
      std::string type_name = stream.ReadString();
      rrlib::rtti::tType type = rrlib::rtti::tType::FindType(type_name);
      if (type == NULL)
      {
        FINROC_LOG_PRINT(ERROR, "Error checking port from port creation deserialization: Type " + type_name + " not available");
        throw std::runtime_error("Error checking port from port creation list deserialization: Type " + type_name + " not available");
      }
      bool output = stream.ReadBoolean();

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
      list.CheckPort(existing_port_with_this_name, *list.io_vector, list.flags, name, type, output, NULL);
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
  node.SetAttribute("showOutputSelection", list.show_output_port_selection);
  std::vector<core::tAbstractPort*> ports;
  list.GetPorts(*list.io_vector, ports, list.ports_flagged_finstructed);
  int size = ports.size();
  for (int i = 0; i < size; i++)
  {
    core::tAbstractPort* p = ports[i];
    rrlib::xml::tNode& child = node.AddChildNode("port");
    child.SetAttribute("name", p->GetCName());
    child.SetAttribute("type", p->GetDataType().GetName());
    tFinstructableGroup::AddDependency(p->GetDataType());
    if (list.show_output_port_selection)
    {
      child.SetAttribute("output", p->IsOutputPort());
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
  list.show_output_port_selection = node.GetBoolAttribute("showOutputSelection");
  std::vector<core::tAbstractPort*> ports;
  list.GetPorts(*list.io_vector, ports, list.ports_flagged_finstructed);
  size_t i = 0u;
  for (rrlib::xml::tNode::const_iterator port = node.ChildrenBegin(); port != node.ChildrenEnd(); ++port, ++i)
  {
    core::tAbstractPort* ap = i < ports.size() ? ports[i] : NULL;
    std::string port_name = port->Name();
    assert(port_name.compare("port") == 0);
    bool b = false;
    if (list.show_output_port_selection)
    {
      b = port->GetBoolAttribute("output");
    }
    std::string dt_name = port->GetStringAttribute("type");
    rrlib::rtti::tType dt = rrlib::rtti::tType::FindType(dt_name);
    if (dt == NULL)
    {
      FINROC_LOG_PRINT(ERROR, "Error checking port from port creation deserialization: Type " + dt_name + " not available");
      throw std::runtime_error("Error checking port from port creation list deserialization: Type " + dt_name + " not available");
    }
    list.CheckPort(ap, *list.io_vector, list.flags, port->GetStringAttribute("name"), dt, b, NULL);
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
