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
/*!\file    plugins/runtime_construction/tDataTypeReference.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-02
 *
 * \brief   Contains tDataTypeReference
 *
 * \b tDataTypeReference
 *
 * Reference to data type.
 * Data types can be exchanged among processes with this type
 * meant for tStructureParameters.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__runtime_construction__tDataTypeReference_h__
#define __plugins__runtime_construction__tDataTypeReference_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "rrlib/rtti/rtti.h"

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
//! Refernece to data type
/*!
 * Reference to data type.
 * Data types can be exchanged among processes with this type
 * meant for tStructureParameters.
 */
class tDataTypeReference
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tDataTypeReference();

  /*!
   * \return Referenced data type - null if it doesn't exist in this runtime
   */
  inline rrlib::rtti::tType Get() const
  {
    return referenced;
  }

  /*!
   * \param dt new DataType to reference
   */
  inline void Set(rrlib::rtti::tType dt)
  {
    referenced = dt;
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! referenced data type */
  rrlib::rtti::tType referenced;

};

rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tDataTypeReference& data_type_reference);
rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tDataTypeReference& data_type_reference);
rrlib::serialization::tStringOutputStream &operator << (rrlib::serialization::tStringOutputStream& stream, const tDataTypeReference& data_type_reference);
rrlib::serialization::tStringInputStream &operator >> (rrlib::serialization::tStringInputStream& stream, tDataTypeReference& data_type_reference);

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
