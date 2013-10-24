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
/*!\file    plugins/runtime_construction/tPortCreationList.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-02
 *
 * \brief   Contains tPortCreationList
 *
 * \b tPortCreationList
 *
 * List of ports to create.
 * Is only meant to be used in StaticParameters.
 * For this reason, it is not real-time capable and a little more memory-efficient.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__runtime_construction__tPortCreationList_h__
#define __plugins__runtime_construction__tPortCreationList_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "plugins/data_ports/tPort.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tDataTypeReference.h"

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

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Port Creation Data
/*!
 * List of ports to create.
 * Is only meant to be used in StaticParameters.
 * For this reason, it is not real-time capable and a little more memory-efficient.
 */
class tPortCreationList : public rrlib::rtti::tIsListType<false, false>, public rrlib::util::tNoncopyable
{
  typedef core::tFrameworkElement::tFlag tFlag;
  typedef core::tFrameworkElement::tFlags tFlags;

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tPortCreationList();

  /*!
   * \param port_group Port group that this list is used for editing for
   * \param flags Flags for port creation
   * \param selectable_create_options Which creation options should be visible and selectable in finstruct?
   * \param ports_flagged_finstructed Deal only with ports flagged finstructed?
   */
  tPortCreationList(core::tFrameworkElement& port_group, tFlags flags,
                    const tPortCreateOptions& selectable_create_options, bool ports_flagged_finstructed = true);

  /*!
   * Add entry to list
   *
   * \param name Port name
   * \param dt Data type
   * \param create_options Create options for this port
   */
  void Add(const std::string& name, rrlib::rtti::tType dt, const tPortCreateOptions& create_options = tPortCreateOptions());

  /*!
   * Add entry to list
   *
   * \param T Data type
   * \param name Port name
   * \param create_options Create options for this port
   */
  template <typename T>
  void Add(const std::string& name, const tPortCreateOptions& create_options = tPortCreateOptions())
  {
    Add(name, rrlib::rtti::tDataType<typename data_ports::tPort<T>::tPortBuffer>(), create_options);
  }

  /*!
   * Applies changes to another IO vector
   *
   * \param io_vector Other io vector
   * \param flags Flags to use for port creation
   */
  void ApplyChanges(core::tFrameworkElement& io_vector, tFlags flags);

  /*!
   * \return size of list
   */
  int GetSize() const;

  /*!
   * Initially set up list for local operation
   *
   * \param managed_io_vector FrameworkElement that list is wrapping
   * \param port_creation_flags Flags for port creation
   * \param selectable_create_options Which creation options should be visible and selectable in finstruct?
   */
  void InitialSetup(core::tFrameworkElement& managed_io_vector, tFlags port_creation_flags, const tPortCreateOptions& selectable_create_options = tPortCreateOptions());

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  friend rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tPortCreationList& list);
  friend rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tPortCreationList& list);
  friend rrlib::xml::tNode& operator << (rrlib::xml::tNode& node, const tPortCreationList& list);
  friend const rrlib::xml::tNode& operator >> (const rrlib::xml::tNode& node, tPortCreationList& list);

  /*!
   * Entry in list
   */
  struct tEntry
  {
  public:

    /*! Port name */
    std::string name;

    /*! Port type - as string (used remote) */
    tDataTypeReference type;

    /*! Port creation options for this specific port (e.g. output port? shared port?) */
    tPortCreateOptions create_options;

    tEntry(const std::string& name, const std::string& type, const tPortCreateOptions& create_options);

  };

  /*!
   * Which creation options should be visible and selectable in finstruct?
   * (the user can e.g. select whether port is input or output port - or shared)
   */
  tPortCreateOptions selectable_create_options;

  /*! List backend (for remote Runtimes) */
  std::vector<tEntry> list;

  /*! FrameworkElement that list is wrapping (for local Runtimes) */
  core::tFrameworkElement* io_vector;

  /*! Flags for port creation */
  tFlags flags;

  /*!
   * Deal only with ports flagged finstructed? (for local Runtimes)
   * Crated ports will be flagged finstructed. Only ports flagged finstructed will be deleted.
   * (Used for editable interfaces in groups)
   */
  bool ports_flagged_finstructed;

  /*!
   * Check whether we need to make adjustments to port
   *
   * \param existing_port Port to check
   * \param io_vector Parent
   * \param flags Creation flags
   * \param name New name
   * \param type new data type
   * \param create_options Selected create options for port
   * \param prototype Port prototype (only interesting for listener)
   */
  void CheckPort(core::tAbstractPort* existing_port, core::tFrameworkElement& io_vector, tFlags flags, const std::string& name,
                 rrlib::rtti::tType type, const tPortCreateOptions& create_options, core::tAbstractPort* prototype);

  /*!
   * Returns all child ports of specified framework element
   *
   * \param elem Framework Element
   * \param result List containing result
   * \param finstructed_ports_only Only retrieve finstructed ports?
   */
  static void GetPorts(const core::tFrameworkElement& elem, std::vector<core::tAbstractPort*>& result, bool finstructed_ports_only);
};

rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tPortCreationList& list);
rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tPortCreationList& list);
rrlib::xml::tNode& operator << (rrlib::xml::tNode& node, const tPortCreationList& list);
const rrlib::xml::tNode& operator >> (const rrlib::xml::tNode& node, tPortCreationList& list);

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
