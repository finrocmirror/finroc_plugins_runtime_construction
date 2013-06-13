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
#include "core/port/tPortGroup.h"
#include <bitset>

//----------------------------------------------------------------------
// Internal includes with ""
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

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  /*! Maximum number of editable interfaces */
  enum { cMAX_INTERFACE_COUNT = 32 };

  /*!
   * Static info for a single interface type of a component type
   * (exists once for every interface type)
   */
  struct tStaticInterfaceInfo
  {
    /*! Interface name */
    const char* name;

    /*! Any extra flags to assign to interface */
    core::tFrameworkElement::tFlags extra_interface_flags;

    /*! Any extra flags to assign to all ports */
    core::tFrameworkElement::tFlags default_port_flags;

    /*! Should output port selection be visible in finstruct? (the user can select whether port is input or output port) */
    bool show_output_port_selection;
  };

  /*! Should not be called. Exists for rrlib_rtti */
  tEditableInterfaces();

  /*!
   * \param static_interface_info Static info on editable interfaces
   * \param interface_array Pointer to array (e.g. in group) containing editable interfaces - some entries may be null
   * \param shared_interfaces Bitset that defines which interfaces should be shared with other runtime environments
   */
  tEditableInterfaces(const std::vector<tStaticInterfaceInfo>& static_interface_info, core::tPortGroup** interface_array,
                      std::bitset<cMAX_INTERFACE_COUNT> shared_interfaces);

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  friend rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tEditableInterfaces& interfaces);
  friend rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tEditableInterfaces& interfaces);

  /*! Static info on editable interfaces */
  const std::vector<tStaticInterfaceInfo>& static_interface_info;

  /*! Pointer to array (e.g. in group) containing editable interfaces - some entries may be null */
  core::tPortGroup** interface_array;

  /*! Bitset that defines which interfaces should be shared with other runtime environments */
  const std::bitset<cMAX_INTERFACE_COUNT> shared_interfaces;
};


rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tEditableInterfaces& interfaces);
rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tEditableInterfaces& interfaces);

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif