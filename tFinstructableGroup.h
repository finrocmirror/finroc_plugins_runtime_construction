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
/*!\file    plugins/runtime_construction/tFinstructableGroup.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-02
 *
 * \brief   Contains tFinstructableGroup
 *
 * \b tFinstructableGroup
 *
 * The contents of FinstructableGroups can be edited using Finstruct.
 *
 * They get an XML file and optionally an attribute tree in the constructor.
 * The contents of the group are determined entirely by the contents of the
 * XML file.
 * Changes made using finstruct can be saved back to this XML file.
 */
//----------------------------------------------------------------------
#ifndef __plugins__runtime_construction__tFinstructableGroup_h__
#define __plugins__runtime_construction__tFinstructableGroup_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "core/port/tAbstractPort.h"
#include "plugins/parameters/tStaticParameter.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tStandardCreateModuleAction.h"

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
class tRuntimeConstructionPlugin;
}

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Group whose contents can be constructed/edited using finstruct.
/*!
 * The contents of FinstructableGroups can be edited using Finstruct.
 *
 * They get an XML file and optionally an attribute tree in the constructor.
 * The contents of the group are determined entirely by the contents of the
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
  std::unique_ptr<parameters::tStaticParameter<std::string>> xml_file;


  tFinstructableGroup(tFrameworkElement* parent, const std::string& name, tFlags flags = tFlags());

  /*!
   * (if the provided file does not exist, it is created, when contents are saved - and a warning is displayed)
   * (if the provided file exists, its contents are loaded when group is initialized)
   *
   * \param xml_file Name of fixed XML file (relative to finroc repository) that determines contents of this group
   */
  tFinstructableGroup(tFrameworkElement* parent, const std::string& name, const std::string& xml_file, tFlags flags = tFlags());

  /*!
   * Helper method to collect data types that need to be loaded before the contents of
   * this XML file can be instantiated.
   * (only has an effect if the current thread is currently saving this group to a file)
   *
   * \param dt Data type required to instantiate this .xml
   */
  static void AddDependency(const rrlib::rtti::tType& dt);

  /*!
   * Save contents of group back to Xml file
   *
   * \exception Throws std::runtime_error if saving fails
   */
  void SaveXml();

  /*!
   * Scan for command line arguments in specified .finroc xml file.
   * (for finroc executable)
   *
   * \param finroc_file File to scan in.
   * \return List of command line arguments.
   */
  static std::vector<std::string> ScanForCommandLineArgs(const std::string& finroc_file);

  /*!
   * Mark element as finstructed
   * (should only be called by AdminServer and CreateModuleActions)
   *
   * \param fe Framework element to mark
   * \param create_action Action with which framework element was created
   * \param params Parameters that module was created with (may be null)
   */
  static void SetFinstructed(tFrameworkElement& fe, tCreateFrameworkElementAction* create_action, tConstructorParameters* params);

  /*!
   * \param main_name Default name when group is main part
   */
  void SetMainName(const std::string main_name)
  {
    this->main_name = main_name;
  }

//----------------------------------------------------------------------
// Protected methods
//----------------------------------------------------------------------
protected:

  virtual void OnStaticParameterChange(); // TODO mark override with gcc 4.7
  virtual void PostChildInit(); // TODO mark override with gcc 4.7

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  friend class internal::tRuntimeConstructionPlugin;

  /*! Default name when group is main part */
  std::string main_name;

  /*! This string contains xml file to load - if fixed string was specified in constructor */
  const std::string fixed_xml_name;

  /*!
   * Helper method to collect .so files that need to be loaded before the contents of
   * this XML file can be instantiated.
   * (only has an effect if the current thread is currently saving this group to a file)
   *
   * \param dependency .so file that needs to be loaded
   */
  static void AddDependency(const std::string& dependency);

  /*!
   * \param cRelative port link
   * \return Port - or null if it couldn't be found
   */
  core::tAbstractPort* GetChildPort(const std::string& link);

  /*!
   * \param target_link (as from link edge)
   * \param this_group_link Qualified link of this finstructable group
   * \return Relative link to this port (or absolute link if it is globally unique)
   */
  std::string GetEdgeLink(const std::string& target_link, const std::string& this_group_link);

  /*!
   * \param ap Port
   * \param this_group_link Qualified link of this finstructable group
   * \return Relative link to this port (or absolute link if it is globally unique)
   */
  std::string GetEdgeLink(core::tAbstractPort& ap, const std::string& this_group_link);

  /*!
   * \return Returns raw xml name to use for loading and saving (either fixed string or from static parameter)
   */
  std::string GetXmlFileString();

  /*!
   * Intantiate element
   *
   * \param node xml node that contains data for instantiation
   * \param parent Parent element
   */
  void Instantiate(const rrlib::xml::tNode& node, tFrameworkElement* parent);

  /*!
   * Is this finstructable group the one responsible for saving parameter's config entry?
   *
   * \param ap Framework element to check this for (usually parameter port)
   * \return Answer.
   */
  bool IsResponsibleForConfigFileConnections(tFrameworkElement& ap) const;

  /*!
   * Loads and instantiates contents of xml file
   *
   * \param xml_file xml file to load
   */
  void LoadXml(const std::string& xml_file_);

  /*!
   * Log exception (convenience method)
   *
   * \param e Exception
   */
  void LogException(const std::exception& e);

  /*!
   * Make fully-qualified link from relative one
   *
   * \param link Relative Link
   * \param this_group_link Qualified link of this finstructable group
   * \return Fully-qualified link
   */
  std::string QualifyLink(const std::string& link, const std::string& this_group_link);

  /*!
   * Serialize children of specified framework element
   *
   * \param node XML node to serialize to
   * \param current Framework element
   */
  void SerializeChildren(rrlib::xml::tNode& node, tFrameworkElement& current);

  /*!
   * Recursive helper function for ScanForCommandLineArgs
   *
   * \param result Result list
   * \param parent Node to scan childs of
   */
  static void ScanForCommandLineArgsHelper(std::vector<std::string>& result, const rrlib::xml::tNode& parent);

  /*!
   * Perform some static initialization w.r.t. to state at program startup
   */
  static void StaticInit();
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
