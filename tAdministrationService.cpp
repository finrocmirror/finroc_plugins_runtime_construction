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
/*!\file    plugins/runtime_construction/tAdministrationService.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-02
 *
 */
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tAdministrationService.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <boost/lexical_cast.hpp>
#include "core/tRuntimeEnvironment.h"
#include "core/tRuntimeSettings.h"
#include "plugins/data_ports/tGenericPort.h"
#include "plugins/parameters/tConfigFile.h"
#include "plugins/parameters/internal/tParameterInfo.h"
#include "plugins/scheduling/tExecutionControl.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tCreateFrameworkElementAction.h"
#include "plugins/runtime_construction/tFinstructableGroup.h"
#include "plugins/runtime_construction/internal/dynamic_loading.h"

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
static const char* cPORT_NAME = "Administration";

static rpc_ports::tRPCInterfaceType<tAdministrationService> cTYPE("Administration Interface", &tAdministrationService::Connect,
    &tAdministrationService::CreateModule, &tAdministrationService::DeleteElement, &tAdministrationService::Disconnect,
    &tAdministrationService::DisconnectAll, &tAdministrationService::GetAnnotation, &tAdministrationService::GetCreateModuleActions,
    &tAdministrationService::GetModuleLibraries, &tAdministrationService::GetParameterInfo, &tAdministrationService::IsExecuting,
    &tAdministrationService::LoadModuleLibrary, &tAdministrationService::PauseExecution, &tAdministrationService::SaveAllFinstructableFiles,
    &tAdministrationService::SaveFinstructableGroup, &tAdministrationService::SetAnnotation, &tAdministrationService::SetPortValue,
    &tAdministrationService::StartExecution);

static tAdministrationService administration_service;

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------

static core::tRuntimeEnvironment& Runtime()
{
  return core::tRuntimeEnvironment::GetInstance();
}

/*!
 * Returns all relevant execution controls for start/stop command on specified element
 * (Helper method for IsExecuting, StartExecution and PauseExecution)
 *
 * \param result Result buffer for list of execution controls
 * \param element_handle Handle of element
 */
static void GetExecutionControls(std::vector<scheduling::tExecutionControl*>& result, int element_handle)
{
  core::tFrameworkElement* element = Runtime().GetElement(element_handle);
  if (element)
  {
    scheduling::tExecutionControl::FindAll(result, *element);
    if (result.size() == 0)
    {
      scheduling::tExecutionControl* control = scheduling::tExecutionControl::Find(*element);
      if (control)
      {
        result.push_back(control);
      }
    }
  }
}

tAdministrationService::tAdministrationService()
{}

tAdministrationService::~tAdministrationService()
{}

void tAdministrationService::Connect(int source_port_handle, int destination_port_handle)
{
  auto cVOLATILE = core::tFrameworkElement::tFlag::VOLATILE;
  core::tAbstractPort* source_port = Runtime().GetPort(source_port_handle);
  core::tAbstractPort* destination_port = Runtime().GetPort(destination_port_handle);
  if ((!source_port) || (!destination_port))
  {
    FINROC_LOG_PRINT(WARNING, "At least one port to be connected does not exist");
    return;
  }
  if (source_port->GetFlag(cVOLATILE) && destination_port->GetFlag(cVOLATILE))
  {
    FINROC_LOG_PRINT(WARNING, "Cannot really persistently connect two network ports: ", source_port->GetQualifiedLink(), ", ", destination_port->GetQualifiedLink());
  }

  // Connect
  if (source_port->GetFlag(cVOLATILE) && (!destination_port->GetFlag(cVOLATILE)))
  {
    destination_port->ConnectTo(source_port->GetQualifiedLink(), core::tAbstractPort::tConnectDirection::AUTO, true);
  }
  else if (destination_port->GetFlag(cVOLATILE) && (!source_port->GetFlag(cVOLATILE)))
  {
    source_port->ConnectTo(destination_port->GetQualifiedLink(), core::tAbstractPort::tConnectDirection::AUTO, true);
  }
  else
  {
    source_port->ConnectTo(*destination_port, core::tAbstractPort::tConnectDirection::AUTO, true);
  }

  // Connection check
  if (!source_port->IsConnectedTo(*destination_port))
  {
    FINROC_LOG_PRINT(WARNING, "Could not connect ports '", source_port->GetQualifiedName(), "' and '", destination_port->GetQualifiedName(), "'.");
  }
  else
  {
    FINROC_LOG_PRINT(USER, "Connected ports ", source_port->GetQualifiedName(), " ", destination_port->GetQualifiedName());
  }
}

void tAdministrationService::CreateAdministrationPort()
{
  rpc_ports::tServerPort<tAdministrationService>(administration_service, cPORT_NAME, cTYPE, core::tFrameworkElement::tFlag::SHARED);
}

std::string tAdministrationService::CreateModule(uint32_t create_action_index, const std::string& module_name, int parent_handle, const rrlib::serialization::tMemoryBuffer& serialized_creation_parameters)
{
  std::string error_message;

  try
  {
    rrlib::thread::tLock lock(Runtime().GetStructureMutex());
    const std::vector<tCreateFrameworkElementAction*>& create_actions = tCreateFrameworkElementAction::GetConstructibleElements();
    if (create_action_index >= create_actions.size())
    {
      error_message = "Invalid construction action index";
    }
    else
    {
      tCreateFrameworkElementAction* create_action = create_actions[create_action_index];
      core::tFrameworkElement* parent = core::tRuntimeEnvironment::GetInstance().GetElement(parent_handle);
      if (parent == NULL || (!parent->IsReady()))
      {
        error_message = "Parent not available. Cancelling remote module creation.";
      }
      else if ((!core::tRuntimeSettings::DuplicateQualifiedNamesAllowed()) && parent->GetChild(module_name))
      {
        error_message = std::string("Element with name '") + module_name + "' already exists. Creating another module with this name is not allowed.";
      }
      else
      {
        FINROC_LOG_PRINT(USER, "Creating Module ", parent->GetQualifiedLink(), "/", module_name);
        std::unique_ptr<tConstructorParameters> parameters;
        if (create_action->GetParameterTypes() && create_action->GetParameterTypes()->Size() > 0)
        {
          parameters.reset(create_action->GetParameterTypes()->Instantiate());
          rrlib::serialization::tInputStream input_stream(serialized_creation_parameters, rrlib::serialization::tTypeEncoding::NAMES);
          for (size_t i = 0; i < parameters->Size(); i++)
          {
            parameters::internal::tStaticParameterImplementationBase& parameter = parameters->Get(i);
            try
            {
              parameter.DeserializeValue(input_stream);
            }
            catch (const std::exception& e)
            {
              error_message = "Error deserializing value for parameter " + parameter.GetName();
              FINROC_LOG_PRINT(ERROR, e);
            }
          }
        }
        core::tFrameworkElement* created = create_action->CreateModule(parent, module_name, parameters.get());
        tFinstructableGroup::SetFinstructed(*created, create_action, parameters.get());
        created->Init();
        parameters.release();
        FINROC_LOG_PRINT(USER, "Creating Module succeeded");
      }
    }
  }
  catch (const std::exception& e)
  {
    FINROC_LOG_PRINT(ERROR, e);
    error_message = e.what();
  }

  // Possibly print error message
  if (error_message.size() > 0)
  {
    FINROC_LOG_PRINT(ERROR, error_message);
  }

  return error_message;
}

void tAdministrationService::DeleteElement(int element_handle)
{
  core::tFrameworkElement* element = Runtime().GetElement(element_handle);
  if (element && (!element->IsDeleted()))
  {
    FINROC_LOG_PRINT(USER, "Deleting element ", element->GetQualifiedLink());
    element->ManagedDelete();
  }
  else
  {
    FINROC_LOG_PRINT(ERROR, "Could not delete Framework element, because it does not appear to be available.");
  }
}

void tAdministrationService::Disconnect(int source_port_handle, int destination_port_handle)
{
  auto cVOLATILE = core::tFrameworkElement::tFlag::VOLATILE;
  core::tAbstractPort* source_port = Runtime().GetPort(source_port_handle);
  core::tAbstractPort* destination_port = Runtime().GetPort(destination_port_handle);
  if ((!source_port) || (!destination_port))
  {
    FINROC_LOG_PRINT(WARNING, "At least one port to be disconnected does not exist");
    return;
  }
  if (source_port->GetFlag(cVOLATILE))
  {
    destination_port->DisconnectFrom(source_port->GetQualifiedLink());
  }
  if (destination_port->GetFlag(cVOLATILE))
  {
    source_port->DisconnectFrom(destination_port->GetQualifiedLink());
  }
  source_port->DisconnectFrom(*destination_port);
  if (source_port->IsConnectedTo(*destination_port))
  {
    FINROC_LOG_PRINT(WARNING, "Could not disconnect ports ", source_port->GetQualifiedName(), " ", destination_port->GetQualifiedName());
  }
  else
  {
    FINROC_LOG_PRINT(USER, "Disconnected ports ", source_port->GetQualifiedName(), " ", destination_port->GetQualifiedName());
  }
}

void tAdministrationService::DisconnectAll(int port_handle)
{
  core::tAbstractPort* port = Runtime().GetPort(port_handle);
  if (!port)
  {
    FINROC_LOG_PRINT(WARNING, "Port to be disconnected does not exist");
    return;
  }
  port->DisconnectAll();
  FINROC_LOG_PRINT(USER, "Disconnected port ", port->GetQualifiedName());
}

rrlib::serialization::tMemoryBuffer tAdministrationService::GetAnnotation(int element_handle, const std::string& annotation_type_name)
{
  core::tFrameworkElement* element = Runtime().GetElement(element_handle);
  rrlib::rtti::tType type = rrlib::rtti::tType::FindType(annotation_type_name);
  if (element && element->IsReady() && type != NULL)
  {
    core::tAnnotation* result = element->GetAnnotation(type.GetRttiName());
    if (result)
    {
      rrlib::serialization::tMemoryBuffer result_buffer;
      rrlib::serialization::tOutputStream output_stream(result_buffer, rrlib::serialization::tTypeEncoding::NAMES);
      //output_stream << result->GetType();
      type.Serialize(output_stream, result);
      output_stream.Close();
      return result_buffer;
    }
  }
  else
  {
    FINROC_LOG_PRINT(ERROR, "Could not query element for annotation type ", annotation_type_name);
  }
  return rrlib::serialization::tMemoryBuffer(0);
}

rrlib::serialization::tMemoryBuffer tAdministrationService::GetCreateModuleActions()
{
  rrlib::serialization::tMemoryBuffer result_buffer;
  rrlib::serialization::tOutputStream output_stream(result_buffer, rrlib::serialization::tTypeEncoding::NAMES);
  const std::vector<tCreateFrameworkElementAction*>& module_types = tCreateFrameworkElementAction::GetConstructibleElements();
  for (size_t i = 0u; i < module_types.size(); i++)
  {
    const tCreateFrameworkElementAction& create_action = *module_types[i];
    output_stream.WriteString(create_action.GetName());
    output_stream.WriteString(create_action.GetModuleGroup());
    output_stream.WriteBoolean(create_action.GetParameterTypes());
    if (create_action.GetParameterTypes())
    {
      output_stream << *create_action.GetParameterTypes();
    }
  }
  output_stream.Close();
  return result_buffer;
}

rrlib::serialization::tMemoryBuffer tAdministrationService::GetModuleLibraries()
{
  rrlib::serialization::tMemoryBuffer result_buffer;
  rrlib::serialization::tOutputStream output_stream(result_buffer);
  std::vector<std::string> libs = internal::GetLoadableFinrocLibraries();
  for (size_t i = 0; i < libs.size(); i++)
  {
    output_stream.WriteString(libs[i]);
  }
  output_stream.Close();
  return result_buffer;
}

rrlib::serialization::tMemoryBuffer tAdministrationService::GetParameterInfo(int root_element_handle)
{
  core::tFrameworkElement* root = Runtime().GetElement(root_element_handle);
  if ((!root) || (!root->IsReady()))
  {
    FINROC_LOG_PRINT(ERROR, "Could not get parameter info for framework element ", root_element_handle);
    return rrlib::serialization::tMemoryBuffer(0);
  }

  parameters::tConfigFile* config_file = parameters::tConfigFile::Find(*root);
  rrlib::serialization::tMemoryBuffer result_buffer;
  rrlib::serialization::tOutputStream output_stream(result_buffer, rrlib::serialization::tTypeEncoding::NAMES);
  if (!config_file)
  {
    output_stream.WriteBoolean(false);
  }
  else
  {
    output_stream.WriteBoolean(true);
    output_stream.WriteInt(config_file->GetAnnotated<core::tFrameworkElement>()->GetHandle());
    output_stream << *config_file;
    for (auto it = root->SubElementsBegin(true); it != root->SubElementsEnd(); ++it)
    {
      parameters::tConfigFile* element_config = it->GetAnnotation<parameters::tConfigFile>();
      if (element_config)
      {
        output_stream.WriteByte(1);
        output_stream.WriteInt(it->GetHandle());
        output_stream.WriteString(element_config->GetFilename());
        output_stream.WriteBoolean(element_config->IsActive());
      }
      else
      {
        parameters::internal::tParameterInfo* parameter_info = it->GetAnnotation<parameters::internal::tParameterInfo>();
        if (parameter_info && config_file == parameters::tConfigFile::Find(*it))
        {
          output_stream.WriteByte(2);
          output_stream.WriteInt(it->GetHandle());
          output_stream.WriteString(parameter_info->GetConfigEntry());
        }
      }
    }
  }
  output_stream.Close();
  return result_buffer;
}

tAdministrationService::tExecutionStatus tAdministrationService::IsExecuting(int element_handle)
{
  std::vector<scheduling::tExecutionControl*> controls;
  GetExecutionControls(controls, element_handle);

  bool stopped = false;
  bool running = false;
  for (auto it = controls.begin(); it < controls.end(); it++)
  {
    stopped |= (!(*it)->IsRunning());
    running |= (*it)->IsRunning();
  }
  if (running && stopped)
  {
    return tExecutionStatus::BOTH;
  }
  else if (running)
  {
    return tExecutionStatus::RUNNING;
  }
  else if (stopped)
  {
    return tExecutionStatus::PAUSED;
  }
  return tExecutionStatus::NONE;
}

rrlib::serialization::tMemoryBuffer tAdministrationService::LoadModuleLibrary(const std::string& library_name)
{
  FINROC_LOG_PRINT(USER, "Loading library ", library_name);
  internal::DLOpen(library_name.c_str());
  return GetCreateModuleActions();
}

void tAdministrationService::PauseExecution(int element_handle)
{
  std::vector<scheduling::tExecutionControl*> controls;
  GetExecutionControls(controls, element_handle);
  if (controls.size() == 0)
  {
    FINROC_LOG_PRINT(WARNING, "Start/Pause command has not effect");
  }
  for (auto it = controls.begin(); it < controls.end(); it++)
  {
    if ((*it)->IsRunning())
    {
      (*it)->Pause();
    }
  }
}

void tAdministrationService::SaveAllFinstructableFiles()
{
  FINROC_LOG_PRINT(USER, "Saving all finstructable files in this process:");
  core::tRuntimeEnvironment& runtime_environment = core::tRuntimeEnvironment::GetInstance();
  for (auto it = runtime_environment.SubElementsBegin(); it != runtime_environment.SubElementsEnd(); ++it)
  {
    if (it->GetFlag(core::tFrameworkElement::tFlag::FINSTRUCTABLE_GROUP))
    {
      try
      {
        (static_cast<tFinstructableGroup&>(*it)).SaveXml();
      }
      catch (const std::exception& e)
      {
        FINROC_LOG_PRINT(ERROR, "Error saving finstructable group ", it->GetQualifiedLink());
        FINROC_LOG_PRINT(ERROR, e);
      }
    }
  }
  FINROC_LOG_PRINT(USER, "Done.");
}

void tAdministrationService::SaveFinstructableGroup(int group_handle)
{
  core::tFrameworkElement* group = Runtime().GetElement(group_handle);
  if (group && group->IsReady() && group->GetFlag(core::tFrameworkElement::tFlag::FINSTRUCTABLE_GROUP))
  {
    try
    {
      (static_cast<tFinstructableGroup*>(group))->SaveXml();
    }
    catch (const std::exception& e)
    {
      FINROC_LOG_PRINT(ERROR, "Error saving finstructable group ", group->GetQualifiedLink());
      FINROC_LOG_PRINT(ERROR, e);
    }
  }
  else
  {
    FINROC_LOG_PRINT(ERROR, "Could not save finstructable group, because it does not appear to be available.");
  }
}

void tAdministrationService::SetAnnotation(int element_handle, const rrlib::serialization::tMemoryBuffer& serialized_annotation)
{
  core::tFrameworkElement* element = Runtime().GetElement(element_handle);
  if (element == NULL || (!element->IsReady()))
  {
    FINROC_LOG_PRINT(ERROR, "Parent not available. Canceling setting of annotation.");
  }
  else
  {
    rrlib::serialization::tInputStream input_stream(serialized_annotation, rrlib::serialization::tTypeEncoding::NAMES);
    rrlib::rtti::tType type;
    input_stream >> type;
    if (type == NULL)
    {
      FINROC_LOG_PRINT(ERROR, "Data type not available. Canceling setting of annotation.");
    }
    else
    {
      core::tAnnotation* annotation = element->GetAnnotation(type.GetRttiName());
      if (annotation == NULL)
      {
        FINROC_LOG_PRINT(ERROR, "Creating new annotations not supported yet. Canceling setting of annotation.");
      }
      else if (typeid(*annotation).name() != type.GetRttiName())
      {
        FINROC_LOG_PRINT(ERROR, "Existing annotation has wrong type?!. Canceling setting of annotation.");
      }
      else
      {
        type.Deserialize(input_stream, annotation);
      }
    }
  }
}

std::string tAdministrationService::SetPortValue(int port_handle, const rrlib::serialization::tMemoryBuffer& serialized_new_value)
{
  core::tAbstractPort* port = Runtime().GetPort(port_handle);
  std::string error_message;
  if (port && port->IsReady())
  {
    if (port->GetFlag(core::tFrameworkElement::tFlag::FINSTRUCT_READ_ONLY))
    {
      return "Port is read-only and cannot be set from finstruct";
    }

    rrlib::thread::tLock lock(port->GetStructureMutex()); // TODO: obtaining structure lock is quite heavy-weight - however, set calls should not occur often
    if (port->IsReady())
    {
      try
      {
        rrlib::serialization::tInputStream input_stream(serialized_new_value, rrlib::serialization::tTypeEncoding::NAMES);
        rrlib::serialization::tDataEncoding encoding;
        rrlib::rtti::tType type;
        input_stream >> encoding >> type;
        data_ports::tGenericPort wrapped_port = data_ports::tGenericPort::Wrap(*port);
        data_ports::tPortDataPointer<rrlib::rtti::tGenericObject> buffer = wrapped_port.GetUnusedBuffer();
        buffer->Deserialize(input_stream, encoding);
        error_message = wrapped_port.BrowserPublish(buffer);
        if (error_message.length() > 0)
        {
          FINROC_LOG_PRINT(WARNING, "Setting value of port '", port->GetQualifiedName(), "' failed: ", error_message);
        }
      }
      catch (const std::exception& e)
      {
        FINROC_LOG_PRINT(WARNING, "Setting value of port '", port->GetQualifiedName(), "' failed: ", e);
        error_message = e.what();
      }
      return error_message;
    }
  }
  error_message = "Port with handle " + boost::lexical_cast<std::string>(port_handle) + " is not available.";
  FINROC_LOG_PRINT(WARNING, "Setting value of port failed: ", error_message);
  return error_message;
}

void tAdministrationService::StartExecution(int element_handle)
{
  std::vector<scheduling::tExecutionControl*> controls;
  GetExecutionControls(controls, element_handle);
  if (controls.size() == 0)
  {
    FINROC_LOG_PRINT(WARNING, "Start/Pause command has not effect");
  }
  for (auto it = controls.begin(); it < controls.end(); it++)
  {
    if (!(*it)->IsRunning())
    {
      (*it)->Start();
    }
  }
}


//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
