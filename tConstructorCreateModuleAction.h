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
/*!\file    plugins/runtime_construction/tConstructorCreateModuleAction.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-02
 *
 * \brief   Contains tConstructorCreateModuleAction
 *
 * \b tConstructorCreateModuleAction
 *
 * Construction action implementation that wraps more complex constructor
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__runtime_construction__tConstructorCreateModuleAction_h__
#define __plugins__runtime_construction__tConstructorCreateModuleAction_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <boost/lexical_cast.hpp>

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

namespace internal
{

/*! Helper to unroll tConstructorParameters */
template<typename TModule, int ARGNO, typename ... TArgs>
struct tInstantiator; // make the compiler happy - see http://stackoverflow.com/questions/1989552/gcc-error-with-variadic-templates-sorry-unimplemented-cannot-expand-identif

template<typename TModule, int ARGNO, typename ARG1, typename ... TArgs>
struct tInstantiator<TModule, ARGNO, ARG1, TArgs...>
{
  template <typename ... Args>
  inline static core::tFrameworkElement* Create(core::tFrameworkElement* parent, const std::string& name, tConstructorParameters* params, Args&... args)
  {
    typedef typename std::remove_reference<ARG1>::type tArgument;
    tArgument arg = params->GetParameter<tArgument>(ARGNO);
    return tInstantiator < TModule, ARGNO + 1, TArgs... >::Create(parent, name, params, args..., arg);
  }
};

template<typename TModule, int ARGNO>
struct tInstantiator<TModule, ARGNO>
{
  template <typename ... Args>
  inline static core::tFrameworkElement* Create(core::tFrameworkElement* parent, const std::string& name, tConstructorParameters* params, Args&... args)
  {
    return new TModule(parent, name, args...);
  }
};

/*! Helper to create constructor parameters */
template<typename ... TArgs>
struct tParameterCreator;

template<typename ARG1, typename ... TArgs>
struct tParameterCreator<ARG1, TArgs...>
{
  inline static void CreateParameter(const std::vector<std::string>& names, size_t index, tConstructorParameters& parameters)
  {
    parameters.AddParameter<ARG1>(names[index]);
    tParameterCreator<TArgs...>::CreateParameter(names, index + 1, parameters);
  }
};

template<>
struct tParameterCreator<>
{
  inline static void CreateParameter(const std::vector<std::string>& names, size_t index, tConstructorParameters& parameters)
  {
  }
};

}

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Non-default-constructor-wrapping construction action
/*!
 * Construction action implementation that wraps more complex constructor
 */
template <typename T, typename ... TArgs>
class tConstructorCreateModuleAction : public tCreateFrameworkElementAction
{
  typedef std::tuple<TArgs...> tArgsTuple;

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tConstructorCreateModuleAction(const std::string& name, const std::string& parameter_names) :
    type_name(name),
    group(GetBinary((void*)CreateModuleImplementation)),
    constructor_parameters()
  {
    // Create vector with parameter names
    std::vector<std::string> names_temp;
    boost::split(names_temp, parameter_names, boost::is_any_of(","));
    std::vector<std::string> names;
    for (size_t i = 0; i < names_temp.size(); i++)
    {
      names.push_back(boost::trim_copy(names_temp[i]));
    }
    while (names.size() < std::tuple_size<tArgsTuple>::value)
    {
      names.push_back("Parameter " + boost::lexical_cast<std::string>(names.size()));
    }

    internal::tParameterCreator<TArgs...>::CreateParameter(names, 0, constructor_parameters);
  }

  virtual core::tFrameworkElement* CreateModule(core::tFrameworkElement* parent, const std::string& name, tConstructorParameters* params) const
  {
    return CreateModuleImplementation(parent, name, params);
  }

  virtual std::string GetModuleGroup() const
  {
    return group;
  }

  virtual std::string GetName() const
  {
    return type_name;
  }

  virtual const tConstructorParameters* GetParameterTypes() const
  {
    return &constructor_parameters;
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Name of module type */
  std::string type_name;

  /*! Name of module type */
  std::string group;

  /*! List with constructor parameters */
  tConstructorParameters constructor_parameters;

  static core::tFrameworkElement* CreateModuleImplementation(core::tFrameworkElement* parent, const std::string& name, tConstructorParameters* params)
  {
    return internal::tInstantiator<T, 0, TArgs...>::Create(parent, name, params);
  }

};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
