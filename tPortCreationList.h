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

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Port Creation Data
/*!
 * List of ports to create.
 * Is only meant to be used in StaticParameters.
 * For this reason, it is not real-time capable and a little more memory-efficient.
 */
class tPortCreationList : public rrlib::rtti::tIsListType<false, false>, boost::noncopyable
{
  typedef core::tFrameworkElement::tFlag tFlag;
  typedef core::tFrameworkElement::tFlags tFlags;

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tPortCreationList();

  /*!
   * Add entry to list
   *
   * \param name Port name
   * \param dt Data type
   * \param output Output port? (possibly irrelevant)
   */
  void Add(const std::string& name, rrlib::rtti::tType dt, bool output);

  /*!
   * Add entry to list
   *
   * \param T Data type
   * \param name Port name
   * \param output Output port? (possibly irrelevant)
   */
  template <typename T>
  void Add(const std::string& name, bool output = false)
  {
    Add(name, rrlib::rtti::tDataType<typename data_ports::tPort<T>::tPortBuffer>(), output);
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
  inline int GetSize() const
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

  /*!
   * Initially set up list for local operation
   *
   * \param managed_io_vector FrameworkElement that list is wrapping
   * \param port_creation_flags Flags for port creation
   * \param show_output_port_selection Should output port selection be visible in finstruct?
   */
  void InitialSetup(core::tFrameworkElement& managed_io_vector, tFlags port_creation_flags, bool show_output_port_selection);

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

    /*! Output port? */
    bool output_port;

    tEntry(const std::string& name, const std::string& type, bool output_port);

  };

  /*! Should output port selection be visible in finstruct? */
  bool show_output_port_selection;

  /*! List backend (for remote Runtimes) */
  std::vector<tEntry> list;

  /*! FrameworkElement that list is wrapping (for local Runtimes) */
  core::tFrameworkElement* io_vector;

  /*! Flags for port creation */
  tFlags flags;


  /*!
   * Check whether we need to make adjustments to port
   *
   * \param ap Port to check
   * \param io_vector Parent
   * \param flags Creation flags
   * \param name New name
   * \param dt new data type
   * \param output output port
   * \param prototype Port prototype (only interesting for listener)
   */
  void CheckPort(core::tAbstractPort* ap, core::tFrameworkElement& io_vector, tFlags flags, const std::string& name, rrlib::rtti::tType dt, bool output, core::tAbstractPort* prototype);

  /*!
   * Returns all child ports of specified framework element
   *
   * \param elem Framework Element
   * \param result List containing result
   */
  static void GetPorts(const core::tFrameworkElement& elem, std::vector<core::tAbstractPort*>& result);
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
