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
/*!\file    plugins/runtime_construction/tGroupInterface.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-02
 *
 */
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tGroupInterface.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tStandardCreateModuleAction.h"
#include "plugins/runtime_construction/tConstructorCreateModuleAction.h"

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

static tStandardCreateModuleAction<tGroupInterface> cCREATE_ACTION("Default Interface");
static tConstructorCreateModuleAction<tGroupInterface, tGroupInterface::tDataClassification, tGroupInterface::tPortDirection, bool, bool>
cCOMPLEX_CREATE_ACTION("Interface", "Data classification, Port direction, Shared?, Unique Links");

tGroupInterface::tGroupInterface(core::tFrameworkElement* parent, const std::string& name) :
  tPortGroup(parent, name, tFlag::INTERFACE, tFlags()),
  ports("Ports", this)
{
  ports.Get().InitialSetup(*this, tFlags(), tPortCreateOption::OUTPUT | tPortCreateOption::SHARED);
}

tGroupInterface::tGroupInterface(core::tFrameworkElement* parent, const std::string& name, tDataClassification data_class, tPortDirection port_dir, bool shared, bool unique_link) :
  tPortGroup(parent, name, ComputeFlags(data_class, shared, unique_link), ComputePortFlags(port_dir, shared, unique_link)),
  ports("Ports", this)
{
  tPortCreateOptions selectable_create_options = shared ? tPortCreateOptions() : tPortCreateOptions(tPortCreateOption::SHARED);
  if (port_dir == tPortDirection::BOTH)
  {
    selectable_create_options |= tPortCreateOption::OUTPUT;
  }

  ports.Get().InitialSetup(*this, ComputePortFlags(port_dir, shared, unique_link), selectable_create_options);
}

core::tFrameworkElement::tFlags tGroupInterface::ComputeFlags(tDataClassification data_class, bool shared, bool unique_link)
{
  tFlags flags = tFlag::INTERFACE;
  if (data_class == tDataClassification::SENSOR_DATA)
  {
    flags |= tFlag::SENSOR_DATA;
  }
  else if (data_class == tDataClassification::CONTROLLER_DATA)
  {
    flags |= tFlag::CONTROLLER_DATA;
  }
  if (shared)
  {
    flags |= tFlag::SHARED;
  }
  if (unique_link)
  {
    flags |= tFlag::GLOBALLY_UNIQUE_LINK;
  }
  return flags;
}

core::tFrameworkElement::tFlags tGroupInterface::ComputePortFlags(tPortDirection port_dir, bool shared, bool unique_link)
{
  tFlags flags;
  if (shared)
  {
    flags |= tFlag::SHARED;
  }
  if (unique_link)
  {
    flags |= tFlag::GLOBALLY_UNIQUE_LINK;
  }
  if (port_dir == tPortDirection::INPUT_ONLY)
  {
    flags |= tFlag::ACCEPTS_DATA | tFlag::EMITS_DATA;
  }
  else if (port_dir == tPortDirection::OUTPUT_ONLY)
  {
    flags |= tFlag::OUTPUT_PORT | tFlag::EMITS_DATA;
  }
  return flags;
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
