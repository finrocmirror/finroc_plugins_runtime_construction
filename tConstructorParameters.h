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
/*!\file    plugins/runtime_construction/tConstructorParameters.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-02
 *
 * \brief   Contains tConstructorParameters
 *
 * \b tConstructorParameters
 *
 * Parameters used to instantiate a module
 * are stored separately from static parameters.
 * Therefore, we need an extra class for this.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__runtime_construction__tConstructorParameters_h__
#define __plugins__runtime_construction__tConstructorParameters_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "plugins/parameters/tStaticParameter.h"

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
//! Constructor parameter list
/*!
 * Parameters used to instantiate a module
 * are stored separately from static parameters.
 * Therefore, we need an extra class for this.
 */
class tConstructorParameters : public parameters::internal::tStaticParameterList
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tConstructorParameters();

  template <typename T, typename ... ARGS>
  void AddParameter(ARGS && ... args)
  {
    tParameter<T> p(this, args...);
  }

  virtual std::string GetLogDescription() const;

  template <typename T>
  T GetParameter(size_t index)
  {
    tParameter<T> p(Get(index));
    return p.Get();
  }

  /*!
   * If this is constructor parameter prototype: create instance that can be filled with values
   * (More or less clones parameter list (deep-copy without values))
   *
   * \return Cloned list
   */
  tConstructorParameters* Instantiate() const;

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  template <typename T>
  class tParameter : public parameters::tStaticParameter<T>
  {
    typedef typename parameters::tStaticParameter<T>::tImplementation tImplementation;

  public:

    template <typename ... ARGS>
    tParameter(tConstructorParameters* parent, const ARGS&... args)
    {
      core::tPortWrapperBase::tConstructorArguments<parameters::internal::tParameterCreationInfo<T>> creation_info(args...);
      tImplementation* implementation = tImplementation::CreateInstance(creation_info, true);
      this->SetImplementation(implementation);
      parent->Add(*implementation);
    }

    tParameter(parameters::internal::tStaticParameterImplementationBase& raw)
    {
      this->SetImplementation(&static_cast<tImplementation&>(raw));
    }
  };

};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
