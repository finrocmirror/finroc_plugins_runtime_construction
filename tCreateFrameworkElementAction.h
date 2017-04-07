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
/*!\file    plugins/runtime_construction/tCreateFrameworkElementAction.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-02
 *
 * \brief   Contains tCreateFrameworkElementAction
 *
 * \b tCreateFrameworkElementAction
 *
 * Classes that implement this interface provide a generic method for
 * creating modules/groups etc.
 *
 * When such actions are instantiated, they are automatically added to list of constructible elements.
 */
//----------------------------------------------------------------------
#ifndef __plugins__runtime_construction__tCreateFrameworkElementAction_h__
#define __plugins__runtime_construction__tCreateFrameworkElementAction_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tConstructorParameters.h"
#include "plugins/runtime_construction/tSharedLibrary.h"

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
//! Base class for all actions that are available for creating framework elements.
/*!
 * Classes that implement this interface provide a generic method for
 * creating modules/groups etc.
 *
 * When such actions are instantiated, they are automatically added to list of constructible elements.
 */
class tCreateFrameworkElementAction : private rrlib::util::tNoncopyable
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  typedef rrlib::serialization::tRegister<tCreateFrameworkElementAction*, 64, 128, uint16_t> tRegister;

  tCreateFrameworkElementAction();

  virtual ~tCreateFrameworkElementAction() = default;

  /*!
   * Create Module (or Group)
   *
   * \param name Name of instantiated module
   * \param parent Parent of instantiated module
   * \param params Parameters
   * \return Created Module (or Group)
   */
  virtual core::tFrameworkElement* CreateModule(core::tFrameworkElement* parent, const std::string& name, tConstructorParameters* params = nullptr) const = 0;

  /*!
   * \return Returns .so file in which address provided as argument is found by dladdr
   */
  tSharedLibrary GetBinary(void* addr);

  /*!
   * \return List with framework element types that can be instantiated in this runtime using this standard mechanism
   */
  static const tRegister& GetConstructibleElements();

  /*!
   * \return Returns name of group to which this create module action belongs
   */
  virtual tSharedLibrary GetModuleGroup() const = 0;

  /*!
   * \return Name of module type to be created
   */
  virtual std::string GetName() const = 0;

  /*!
   * \return Returns types of parameters that the create method requires
   */
  virtual const tConstructorParameters* GetParameterTypes() const = 0;

  /*!
   * \return Whether create action is deprecated;
   */
  virtual bool IsDeprecated() const;

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
