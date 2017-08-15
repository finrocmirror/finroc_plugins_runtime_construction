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
#include "core/port/tPortGroup.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tPortCreationList.h"

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
class tEditableInterfaces : public core::tAnnotation
{

  /*! Entry in list of editable interfaces */
  typedef std::pair<core::tPortGroup*, tPortCreateOptions> tEditableInterfaceEntry;

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  /*! Listener notified whenever editable interfaces are changed */
  class tListener
  {
  public:
    virtual void OnEditableInterfacesChange() = 0;
  };

  /*!
   * Adds interface to list of editable interfaces of its parent component
   *
   * \param interface Interface to add to list (should not yet be initialized)
   * \param port_create_options Available port create options - e.g. in finstruct (are intelligently adjusted - e.g. shared option is unset if all ports are shared anyway)
   * \param at_front Add interface to front of list instead of back? (Can be used to make primary interfaces appear first in finstruct dialog)
   */
  static void AddInterface(core::tPortGroup& interface, tPortCreateOptions port_create_options, bool at_front = false);

  /*!
   * \return Listener to be notified whenever editable interfaces are changed
   */
  tListener* GetListener() const
  {
    return listener;
  }

  /*!
   * Loads and instantiates ports for one interface from information in xml node.
   * Primary use case is loading finstructable groups.
   *
   * \param node XML node to load ports from
   * \return Interface whose ports were updated
   *
   * \throw Throws different kinds of std::exceptions if loading fails
   */
  core::tPortGroup& LoadInterfacePorts(const rrlib::xml::tNode& node);

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
   * Saves ports of an interface to xml node.
   * Primary use case is saving finstructable groups (called by SaveAllNonEmptyInterfaces)
   *
   * \param node XML node to save port information to
   * \param entry Interface to save
   */
  void SaveInterfacePorts(rrlib::xml::tNode& node, tEditableInterfaceEntry& entry);

  /*!
   * \param Listener to be notified whenever editable interfaces are changed (may be nullptr to disable listener)
   */
  void SetListener(tListener* listener)
  {
    this->listener = listener;
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  friend rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tEditableInterfaces& interfaces);
  friend rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tEditableInterfaces& interfaces);

  /*! List of editable interfaces */
  std::vector<tEditableInterfaceEntry> editable_interfaces;

  /*! Listener to be notified whenever editable interfaces are changed */
  tListener* listener = nullptr;
};


rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tEditableInterfaces& interfaces);
rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tEditableInterfaces& interfaces);

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
