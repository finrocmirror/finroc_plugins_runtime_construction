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
/*!\file    plugins/runtime_construction/tGroupInterface.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-02
 *
 * \brief   Contains tGroupInterface
 *
 * \b tGroupInterface
 *
 * Interfaces (set of ports) that can be created and edited using finstruct.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__runtime_construction__tGroupInterface_h__
#define __plugins__runtime_construction__tGroupInterface_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "core/port/tPortGroup.h"
#include "plugins/parameters/tStaticParameter.h"

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
//! Finstructable Interface
/*!
 * Interfaces (set of ports) that can be created and edited using finstruct.
 */
class tGroupInterface : public core::tPortGroup
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  enum class tDataClassification
  {
    SENSOR_DATA, CONTROLLER_DATA, ANY
  };

  enum class tPortDirection
  {
    INPUT_ONLY, OUTPUT_ONLY, BOTH
  };


  /*!
   * Default constructor
   *
   * \param name Interface name
   * \param parent Parent element
   */
  tGroupInterface(core::tFrameworkElement* parent, const std::string& name);

  /*!
   * Advanced constructor
   *
   * \param name Interface name
   * \param parent Parent element
   * \param data_class Classifies data in this interface
   * \param port_dir Which types of ports can be created in this interface?
   * \param shared Shared interface/ports?
   * \param unique_link Do ports habe globally unique link
   */
  tGroupInterface(core::tFrameworkElement* parent, const std::string& name, tDataClassification data_class, tPortDirection port_dir, bool shared, bool unique_link);

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! List of ports */
  parameters::tStaticParameter<tPortCreationList> ports;

  /*!
   * Compute port flags
   *
   * \param data_class Classifies data in this interface
   * \param port_dir Which types of ports can be created in this interface?
   * \param shared Shared interface/ports?
   * \param unique_link Do ports habe globally unique link
   * \return flags for these parameters
   */
  static tFlags ComputeFlags(tDataClassification data_class, bool shared, bool unique_link);

  /*!
   * Compute port flags
   *
   * \param data_class Classifies data in this interface
   * \param port_dir Which types of ports can be created in this interface?
   * \param shared Shared interface/ports?
   * \param unique_link Do ports habe globally unique link
   * \return flags for these parameters
   */
  static tFlags ComputePortFlags(tPortDirection port_dir, bool shared, bool unique_link);
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
