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
/*!\file    plugins/runtime_construction/tAdministrationService.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-02
 *
 * \brief   Contains tAdministrationService
 *
 * \b tAdministrationService
 *
 * Service for administration.
 * It provides functions to create, modify and delete Finroc elements such
 * as groups, modules, ports and port connections at application runtime.
 * One port needs to be created to be able to edit application
 * structure using finstruct (currently done in dynamic_loading.cpp)
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__runtime_construction__tAdministrationService_h__
#define __plugins__runtime_construction__tAdministrationService_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "plugins/rpc_ports/tServerPort.h"

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
//! Service for administration
/*!
 * Service for administration.
 * It provides functions to create, modify and delete Finroc elements such
 * as groups, modules, ports and port connections at application runtime.
 * One port needs to be created to be able to edit application
 * structure using finstruct (currently done in dynamic_loading.cpp)
 */
class tAdministrationService : public rpc_ports::tRPCInterface
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  /*!
   * Return values for IsExecuting
   */
  enum class tExecutionStatus
  {
    NONE,
    PAUSED,
    RUNNING,
    BOTH
  };


  tAdministrationService();

  ~tAdministrationService();


  /*!
   * Connect source port to destination port
   *
   * \param source_port_handle Handle of source port
   * \param destination_port_handle Handle of destination port
   */
  void Connect(int source_port_handle, int destination_port_handle);

  /*!
   * Instantiates port for adiministration
   */
  static void CreateAdministrationPort();

  /*!
   * Created module
   *
   * \param create_action_index Index of create action
   * \param module_name Name to give new module
   * \param parent_handle Handle of parent element
   * \param serialized_creation_parameters Serialized constructor parameters in case the module requires such - otherwise empty
   * \return Empty string if it worked - otherwise error message
   */
  std::string CreateModule(uint32_t create_action_index, const std::string& module_name, int parent_handle, const rrlib::serialization::tMemoryBuffer& serialized_creation_parameters);

  /*!
   * Deletes specified framework element
   *
   * \param element_handle Handle of framework element
   */
  void DeleteElement(int element_handle);

  /*!
   * Disconnect the two ports
   *
   * \param source_port_handle Handle of source port
   * \param destination_port_handle Handle of destination port
   */
  void Disconnect(int source_port_handle, int destination_port_handle);

  /*!
   * Disconnect all ports from port with specified handle
   *
   * \param port_handle Port handle
   */
  void DisconnectAll(int port_handle);

  /*!
   * Retrieve annotation from specified framework element
   *
   * \param element_handle Handle of framework element
   * \param annotation_type_name Name of annotation type
   * \return Serialized annotation
   */
  rrlib::serialization::tMemoryBuffer GetAnnotation(int element_handle, const std::string& annotation_type_name);

  /*!
   * \return All actions for creating framework element currently registered in this runtime environment - serialized
   */
  rrlib::serialization::tMemoryBuffer GetCreateModuleActions();

  /*!
   * \return Available module libraries (.so files) that have not been loaded yet - serialized
   */
  rrlib::serialization::tMemoryBuffer GetModuleLibraries();

  /*!
   * \param root_element_handle Handle of root element to get parameter info below of
   * \return Serialized parameter info
   */
  rrlib::serialization::tMemoryBuffer GetParameterInfo(int root_element_handle);

  /*!
   * \param element_handle Handle of framework element
   * \return Is specified framework element currently executing?
   */
  tExecutionStatus IsExecuting(int element_handle);

  /*!
   * Dynamically loads specified module library (.so file)
   *
   * \param library_name File name of library to load
   * \return Available module libraries (.so files) that have not been loaded yet - serialized (Updated version)
   */
  rrlib::serialization::tMemoryBuffer LoadModuleLibrary(const std::string& library_name);

  /*!
   * Pauses execution of tasks in specified framework element
   * (possibly its parent thread container - if there is no such, then all children)
   *
   * \param element_handle Handle of framework element
   */
  void PauseExecution(int element_handle);

  /*!
   * Saves all finstructable files in this runtime environment
   */
  void SaveAllFinstructableFiles();

  /*!
   * Save contents finstructable group to xml file
   *
   * \param group_handle Handle of group to save
   */
  void SaveFinstructableGroup(int group_handle);

  /*!
   * Set/change annotation of specified framework element
   *
   * \param element_handle Handle of framework element
   * \param serialized_annotation Serialized annotation (including type of annotation)
   */
  void SetAnnotation(int element_handle, const rrlib::serialization::tMemoryBuffer& serialized_annotation);

  /*!
   * Set value of port
   *
   * \param port_handle Port handle
   * \param serialized_new_value New value - serialized
   * \return Empty string if it worked - otherwise error message
   */
  std::string SetPortValue(int port_handle, const rrlib::serialization::tMemoryBuffer& serialized_new_value);

  /*!
   * Starts executing tasks in specified framework element
   * (possibly its parent thread container - if there is no such, then all children)
   *
   * \param element_handle Handle of framework element
   */
  void StartExecution(int element_handle);

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
