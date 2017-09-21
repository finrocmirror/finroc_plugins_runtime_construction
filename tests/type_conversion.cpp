//
// You received this file as part of Finroc
// A framework for integrated robot control
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
/*!\file    plugins/runtime_construction/tests/type_conversion.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2017-08-21
 *
 */
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <cstdlib>
#include <iostream>
#include "rrlib/util/tUnitTestSuite.h"
#include "core/tRuntimeEnvironment.h"
#include "plugins/structure/tTopLevelThreadContainer.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tests/test_types.h"

//----------------------------------------------------------------------
// Debugging
//----------------------------------------------------------------------
#include <cassert>


//----------------------------------------------------------------------
// Namespace usage
//----------------------------------------------------------------------
using namespace finroc;

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Const values
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------

class tTestTypeConversion : public rrlib::util::tUnitTestSuite
{
  RRLIB_UNIT_TESTS_BEGIN_SUITE(tTestTypeConversion);
  RRLIB_UNIT_TESTS_ADD_TEST(TestConversion);
  RRLIB_UNIT_TESTS_END_SUITE;

private:

  structure::tTopLevelThreadContainer<>* main_thread;

  virtual void InitializeTests() override
  {
    main_thread = new structure::tTopLevelThreadContainer<>("TestTypeConversion", "plugins/runtime_construction/tests/type_conversion.finroc");
    main_thread->SetCycleTime(std::chrono::milliseconds(1));
    main_thread->Init();
  }

  virtual void CleanUp() override
  {
    main_thread->ManagedDelete();
  }

  void TestConversion()
  {
    rrlib::time::tTimestamp start = rrlib::time::Now() - std::chrono::milliseconds(2);
    for (size_t i = 0; i < 10; i++)
    {
      main_thread->execution_duration.Publish(rrlib::time::Now() - start);

      for (auto it = main_thread->GetControllerOutputs().ChildPortsBegin(); it != main_thread->GetControllerOutputs().ChildPortsEnd(); ++it)
      {
        data_ports::common::tAbstractDataPort& data_port = static_cast<data_ports::common::tAbstractDataPort&>(*it);
        //std::cout << i << " " << data_port << " " << data_port.HasChanged() << std::endl;
        RRLIB_UNIT_TESTS_ASSERT(data_port.HasChanged());
        data_port.ResetChanged();
      }
    }
  }
};

RRLIB_UNIT_TESTS_REGISTER_SUITE(tTestTypeConversion);
