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
/*!\file    plugins/runtime_construction/tFinstructableGroup.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-02
 *
 */
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "core/tFrameworkElementTags.h"
#include "core/file_lookup.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tStandardCreateModuleAction.h"
#include "plugins/runtime_construction/tFinstructable.h"

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

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Group whose contents can be constructed/edited using finstruct.
/*!
 * (This class only exists for backward-compatibility - as some .xml files
 *  referencing FinstructableGroups exists)
 *
 * The contents of FinstructableGroups can be edited using Finstruct.
 *
 * The contents of the group are determined entirely by the contents of an
 * XML file.
 * Changes made using finstruct can be saved back to this XML file.
 */
class tFinstructableGroup : public core::tFrameworkElement
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  /*!
   * Contains name of XML file to use
   * Parameter only exists if no (fixed) XML file was provided via constructor
   */
  parameters::tStaticParameter<std::string> xml_file;


  tFinstructableGroup(tFrameworkElement* parent, const std::string& name, tFlags flags = tFlags()) :
    tFrameworkElement(parent, name, flags | tFlag::FINSTRUCTABLE_GROUP),
    xml_file("XML file", this, ""),
    xml_filename()
  {
    core::tFrameworkElementTags::AddTag(*this, "group");
    this->EmplaceAnnotation<tFinstructable>(xml_filename);
  }

//----------------------------------------------------------------------
// Protected methods
//----------------------------------------------------------------------
protected:

  virtual void OnStaticParameterChange() override
  {
    if (xml_file.HasChanged() && xml_file.Get().length() > 0)
    {
      xml_filename = xml_file.Get();
      //if (this.childCount() == 0) { // TODO: original intension: changing xml files to mutliple existing ones in finstruct shouldn't load all of them
      if (core::FinrocFileExists(xml_filename))
      {
        this->GetAnnotation<tFinstructable>()->LoadXml();
      }
      else
      {
        FINROC_LOG_PRINT(DEBUG, "Cannot find XML file ", xml_filename, ". Creating empty group. You may edit and save this group using finstruct.");
      }
    }
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! String that always contains current xml file name */
  std::string xml_filename;

};

tStandardCreateModuleAction<tFinstructableGroup> cCREATE_ACTION_FOR_T_FINSTRUCTABLE_GROUP("Finstructable Group");

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
