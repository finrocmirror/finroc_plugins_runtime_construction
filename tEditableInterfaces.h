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
/*!\file    plugins/runtime_construction/tEditableInterfaces.h
 *
 * \author  Max Reichardt
 *
 * \date    2013-06-04
 *
 * \brief   Contains tEditableInterfaces
 *
 * \b tEditableInterfaces
 *
 * Annotation for framework elements (usually finstructable groups)
 * whose interfaces can be edited via finstruct (via the administration
 * service to be more specific).
 *
 * Adding this annotation makes the specified interfaces editable.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__runtime_construction__tEditableInterfaces_h__
#define __plugins__runtime_construction__tEditableInterfaces_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <bitset>

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tInterfaces.h"

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
// Class declaration
//----------------------------------------------------------------------
//! Annotation for elements with editable interfaces
/*!
 * Annotation for framework elements (usually finstructable groups)
 * whose interfaces can be edited via finstruct (via the administration
 * service to be more specific).
 *
 * Adding this annotation makes the specified interfaces editable.
 */
class tEditableInterfaces : public tInterfaces
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  using tInterfaces::tInterfaces;

  /*!
   * Loads and instantiates ports for one interface from information in xml node.
   * Primary use case is loading finstructable groups.
   *
   * \param node XML node to load ports from
   *
   * \throw Throws different kinds of std::exceptions if loading fails
   */
  void LoadInterfacePorts(const rrlib::xml::tNode& node);

  /*!
   * Saves port information for all interfaces containing ports to the specified parent node.
   * Primary use case is saving finstructable groups.
   * For each non-empty interface a child node will be created:
   *
   * <interface name="name">
   *   <port name="port 1 name" type="Number"/>
   *   <port name="port 2 name" type="Number"/>
   *   ...
   * </interface>
   *
   * \param parent_node Node to add 'interface'-nodes to
   */
  void SaveAllNonEmptyInterfaces(rrlib::xml::tNode& parent_node);

  /*!
   * Saves port of an interface to xml node.
   * Primary use case is saving finstructable groups (called by SaveAllNonEmptyInterfaces)
   *
   * \param node XML node to save port information to
   * \param index Index of interface to save port information of
   */
  void SaveInterfacePorts(rrlib::xml::tNode& node, size_t index);

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

};


rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tEditableInterfaces& interfaces);
rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tEditableInterfaces& interfaces);

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
