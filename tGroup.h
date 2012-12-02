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
/*!\file    plugins/runtime_construction/tGroup.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-02
 *
 * \brief   Contains tGroup
 *
 * \b tGroup
 *
 * Simple empty group that can be created and filled using finstruct.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__runtime_construction__tGroup_h__
#define __plugins__runtime_construction__tGroup_h__

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
//! Empty group
/*!
 * Simple empty group that can be created and filled using finstruct.
 */
class tGroup : public core::tPortGroup
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tGroup(tFrameworkElement* parent, const std::string& name, tFlags flags = tFlags());

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! List of ports */
  parameters::tStaticParameter<tPortCreationList> ports;

};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
