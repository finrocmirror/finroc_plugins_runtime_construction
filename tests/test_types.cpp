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
/*!\file    plugins/runtime_construction/tests/test_types.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2017-09-04
 *
 */
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tests/test_types.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "rrlib/rtti/tStaticTypeRegistration.h"
#include "rrlib/rtti_conversion/definition/tVoidFunctionConversionOperation.h"
#include "rrlib/rtti_conversion/tStaticCastOperation.h"

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
namespace tests
{

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Const values
//----------------------------------------------------------------------

const uint8_t cNO_UINT = 4;

class tExtendedMemoryBuffer : public rrlib::serialization::tMemoryBuffer
{
  using tMemoryBuffer::tMemoryBuffer;

  int internal_int;

public:
  operator const int&() const
  {
    return internal_int;
  }

  operator const uint8_t&() const
  {
    return cNO_UINT;
  }
};

}
}
}

namespace rrlib
{
namespace rtti
{
namespace conversion
{

template <>
struct StaticCastReferencesSourceWithVariableOffset<finroc::runtime_construction::tests::tExtendedMemoryBuffer, uint8_t&>
{
  enum { value = 1 };
};

auto& cBUILTIN_TYPE_CASTS = tStaticCastOperation::
                            Register<finroc::runtime_construction::tests::tExtendedMemoryBuffer, int&>()
                            .Register<finroc::runtime_construction::tests::tExtendedMemoryBuffer, uint8_t&>();

}
}
}

namespace finroc
{
namespace runtime_construction
{
namespace tests
{

rrlib::rtti::tStaticTypeRegistration init_type = rrlib::rtti::tStaticTypeRegistration("finroc_plugins_runtime_construction_test_types").
    Add<std::tuple<std::pair<uint8_t, uint8_t>, int, short>>().
    Add<std::array<uint8_t, 8>>().
    Add<std::vector<std::vector<uint8_t>>>().
    Add<tExtendedMemoryBuffer>();

void StringToVectorVectorConversion(const std::string& source, std::vector<std::vector<uint8_t>>& destination)
{
  destination.clear();
  for (size_t i = 0; i < source.length(); i++)
  {
    if ((i % 2) == 0)
    {
      destination.emplace_back();
    }
    destination.back().emplace_back(source[i]);
  }
}

const rrlib::rtti::conversion::tVoidFunctionConversionOperation<std::string, std::vector<std::vector<uint8_t>>, decltype(&StringToVectorVectorConversion), &StringToVectorVectorConversion> cSTRING_TO_VECTOR_VECTOR("ToVectorVector");

class tWrapMemoryBufferAgain: public rrlib::rtti::conversion::tRegisteredConversionOperation
{
public:
  tWrapMemoryBufferAgain() : tRegisteredConversionOperation(rrlib::util::tManagedConstCharPointer("Wrap Again", false), rrlib::rtti::tDataType<rrlib::serialization::tMemoryBuffer>(), rrlib::rtti::tDataType<tExtendedMemoryBuffer>(), &cCONVERSION_OPTION)
  {}

  static void FirstConversionFunction(const rrlib::rtti::tTypedConstPointer& source_object, const rrlib::rtti::tTypedPointer& destination_object, const rrlib::rtti::conversion::tCurrentConversionOperation& operation)
  {
    const rrlib::serialization::tMemoryBuffer& input_buffer = *source_object.Get<rrlib::serialization::tMemoryBuffer>();
    if (input_buffer.GetSize())
    {
      const tExtendedMemoryBuffer buffer(const_cast<char*>(input_buffer.GetBufferPointer()), input_buffer.GetSize());
      operation.Continue(rrlib::rtti::tTypedConstPointer(&buffer), destination_object);
    }
    else
    {
      const tExtendedMemoryBuffer buffer(0);
      operation.Continue(rrlib::rtti::tTypedConstPointer(&buffer), destination_object);
    }
  }

  static void FinalConversionFunction(const rrlib::rtti::tTypedConstPointer& source_object, const rrlib::rtti::tTypedPointer& destination_object, const rrlib::rtti::conversion::tCurrentConversionOperation& operation)
  {
    tExtendedMemoryBuffer& buffer = *destination_object.Get<tExtendedMemoryBuffer>();
    const rrlib::serialization::tMemoryBuffer& input_buffer = *source_object.Get<rrlib::serialization::tMemoryBuffer>();
    buffer = tExtendedMemoryBuffer(const_cast<char*>(input_buffer.GetBufferPointer()), input_buffer.GetSize());
  }

  static constexpr rrlib::rtti::conversion::tConversionOption cCONVERSION_OPTION = rrlib::rtti::conversion::tConversionOption(rrlib::rtti::tDataType<rrlib::serialization::tMemoryBuffer>(), rrlib::rtti::tDataType<tExtendedMemoryBuffer>(), true, &FirstConversionFunction, &FinalConversionFunction);
} cWRAP_MEMORY_BUFFER_AGAIN;

constexpr rrlib::rtti::conversion::tConversionOption tWrapMemoryBufferAgain::cCONVERSION_OPTION;

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}
