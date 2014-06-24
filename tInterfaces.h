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
/*!\file    plugins/runtime_construction/tInterfaces.h
 *
 * \author  Max Reichardt
 *
 * \date    2014-06-25
 *
 * \brief   Contains tEditableInterfaces
 *
 * \b tEditableInterfaces
 *
 * Annotation for framework elements (usually finstructable groups)
 * whose interfaces can be created when needed.
 */
//----------------------------------------------------------------------
#ifndef __plugins__runtime_construction__tInterfaces_h__
#define __plugins__runtime_construction__tInterfaces_h__

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

/*! Port Creation options for single ports in port creation list */
enum class tPortCreateOption
{
  OUTPUT, //!< Create an output port?
  SHARED  //!< Creast a shared port?
};

/*!
 * Set of port creation options
 */
typedef rrlib::util::tEnumBasedFlags<tPortCreateOption, uint8_t> tPortCreateOptions;

constexpr tPortCreateOptions operator | (const tPortCreateOptions& options1, const tPortCreateOptions& options2)
{
  return tPortCreateOptions(options1.Raw() | options2.Raw());
}

constexpr tPortCreateOptions operator | (tPortCreateOption option1, tPortCreateOption option2)
{
  return tPortCreateOptions(option1) | tPortCreateOptions(option2);
}

class tEditableInterfaces;

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Annotation for elements with lazily created interfaces
/*!
 * Annotation for framework elements (usually finstructable groups)
 * whose interfaces can be created when needed.
 */
class tInterfaces : public core::tAnnotation
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

    /*! Which creation options should be visible and selectable in finstruct? */
    tPortCreateOptions selectable_create_options;
  };

  /*! Should not be called. Exists for rrlib_rtti */
  tInterfaces();

  /*!
   * \param static_interface_info Static info on editable interfaces
   * \param interface_array Pointer to array (e.g. in group) containing editable interfaces - some entries may be null
   * \param shared_interfaces Bitset that defines which interfaces should be shared with other runtime environments
   */
  tInterfaces(const std::vector<tStaticInterfaceInfo>& static_interface_info, core::tPortGroup** interface_array,
              std::bitset<cMAX_INTERFACE_COUNT> shared_interfaces);

  /*!
   * Creates interface according to this static interface info.
   * Places interface in interface array provided in constructor
   *
   * \param parent Parent framework element
   * \param index Index of interface to create
   * \param initialize Initialize created array?
   * \return Returns Reference to created interface
   */
  core::tPortGroup& CreateInterface(core::tFrameworkElement* parent, size_t index, bool initialize) const;

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  friend class tEditableInterfaces;
  friend rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tEditableInterfaces& interfaces);
  friend rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tEditableInterfaces& interfaces);

  /*! Static info on editable interfaces */
  const std::vector<tStaticInterfaceInfo>& static_interface_info;

  /*! Pointer to array (e.g. in group) containing editable interfaces - some entries may be null */
  core::tPortGroup** interface_array;

  /*! Bitset that defines which interfaces should be shared with other runtime environments */
  const std::bitset<cMAX_INTERFACE_COUNT> shared_interfaces;

  /*!
   * \interface_index Index of interface
   * \return Default port flags for specified interface
   */
  core::tFrameworkElement::tFlags GetDefaultPortFlags(size_t interface_index) const;
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
