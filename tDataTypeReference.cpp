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
/*!\file    plugins/runtime_construction/tDataTypeReference.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-02
 *
 */
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tDataTypeReference.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "plugins/data_ports/numeric/tNumber.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------

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

rrlib::rtti::tDataType<tDataTypeReference> cTYPE;

tDataTypeReference::tDataTypeReference() :
  referenced(rrlib::rtti::tDataType<data_ports::numeric::tNumber>())
{
}

rrlib::serialization::tOutputStream& operator << (rrlib::serialization::tOutputStream& stream, const tDataTypeReference& data_type_reference)
{
  stream.WriteString(data_type_reference.Get().GetName());
  return stream;
}

rrlib::serialization::tInputStream& operator >> (rrlib::serialization::tInputStream& stream, tDataTypeReference& data_type_reference)
{
  data_type_reference.Set(rrlib::rtti::tType::FindType(stream.ReadString()));
  return stream;
}

rrlib::serialization::tStringOutputStream &operator << (rrlib::serialization::tStringOutputStream& stream, const tDataTypeReference& data_type_reference)
{
  stream.Append(data_type_reference.Get().GetName());
  return stream;
}

rrlib::serialization::tStringInputStream &operator >> (rrlib::serialization::tStringInputStream& stream, tDataTypeReference& data_type_reference)
{
  data_type_reference.Set(rrlib::rtti::tType::FindType(stream.ReadAll()));
  return stream;
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
