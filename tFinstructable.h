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
/*!\file    plugins/runtime_construction/tFinstructable.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-02
 *
 * \brief   Contains tFinstructable
 *
 * \b tFinstructable
 *
 * The contents of Finstructable framework elements can be edited using
 * Finstruct.
 *
 * They get a reference to an XML file in the constructor.
 * The contents of a finstructable are determined by the contents of the
 * XML file.
 * Changes made using finstruct can be saved back to this XML file.
 *
 * Finstructable can be added as an annotation to any framework element.
 */
//----------------------------------------------------------------------
#ifndef __plugins__runtime_construction__tFinstructable_h__
#define __plugins__runtime_construction__tFinstructable_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "rrlib/xml/tNode.h"
#include "core/port/tAbstractPort.h"

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
class tRuntimeConstructionPlugin;
}

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Group whose contents can be constructed/edited using finstruct.
/*!
 * The contents of Finstructable framework elements can be edited using
 * Finstruct.
 *
 * They get a reference to an XML file in the constructor.
 * The contents of a finstructable are determined by the contents of the
 * XML file.
 * Changes made using finstruct can be saved back to this XML file.
 *
 * Finstructable can be added as an annotation to any framework element.
 */
class tFinstructable : public core::tAnnotation
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  /*!
   * (if the provided file does not exist, it is created, when contents are saved - and a warning is displayed)
   * (if the provided file exists, its contents are loaded when group is initialized)
   * (if the provided file name is empty, nothing is loaded or saved)
   *
   * \param xml_file Reference to name of xml file that determines contents of this group
   */
  tFinstructable(const std::string& xml_file);

  /*!
   * Helper method to collect data types that need to be loaded before the contents of
   * this XML file can be instantiated.
   * (only has an effect if the current thread is currently saving this group to a file)
   *
   * \param dt Data type required to instantiate this .xml
   */
  static void AddDependency(const rrlib::rtti::tType& dt);

  /*! for rrlib_logging */
  std::string GetLogDescription() const;

  /*!
   * Loads and instantiates contents of xml file
   *
   * \param xml_file xml file to load
   */
  void LoadXml();

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
  static void SetFinstructed(core::tFrameworkElement& fe, tCreateFrameworkElementAction& create_action, tConstructorParameters* params);

  /*!
   * \param main_name Default name when group is main part
   */
  void SetMainName(const std::string main_name)
  {
    this->main_name = main_name;
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  friend class internal::tRuntimeConstructionPlugin;

  /*! Default name when group is main part */
  std::string main_name;

  /*! Reference to string that contains xml file name to load and save */
  const std::string& xml_file;


  virtual void AnnotatedObjectInitialized() override;

  /*!
   * Helper method to collect .so files that need to be loaded before the contents of
   * this XML file can be instantiated.
   * (only has an effect if the current thread is currently saving this group to a file)
   *
   * \param dependency .so file that needs to be loaded
   */
  static void AddDependency(const tSharedLibrary& dependency);

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
   * \return Root framework element that this annotation belongs to
   */
  core::tFrameworkElement* GetFrameworkElement() const
  {
    return this->GetAnnotated<core::tFrameworkElement>();
  }

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
  void Instantiate(const rrlib::xml::tNode& node, core::tFrameworkElement* parent);

  /*!
   * Is this finstructable group the one responsible for saving parameter's config entry?
   *
   * \param ap Framework element to check this for (usually parameter port)
   * \return Answer.
   */
  bool IsResponsibleForConfigFileConnections(core::tFrameworkElement& ap) const;

  /*!
   * Helper function for loading parameter links
   *
   * \param node Parameter XML node
   * \param parameter_port Abstract Port that should be a parameter
   */
  void LoadParameter(const rrlib::xml::tNode& node, core::tAbstractPort& parameter_port);

  /*!
   * Recursive helper function for loading parameter_links section of XML file
   *
   * \param node Current XML node
   * \param element Framework element corresponding to node
   */
  void ProcessParameterLinksNode(const rrlib::xml::tNode& node, core::tFrameworkElement& element);

  /*!
   * Make fully-qualified link from relative one
   *
   * \param link Relative Link
   * \param this_group_link Qualified link of this finstructable group
   * \return Fully-qualified link
   */
  std::string QualifyLink(const std::string& link, const std::string& this_group_link);

  /*!
   * Recursive helper function for saving parameter_links section of XML file
   *
   * \param node Current XML node
   * \param element Framework element corresponding to node
   * \result True if any parameters were saved below node
   */
  bool SaveParameterConfigEntries(rrlib::xml::tNode& node, core::tFrameworkElement& element);

  /*!
   * Serialize children of specified framework element
   *
   * \param node XML node to serialize to
   * \param current Framework element
   */
  void SerializeChildren(rrlib::xml::tNode& node, core::tFrameworkElement& current);

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
