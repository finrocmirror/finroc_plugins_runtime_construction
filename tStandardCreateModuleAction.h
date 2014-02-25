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
/*!\file    plugins/runtime_construction/tStandardCreateModuleAction.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-02
 *
 * \brief   Contains tStandardCreateModuleAction
 *
 * \b tStandardCreateModuleAction
 *
 * Default create module action for finroc elements.
 * Modules need to have a constructor taking parent as first parameter and name as second.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__runtime_construction__tStandardCreateModuleAction_h__
#define __plugins__runtime_construction__tStandardCreateModuleAction_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tCreateFrameworkElementAction.h"

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
//! Default create action implementation
/*!
 * Default create module action for finroc elements.
 * Modules need to have a constructor taking parent as first parameter and name as second.
 */
template<typename T>
class tStandardCreateModuleAction : public tCreateFrameworkElementAction
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  /*!
   * \param group Name of module group
   * \param type_name Name of module type
   */
  tStandardCreateModuleAction(const std::string& type_name_) :
    group(),
    type_name(type_name_)
  {
    group = GetBinary((void*)CreateModuleImplementation);
  }


  virtual core::tFrameworkElement* CreateModule(core::tFrameworkElement* parent, const std::string& name, tConstructorParameters* params) const override
  {
    return CreateModuleImplementation(parent, name);
  }

  virtual tSharedLibrary GetModuleGroup() const override
  {
    return group;
  }

  virtual std::string GetName() const override
  {
    return type_name;
  }

  virtual const tConstructorParameters* GetParameterTypes() const override
  {
    return NULL;
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Name of module type */
  tSharedLibrary group;

  /*! Name of module type */
  std::string type_name;


  /*! necessary to determine binary (?) */
  static core::tFrameworkElement* CreateModuleImplementation(core::tFrameworkElement* parent, const std::string& name)
  {
    return new T(parent, name);
  }
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
