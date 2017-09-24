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
/*!\file    plugins/runtime_construction/tFinstructable.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-02
 *
 */
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tFinstructable.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <set>
#include "rrlib/util/string.h"
#include "core/file_lookup.h"
#include "core/tRuntimeEnvironment.h"
#include "plugins/parameters/internal/tParameterInfo.h"
#include "rrlib/rtti/tStaticTypeRegistration.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tEditableInterfaces.h"
#include "plugins/runtime_construction/dynamic_loading.h"

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
typedef core::tFrameworkElement tFrameworkElement;
typedef tFrameworkElement::tFlag tFlag;

//----------------------------------------------------------------------
// Const values
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------

/*! Thread currently saving finstructable group */
static rrlib::thread::tThread* saving_thread = nullptr;

/*! Temporary variable for saving: .so files that should be loaded prior to instantiating this group */
static std::set<tSharedLibrary> dependencies_tmp;

/*! Loaded finroc libraries at startup */
static std::set<tSharedLibrary> startup_loaded_finroc_libs;

/*! We do not want to have this prefix in XML file names, as this will not be found when a system installation is used */
static const char* cUNWANTED_XML_FILE_PREFIX = "sources/cpp/";

/*! Characters that are not escaped in path URIs */
static const char* cUNENCODED_RESERVED_CHARACTERS_PATH = "!$&'()*+,;= @";

/*! Current version of file format (YYMM) */
static const uint cVERSION = 1703;

static rrlib::uri::tPath ReplaceInterfaceInPath(const rrlib::uri::tPath& path, const std::string& new_interface)
{
  if (path.Size() < 2)
  {
    return path;
  }
  std::vector<rrlib::uri::tStringRange> path_components;
  for (uint i = 0; i < path.Size(); i++)
  {
    path_components.push_back(path[i]);
  }
  path_components[path_components.size() - 2] = rrlib::uri::tStringRange(new_interface);
  return rrlib::uri::tPath(path.IsAbsolute(), path_components.begin(), path_components.end());
}

tFinstructable::tFinstructable(const std::string& xml_file) :
  main_name(),
  xml_file(xml_file)
{
}

void tFinstructable::AddDependency(const tSharedLibrary& dependency)
{
#ifndef RRLIB_SINGLE_THREADED
  if (&rrlib::thread::tThread::CurrentThread() == saving_thread)
#endif
  {
    if (startup_loaded_finroc_libs.find(dependency) == startup_loaded_finroc_libs.end())
    {
      dependencies_tmp.insert(dependency);
    }
  }
}

void tFinstructable::AddDependency(const rrlib::rtti::tType& dt)
{
  auto shared_library_string = rrlib::rtti::tStaticTypeRegistration::GetTypeRegistrationSharedLibrary(dt);
  if (shared_library_string)
  {
    AddDependency(tSharedLibrary(shared_library_string));
  }
}

core::tAbstractPort* tFinstructable::GetChildPort(const core::tPath& path)
{
  tFrameworkElement* element = GetFrameworkElement()->GetDescendant(path);
  if (element && element->IsPort())
  {
    return static_cast<core::tAbstractPort*>(element);
  }
  return nullptr;
}

core::tPath tFinstructable::GetConnectorPath(const core::tPath& target_path, const core::tPath& this_group_path)
{
  if (target_path.CountCommonElements(this_group_path) == this_group_path.Size())
  {
    return core::tPath(false, target_path.Begin() + this_group_path.Size(), target_path.End());
  }
  return target_path;
}

core::tPath tFinstructable::GetConnectorPath(core::tAbstractPort& port, const core::tPath& this_group_path)
{
  core::tPath port_path = port.GetPath();
  tFrameworkElement* alt_root = port.GetParentWithFlags(tFlag::ALTERNATIVE_LOCAL_URI_ROOT);
  if (alt_root && alt_root->IsChildOf(*GetFrameworkElement()))
  {
    return core::tPath(true, port_path.Begin() + alt_root->GetPath().Size(), port_path.End());
  }
  return core::tPath(true, port_path.Begin() + this_group_path.Size(), port_path.End());
}

std::string tFinstructable::GetLogDescription() const
{
  if (GetFrameworkElement())
  {
    std::stringstream stream;
    stream << *GetFrameworkElement();
    return stream.str();
  }
  return "Unattached Finstructable";
}

std::string tFinstructable::GetXmlFileString()
{
  std::string s = xml_file;
  if (s.find(cUNWANTED_XML_FILE_PREFIX) != std::string::npos)
  {
    FINROC_LOG_PRINT(WARNING, "XML file name '", s, "' is deprecated, because it contains '", cUNWANTED_XML_FILE_PREFIX, "'. File will not be found when installed.");
  }
  return s;
}

void tFinstructable::Instantiate(const rrlib::xml::tNode& node, tFrameworkElement* parent)
{
  std::string name = "component name not read";
  try
  {
    name = node.GetStringAttribute("name");
    std::string group = node.GetStringAttribute("group");
    std::string type = node.GetStringAttribute("type");

    // find action
    tCreateFrameworkElementAction& action = LoadComponentType(group, type);

    // read parameters
    rrlib::xml::tNode::const_iterator child_node = node.ChildrenBegin();
    const rrlib::xml::tNode* parameters = nullptr;
    const rrlib::xml::tNode* constructor_params = nullptr;
    std::string p_name = child_node->Name();
    if (p_name == "constructor")
    {
      constructor_params = &(*child_node);
      ++child_node;
      p_name = child_node->Name();
    }
    if (p_name == "parameters")
    {
      parameters = &(*child_node);
      ++child_node;
    }

    // create mode
    tFrameworkElement* created = nullptr;
    tConstructorParameters* spl = nullptr;
    if (constructor_params != nullptr)
    {
      spl = action.GetParameterTypes()->Instantiate();
      spl->Deserialize(*constructor_params, true);
    }
    created = action.CreateModule(parent, name, spl);
    SetFinstructed(*created, action, spl);
    if (parameters)
    {
      created->GetAnnotation<parameters::internal::tStaticParameterList>()->Deserialize(*parameters, true);
    }
    created->Init();

    // continue with children
    for (; child_node != node.ChildrenEnd(); ++child_node)
    {
      std::string name2 = child_node->Name();
      if (name2 == "element")
      {
        Instantiate(*child_node, created);
      }
      else
      {
        FINROC_LOG_PRINT(WARNING, "Unknown XML tag: ", name2);
      }
    }
  }
  catch (const rrlib::xml::tException& e)
  {
    FINROC_LOG_PRINT(ERROR, "Failed to instantiate component '", name, "'. XML Exception: ", e.what(), ". Skipping.");
  }
  catch (const std::exception& e)
  {
    FINROC_LOG_PRINT(ERROR, "Failed to instantiate component '", name, "'. ", e.what(), ". Skipping.");
  }
}

bool tFinstructable::IsResponsibleForConfigFileConnections(tFrameworkElement& ap) const
{
  return parameters::internal::tParameterInfo::IsFinstructableGroupResponsibleForConfigFileConnections(*GetFrameworkElement(), ap);
}

void tFinstructable::LoadParameter(const rrlib::xml::tNode& node, core::tAbstractPort& parameter_port)
{
  parameters::internal::tParameterInfo* pi = parameter_port.GetAnnotation<parameters::internal::tParameterInfo>();
  bool outermost_group = GetFrameworkElement()->GetParent() == &(core::tRuntimeEnvironment::GetInstance());
  if (!pi)
  {
    FINROC_LOG_PRINT(WARNING, "Port is not a parameter: '", parameter_port, "'. Parameter entry is not loaded.");
  }
  else
  {
    if (outermost_group && node.HasAttribute("cmdline") && (!IsResponsibleForConfigFileConnections(parameter_port)))
    {
      pi->SetCommandLineOption(node.GetStringAttribute("cmdline"));
    }
    else
    {
      pi->Deserialize(node, true, outermost_group);
    }
    try
    {
      pi->LoadValue();
    }
    catch (const std::exception& e)
    {
      FINROC_LOG_PRINT(WARNING, "Unable to load parameter value for '", parameter_port, "'. ", e);
    }
  }
}

void tFinstructable::LoadXml()
{
  {
    rrlib::thread::tLock lock2(core::tRuntimeEnvironment::GetInstance().GetStructureMutex());
    try
    {
      FINROC_LOG_PRINT(DEBUG, "Loading XML: ", GetXmlFileString());
      rrlib::xml::tDocument doc(core::GetFinrocXMLDocument(GetXmlFileString(), false));
      rrlib::xml::tNode& root = doc.RootNode();
      core::tPath path_to_this = GetFrameworkElement()->GetPath();
      if (main_name.length() == 0 && root.HasAttribute("defaultname"))
      {
        main_name = root.GetStringAttribute("defaultname");
      }
      uint version = 0;
      if (root.HasAttribute("version"))
      {
        version = root.GetIntAttribute("version");
      }

      // load dependencies
      if (root.HasAttribute("dependencies"))
      {
        std::vector<std::string> deps;
        std::stringstream stream(root.GetStringAttribute("dependencies"));
        std::string dependency;
        while (std::getline(stream, dependency, ','))
        {
          rrlib::util::TrimWhitespace(dependency);
          tSharedLibrary dep(dependency);
          std::vector<tSharedLibrary> loadable = GetLoadableFinrocLibraries();
          bool loaded = false;
          for (size_t i = 0; i < loadable.size(); i++)
          {
            if (loadable[i] == dep)
            {
              try
              {
                DLOpen(dep);
                loaded = true;
                break;
              }
              catch (const std::exception& exception)
              {
                FINROC_LOG_PRINT(ERROR, exception);
              }
            }
          }
          if (!loaded)
          {
            std::set<tSharedLibrary> loaded_libs = GetLoadedFinrocLibraries();
            if (loaded_libs.find(dep) == loaded_libs.end())
            {
              FINROC_LOG_PRINT(WARNING, "Dependency ", dep.ToString(true), " is not available.");
            }
          }
        }
      }

      // Load components (before interface in order to reduce issues with missing/unregistered data types)
      for (rrlib::xml::tNode::const_iterator node = root.ChildrenBegin(); node != root.ChildrenEnd(); ++node)
      {
        std::string name = node->Name();
        if (name == "element")
        {
          Instantiate(*node, GetFrameworkElement());
        }
      }

      // Load all remaining XML elements
      bool this_is_outermost_composite_component = GetFrameworkElement()->GetParentWithFlags(tFlag::FINSTRUCTABLE_GROUP) == nullptr;
      for (rrlib::xml::tNode::const_iterator node = root.ChildrenBegin(); node != root.ChildrenEnd(); ++node)
      {
        std::string name = node->Name();
        if (name == "interface")
        {
          tEditableInterfaces* editable_interfaces = GetFrameworkElement()->GetAnnotation<tEditableInterfaces>();
          if (editable_interfaces)
          {
            try
            {
              core::tPortGroup& loaded_interface = editable_interfaces->LoadInterfacePorts(*node);

              // Move RPC port to suitable interfaces when loading legacy files
              if (version == 0 && (!loaded_interface.GetFlag(tFlag::INTERFACE_FOR_RPC_PORTS)))
              {
                for (auto it = loaded_interface.ChildPortsBegin(); it != loaded_interface.ChildPortsEnd(); ++it)
                {
                  if (it->GetDataType().GetTypeClassification() == rrlib::rtti::tTypeClassification::RPC_TYPE)
                  {
                    core::tFrameworkElement* services_interface = nullptr;
                    for (auto child_interface = GetFrameworkElement()->ChildrenBegin(); child_interface != GetFrameworkElement()->ChildrenEnd(); ++child_interface)
                    {
                      if (child_interface->IsReady() && (!child_interface->IsPort()) && child_interface->GetFlag(tFlag::INTERFACE) && child_interface->GetFlag(tFlag::INTERFACE_FOR_RPC_PORTS))
                      {
                        if (services_interface)
                        {
                          services_interface = nullptr;
                          break;
                        }
                        services_interface = &(*child_interface);
                      }
                    }

                    if (services_interface)
                    {
                      FINROC_LOG_PRINT(WARNING, "Moving RPC port '", it->GetName(), "' to RPC interface '", *services_interface, "' (auto-update loading legacy files).");
                      auto cKEEP_FLAGS = (tFlag::ACCEPTS_DATA | tFlag::EMITS_DATA | tFlag::OUTPUT_PORT | tFlag::FINSTRUCTED_PORT).Raw();
                      tPortCreationList creation_list(static_cast<core::tPortGroup&>(*services_interface), core::tFrameworkElementFlags(it->GetAllFlags().Raw() & cKEEP_FLAGS), tPortCreateOptions());
                      creation_list.Add(it->GetName(), it->GetDataType());
                      it->ManagedDelete();
                    }
                  }
                }
              }
            }
            catch (const std::exception& e)
            {
              FINROC_LOG_PRINT(WARNING, "Loading interface ports failed. Reason: ", e);
            }
          }
          else
          {
            FINROC_LOG_PRINT(WARNING, "Cannot load interface, because finstructable group does not have any editable interfaces.");
          }
        }
        else if (name == "element")
        {
          // already instantiated
        }
        else if (name == "edge")
        {
          std::string source_string = node->GetStringAttribute("src");
          std::string destination_string = node->GetStringAttribute("dest");
          rrlib::uri::tURI source_uri(source_string);
          rrlib::uri::tURI destination_uri(destination_string);

          try
          {
            rrlib::uri::tURIElements source_uri_parsed;
            rrlib::uri::tURIElements destination_uri_parsed;
            if (version)
            {
              source_uri.Parse(source_uri_parsed);
              destination_uri.Parse(destination_uri_parsed);
            }
            else
            {
              source_uri_parsed.path = rrlib::uri::tPath(source_string);
              destination_uri_parsed.path = rrlib::uri::tPath(destination_string);
            }
            core::tUriConnectOptions connect_options(core::tConnectionFlag::FINSTRUCTED);
            if (node->HasAttribute("flags"))
            {
              connect_options.flags |= rrlib::serialization::Deserialize<core::tConnectionFlags>(node->GetStringAttribute("flags"));
            }

            for (rrlib::xml::tNode::const_iterator conversion_node = node->ChildrenBegin(); conversion_node != node->ChildrenEnd(); ++conversion_node)
            {
              if (conversion_node->Name() == "conversion")
              {
                rrlib::rtti::tType intermediate_type;
                if (conversion_node->HasAttribute("intermediate_type"))
                {
                  intermediate_type = rrlib::rtti::tType::FindType(conversion_node->GetStringAttribute("intermediate_type"));
                }
                if (conversion_node->HasAttribute("operation2"))
                {
                  // Two operations
                  std::string operation1 = conversion_node->GetStringAttribute("operation1");
                  std::string operation2 = conversion_node->GetStringAttribute("operation2");
                  connect_options.conversion_operations = rrlib::rtti::conversion::tConversionOperationSequence(operation1, operation2, intermediate_type);
                  if (conversion_node->HasAttribute("parameter1"))
                  {
                    connect_options.conversion_operations.SetParameterValue(0, conversion_node->GetStringAttribute("parameter1"));
                  }
                  if (conversion_node->HasAttribute("parameter2"))
                  {
                    connect_options.conversion_operations.SetParameterValue(1, conversion_node->GetStringAttribute("parameter2"));
                  }
                }
                else if (conversion_node->HasAttribute("operation"))
                {
                  // One operation
                  std::string operation1 = conversion_node->GetStringAttribute("operation");
                  connect_options.conversion_operations = rrlib::rtti::conversion::tConversionOperationSequence(operation1, "", intermediate_type);
                  if (conversion_node->HasAttribute("parameter"))
                  {
                    connect_options.conversion_operations.SetParameterValue(0, conversion_node->GetStringAttribute("parameter"));
                  }
                }
              }
            }

            if (source_uri_parsed.scheme.length() == 0 && destination_uri_parsed.scheme.length() == 0)
            {
              core::tAbstractPort* source_port = GetChildPort(source_uri_parsed.path);
              core::tAbstractPort* destination_port = GetChildPort(destination_uri_parsed.path);

              // Backward-compatibility: Check whether this a connector between service interfaces now
              if (version == 0 && source_port == nullptr && destination_port == nullptr)
              {
                core::tAbstractPort* service_source_port = GetChildPort(ReplaceInterfaceInPath(source_uri_parsed.path, "Services"));
                core::tAbstractPort* service_destination_port = GetChildPort(ReplaceInterfaceInPath(destination_uri_parsed.path, "Services"));
                if (service_source_port && service_destination_port && service_source_port->GetDataType().GetTypeClassification() == rrlib::rtti::tTypeClassification::RPC_TYPE)
                {
                  FINROC_LOG_PRINT(WARNING, "Adjusted connector's interfaces to service interfaces (auto-update loading legacy files): now connects '", *service_source_port, "' and '", *service_destination_port, "'");
                  source_port = service_source_port;
                  destination_port = service_destination_port;
                }
              }

              if (source_port == nullptr && destination_port == nullptr)
              {
                FINROC_LOG_PRINT(WARNING, "Cannot create connector because neither port is available: '", source_uri_parsed.path, "' and '", destination_uri_parsed.path, "'");
              }
              else if (source_port == nullptr || source_port->GetFlag(tFlag::VOLATILE))
              {
                if (source_uri_parsed.path.IsAbsolute() && this_is_outermost_composite_component && version == 0)
                {
                  FINROC_LOG_PRINT(WARNING, "Interpreting absolute connector source path (", source_uri_parsed.path, ") as legacy TCP connection");
                  destination_port->ConnectTo(rrlib::uri::tURI("tcp:" + rrlib::uri::tURI(source_uri_parsed.path).ToString()));
                }
                else
                {
                  destination_port->ConnectTo(source_uri_parsed.path.IsAbsolute() ? source_uri_parsed.path : path_to_this.Append(source_uri_parsed.path), connect_options);
                }
              }
              else if (destination_port == nullptr || destination_port->GetFlag(tFlag::VOLATILE))
              {
                if (destination_uri_parsed.path.IsAbsolute() && this_is_outermost_composite_component && version == 0)
                {
                  FINROC_LOG_PRINT(WARNING, "Interpreting absolute connector destination path (", destination_uri_parsed.path, ") as legacy TCP connection");
                  source_port->ConnectTo(rrlib::uri::tURI("tcp:" + rrlib::uri::tURI(destination_uri_parsed.path).ToString()));
                }
                else
                {
                  source_port->ConnectTo(destination_uri_parsed.path.IsAbsolute() ? destination_uri_parsed.path : path_to_this.Append(destination_uri_parsed.path), connect_options);
                }
              }
              else
              {
                source_port->ConnectTo(*destination_port, connect_options);
              }
            }
            else
            {
              // Create URI connector
              if (source_uri_parsed.scheme.length() && destination_uri_parsed.scheme.length())
              {
                throw std::runtime_error("Only one port may have an address with an URI scheme");
              }
              core::tAbstractPort* source_port = GetChildPort(source_uri_parsed.scheme.length() == 0 ? source_uri_parsed.path : destination_uri_parsed.path);
              if (!source_port)
              {
                FINROC_LOG_PRINT(WARNING, "Cannot create connector because port is not available: ", source_uri_parsed.scheme.length() == 0 ? source_uri_parsed.path : destination_uri_parsed.path);
              }
              const core::tURI& scheme_uri = source_uri_parsed.scheme.length() != 0 ? source_uri : destination_uri;

              // read parameters
              for (rrlib::xml::tNode::const_iterator parameter_node = node->ChildrenBegin(); parameter_node != node->ChildrenEnd(); ++parameter_node)
              {
                if (parameter_node->Name() == "parameter")
                {
                  connect_options.parameters.emplace(parameter_node->GetStringAttribute("name"), parameter_node->GetTextContent());
                }
              }

              core::tUriConnector::Create(*source_port, scheme_uri, connect_options);
            }
          }
          catch (const std::exception& e)
          {
            FINROC_LOG_PRINT(WARNING, "Creating connector from ", source_uri.ToString(), " to ", destination_uri.ToString(), " failed. Reason: ", e.what());
          }
        }
        else if (name == "parameter") // legacy parameter info support
        {
          core::tURI parameter_uri(node->GetStringAttribute("link"));
          rrlib::uri::tURIElements parameter_uri_parsed;
          parameter_uri.Parse(parameter_uri_parsed);
          core::tAbstractPort* parameter = GetChildPort(parameter_uri_parsed.path);
          if (!parameter)
          {
            FINROC_LOG_PRINT(WARNING, "Cannot set config entry, because parameter is not available: ", parameter_uri.ToString());
          }
          else
          {
            LoadParameter(*node, *parameter);
          }
        }
        else if (name == "parameter_links")
        {
          ProcessParameterLinksNode(*node, *GetFrameworkElement());
        }
        else
        {
          FINROC_LOG_PRINT(WARNING, "Unknown XML tag: ", name);
        }
      }
      FINROC_LOG_PRINT(DEBUG, "Loading XML successful");
    }
    catch (const std::exception& e)
    {
      FINROC_LOG_PRINT(WARNING, "Loading XML '", GetXmlFileString(), "' failed: ", e);
    }
  }
}

void tFinstructable::OnInitialization()
{
  if (!GetFrameworkElement()->GetFlag(tFlag::FINSTRUCTABLE_GROUP))
  {
    throw std::logic_error("Any class using tFinstructable must set tFlag::FINSTRUCTABLE_GROUP in constructor");
  }
}

void tFinstructable::ProcessParameterLinksNode(const rrlib::xml::tNode& node, core::tFrameworkElement& element)
{
  for (auto it = node.ChildrenBegin(); it != node.ChildrenEnd(); ++it)
  {
    if (it->Name() == "element")
    {
      std::string name = it->GetStringAttribute("name");
      core::tFrameworkElement* corresponding_element = element.GetChild(name);
      if (corresponding_element)
      {
        ProcessParameterLinksNode(*it, *corresponding_element);
      }
      else
      {
        FINROC_LOG_PRINT(WARNING, "Cannot find '", element, "/", name, "'. Parameter entries below are not loaded.");
      }
    }
    else if (it->Name() == "parameter")
    {
      std::string name = it->GetStringAttribute("name");
      core::tFrameworkElement* parameter_element = element.GetChild(name);
      if (!parameter_element)
      {
        core::tFrameworkElement* parameters_interface = element.GetChild("Parameters");
        parameter_element = parameters_interface ? parameters_interface->GetChild(name) : nullptr;
      }
      if (parameter_element && parameter_element->IsPort())
      {
        LoadParameter(*it, static_cast<core::tAbstractPort&>(*parameter_element));
      }
      else
      {
        FINROC_LOG_PRINT(WARNING, "Cannot find parameter '", element, "/", name, "'. Parameter entry is not loaded.");
      }
    }
  }
}

bool tFinstructable::SaveParameterConfigEntries(rrlib::xml::tNode& node, core::tFrameworkElement& element)
{
  // Get alphabetically sorted list of children
  std::vector<core::tFrameworkElement*> child_elements;
  std::vector<core::tAbstractPort*> parameter_ports;
  for (auto it = element.ChildrenBegin(); it != element.ChildrenEnd(); ++it)
  {
    if (it->IsReady())
    {
      if (it->GetFlag(tFlag::INTERFACE) && it->GetName() == "Parameters")
      {
        for (auto parameter_it = it->ChildrenBegin(); parameter_it != it->ChildrenEnd(); ++parameter_it)
        {
          if (parameter_it->IsReady() && parameter_it->IsPort() && parameter_it->GetAnnotation<parameters::internal::tParameterInfo>())
          {
            parameter_ports.push_back(&static_cast<core::tAbstractPort&>(*parameter_it));
          }
        }
      }
      else if (it->IsPort() && it->GetAnnotation<parameters::internal::tParameterInfo>())
      {
        parameter_ports.push_back(&static_cast<core::tAbstractPort&>(*it));
      }
      else
      {
        child_elements.push_back(&(*it));
      }
    }
  }

  struct
  {
    bool operator()(core::tFrameworkElement* a, core::tFrameworkElement* b)
    {
      return a->GetName() < b->GetName();
    }
  } comparator;
  std::sort(child_elements.begin(), child_elements.end(), comparator);
  std::sort(parameter_ports.begin(), parameter_ports.end(), comparator);

  // Save parameters first
  bool result = false;
  for (core::tAbstractPort * port : parameter_ports)
  {
    bool outermost_group = GetFrameworkElement()->GetParent() == &(core::tRuntimeEnvironment::GetInstance());
    parameters::internal::tParameterInfo* info = port->GetAnnotation<parameters::internal::tParameterInfo>();
    bool is_responsible_for_parameter_links = IsResponsibleForConfigFileConnections(*port);

    if (info && info->HasNonDefaultFinstructInfo() && (is_responsible_for_parameter_links || (outermost_group && info->GetCommandLineOption().length())))
    {
      // Save Parameter
      rrlib::xml::tNode& parameter_node = node.AddChildNode("parameter");
      parameter_node.SetAttribute("name", port->GetName());

      if (!is_responsible_for_parameter_links)
      {
        parameter_node.SetAttribute("cmdline", info->GetCommandLineOption());
      }
      else
      {
        info->Serialize(parameter_node, true, outermost_group);
      }
      result = true;
    }
  }

  // Process child elements
  for (core::tFrameworkElement * child : child_elements)
  {
    rrlib::xml::tNode& child_node = node.AddChildNode("element");
    child_node.SetAttribute("name", child->GetName());
    bool sub_result = SaveParameterConfigEntries(child_node, *child);
    result |= sub_result;
    if (!sub_result)
    {
      node.RemoveChildNode(child_node);
    }
  }
  return result;

  /*for (auto it = GetFrameworkElement()->SubElementsBegin(); it != GetFrameworkElement()->SubElementsEnd(); ++it)
  {
    if ((!it->IsPort()) || (!it->IsReady()))
    {
      continue;
    }

    core::tAbstractPort& port = static_cast<core::tAbstractPort&>(*it);
    bool outermost_group = GetFrameworkElement()->GetParent() == &(core::tRuntimeEnvironment::GetInstance());
    parameters::internal::tParameterInfo* info = port.GetAnnotation<parameters::internal::tParameterInfo>();

    if (info && info->HasNonDefaultFinstructInfo() && (IsResponsibleForConfigFileConnections(port) || (outermost_group && info->GetCommandLineOption().length())))
    {
      bool parameter_interface = port.GetParent()->GetFlag(tFlag::INTERFACE) || port.GetParent()->GetName() == "Parameters"; // TODO: remove the latter as soon as
      core::tFrameworkElement* target_hierarchy_element = (parameter_interface && port.GetParent()->GetParent()) ? port.GetParent()->GetParent() : port.GetParent();

      // Possibly move up hierarchy in XML
      while (!(target_hierarchy_element == current_hierarchy_element || target_hierarchy_element->IsChildOf(*current_hierarchy_element)))
      {
        current_hierarchy_element = current_hierarchy_element->GetParent();
        current_parameter_links_node = &current_parameter_links_node->Parent();
      }
      // Possibly create hierarchy in XML
      std::vector<core::tFrameworkElement*> elements_to_add;
      core::tFrameworkElement* element_to_add = target_hierarchy_element;
      while (element_to_add != current_hierarchy_element)
      {
        elements_to_add.push_back(element_to_add);
        element_to_add = element_to_add->GetParent();
      }
      for (auto it = elements_to_add.rbegin(); it != elements_to_add.rend(); ++it)
      {
        current_parameter_links_node = &current_parameter_links_node->AddChildNode("element");
        current_parameter_links_node->SetAttribute("name", (*it)->GetName());
      }
      current_hierarchy_element = target_hierarchy_element;

      // Save Parameter
      rrlib::xml::tNode& parameter_node = current_parameter_links_node->AddChildNode("parameter");
      parameter_node.SetAttribute("name", port.GetName());

      if (!IsResponsibleForConfigFileConnections(port))
      {
        parameter_node.SetAttribute("cmdline", info->GetCommandLineOption());
      }
      else
      {
        info->Serialize(parameter_node, true, outermost_group);
      }
    }
  }
  */
}

void tFinstructable::SaveXml()
{
  {
    rrlib::thread::tLock lock2(core::tRuntimeEnvironment::GetInstance().GetStructureMutex());
#ifndef RRLIB_SINGLE_THREADED
    saving_thread = &rrlib::thread::tThread::CurrentThread();
#endif
    dependencies_tmp.clear();
    std::string save_to = core::GetFinrocFileToSaveTo(GetXmlFileString());
    if (save_to.length() == 0)
    {
      std::string save_to_alt = GetXmlFileString();
      std::replace(save_to_alt.begin(), save_to_alt.end(), '/', '_');
      FINROC_LOG_PRINT(USER, "There does not seem to be any suitable location for: '", GetXmlFileString(), "' . For now, using '", save_to_alt, "'.");
      save_to = save_to_alt;
    }
    FINROC_LOG_PRINT(USER, "Saving XML: ", save_to);
    rrlib::xml::tDocument doc;
    try
    {
      rrlib::xml::tNode& root = doc.AddRootNode("Finstructable"); // TODO: find good name ("finroc_structure")

      // serialize default main name
      if (main_name.length() > 0)
      {
        root.SetAttribute("defaultname", main_name);
      }
      root.SetAttribute("version", cVERSION);

      // serialize any editable interfaces
      tEditableInterfaces* editable_interfaces = GetFrameworkElement()->GetAnnotation<tEditableInterfaces>();
      if (editable_interfaces)
      {
        editable_interfaces->SaveAllNonEmptyInterfaces(root);
      }

      // serialize framework elements
      SerializeChildren(root, *GetFrameworkElement());

      // serialize connectors (sorted by port URIs (we do not want changes in finstruct files depending on whether or not an optional/volatile connector exists))
      rrlib::uri::tPath this_path = GetFrameworkElement()->GetPath();
      std::map<std::pair<rrlib::uri::tURI, rrlib::uri::tURI>, std::pair<core::tConnector*, core::tUriConnector*>> connector_map;
      bool this_is_outermost_composite_component = GetFrameworkElement()->GetParentWithFlags(tFlag::FINSTRUCTABLE_GROUP) == nullptr;

      // Get (primary) connectors
      for (auto it = GetFrameworkElement()->SubElementsBegin(); it != GetFrameworkElement()->SubElementsEnd(); ++it)
      {
        if ((!it->IsPort()) || (!it->IsReady()))
        {
          continue;
        }
        core::tAbstractPort& port = static_cast<core::tAbstractPort&>(*it);
        tFrameworkElement* port_parent_group = port.GetParentWithFlags(tFlag::FINSTRUCTABLE_GROUP);

        // Plain connectors
        for (auto it = port.OutgoingConnectionsBegin(); it != port.OutgoingConnectionsEnd(); ++it) // only outgoing edges => we don't get any edges twice
        {
          // Checks whether to save connector
          // only save primary finstructed edges
          if ((!it->Flags().Get(core::tConnectionFlag::FINSTRUCTED)) || it->Flags().Get(core::tConnectionFlag::NON_PRIMARY_CONNECTOR))
          {
            continue;
          }

          // connectors should be saved in innermost composite component that contains both ports (common parent); if there is no such port, then save in outermost composite component
          tFrameworkElement* common_parent = port_parent_group;
          while (common_parent && (!it->Destination().IsChildOf(*common_parent)))
          {
            common_parent = common_parent->GetParentWithFlags(tFlag::FINSTRUCTABLE_GROUP);
          }
          if (common_parent != GetFrameworkElement() && (!(this_is_outermost_composite_component && common_parent == nullptr)))
          {
            // save connector in another group
            continue;
          }

          std::pair<rrlib::uri::tURI, rrlib::uri::tURI> key(rrlib::uri::tURI(GetConnectorPath(port.GetPath(), this_path), cUNENCODED_RESERVED_CHARACTERS_PATH), rrlib::uri::tURI(GetConnectorPath(it->Destination().GetPath(), this_path), cUNENCODED_RESERVED_CHARACTERS_PATH));
          std::pair<core::tConnector*, core::tUriConnector*> value(&(*it), nullptr);
          connector_map.emplace(key, value);
        }

        // Get URI connectors
        for (auto & connector : port.UriConnectors())
        {
          // Checks whether to save connector
          // only save primary finstructed edges
          if ((!connector) || (!connector->Flags().Get(core::tConnectionFlag::FINSTRUCTED)) || connector->Flags().Get(core::tConnectionFlag::NON_PRIMARY_CONNECTOR))
          {
            continue;
          }

          std::pair<rrlib::uri::tURI, rrlib::uri::tURI> key(rrlib::uri::tURI(GetConnectorPath(port.GetPath(), this_path), cUNENCODED_RESERVED_CHARACTERS_PATH), connector->Uri());

          // local URI connectors should be saved in innermost composite component that contains both ports (common parent); if there is no such port, then save in outermost composite component
          if (typeid(*connector) == typeid(core::internal::tLocalUriConnector))
          {
            auto& local_connector = static_cast<core::internal::tLocalUriConnector&>(*connector);
            bool source_uri = local_connector.GetPortReferences()[0].path.Size();
            rrlib::uri::tPath path = local_connector.GetPortReferences()[source_uri ? 0 : 1].path;
            tFrameworkElement* common_parent = port_parent_group;
            rrlib::uri::tPath parent_group_path = common_parent->GetPath();
            while (common_parent && path.CountCommonElements(parent_group_path) != parent_group_path.Size())
            {
              common_parent = common_parent->GetParentWithFlags(tFlag::FINSTRUCTABLE_GROUP);
              parent_group_path = common_parent ? common_parent->GetPath() : parent_group_path;
            }
            if (common_parent != GetFrameworkElement() && (!(this_is_outermost_composite_component && common_parent == nullptr)))
            {
              // save connector in another group
              continue;
            }

            rrlib::uri::tURI port_uri = key.first;
            key.first = source_uri ? rrlib::uri::tURI(path, cUNENCODED_RESERVED_CHARACTERS_PATH) : port_uri;
            key.second = source_uri ? port_uri : rrlib::uri::tURI(path, cUNENCODED_RESERVED_CHARACTERS_PATH);
          }

          std::pair<core::tConnector*, core::tUriConnector*> value(nullptr, connector.get());
          connector_map.emplace(key, value);
        }
      }

      for (auto & entry : connector_map)
      {
        rrlib::xml::tNode& edge = root.AddChildNode("edge"); // TODO: "connector"?
        edge.SetAttribute("src", entry.first.first.ToString());
        edge.SetAttribute("dest", entry.first.second.ToString());

        // Save flags
        core::tConnectionFlags cFLAGS_TO_SAVE = core::tConnectionFlag::DIRECTION_TO_DESTINATION | core::tConnectionFlag::DIRECTION_TO_SOURCE | core::tConnectionFlag::OPTIONAL | core::tConnectionFlag::RECONNECT | core::tConnectionFlag::SCHEDULING_NEUTRAL;
        core::tConnectionFlags flags_to_save((entry.second.first ? entry.second.first->Flags() : entry.second.second->Flags()).Raw() & cFLAGS_TO_SAVE.Raw());
        if (flags_to_save.Raw())
        {
          edge.SetAttribute("flags", rrlib::serialization::Serialize(flags_to_save));
        }

        // Save conversion operation
        const rrlib::rtti::conversion::tConversionOperationSequence& conversion_operations = entry.second.first ? entry.second.first->ConversionOperations() : entry.second.second->ConversionOperations();
        if (conversion_operations.Size())
        {
          rrlib::xml::tNode& conversion = edge.AddChildNode("conversion");
          conversion.SetAttribute(conversion_operations.Size() == 2 ? "operation1" : "operation", conversion_operations[0].first);
          if (conversion_operations.GetParameterValue(0))
          {
            conversion.SetAttribute(conversion_operations.Size() == 2 ? "parameter1" : "parameter", conversion_operations.GetParameterValue(0).ToString());
          }
          if (conversion_operations.IntermediateType())
          {
            conversion.SetAttribute("intermediate_type", conversion_operations.IntermediateType().GetName());
          }
          if (conversion_operations.Size() == 2)
          {
            conversion.SetAttribute("operation2", conversion_operations[1].first);
            if (conversion_operations.GetParameterValue(1))
            {
              conversion.SetAttribute("parameter2", conversion_operations.GetParameterValue(1).ToString());
            }
          }
        }

        // Save parameters of URI connector
        if (entry.second.second)
        {
          core::tUriConnector::tConstParameterDefinitionRange definition_range = entry.second.second->GetParameterDefinitions();
          core::tUriConnector::tParameterValueRange value_range = entry.second.second->GetParameterValues();
          assert(definition_range.End() - definition_range.Begin() == value_range.End() - value_range.Begin());

          auto definition = definition_range.Begin();
          auto value = value_range.Begin();
          for (; definition < definition_range.End(); ++definition, ++value)
          {
            if (!value->Equals(definition->GetDefaultValue()))
            {
              rrlib::xml::tNode& parameter_node = root.AddChildNode("parameter");
              parameter_node.SetAttribute("name", definition->GetName());
              value->Serialize(parameter_node);
            }
          }
        }
      }

      // Save parameter config entries
      auto& parameter_node = root.AddChildNode("parameter_links");
      if (!SaveParameterConfigEntries(parameter_node, *GetFrameworkElement()))
      {
        root.RemoveChildNode(parameter_node);
      }

      // add dependencies
      if (dependencies_tmp.size() > 0)
      {
        std::stringstream s;
        for (auto it = dependencies_tmp.begin(); it != dependencies_tmp.end(); ++it)
        {
          if (it != dependencies_tmp.begin())
          {
            s << ", ";
          }
          s << it->ToString();
        }

        root.SetAttribute("dependencies", s.str());
        dependencies_tmp.clear();
      }

      doc.WriteToFile(save_to);
      FINROC_LOG_PRINT(USER, "Saving successful.");

    }
    catch (const rrlib::xml::tException& e)
    {
      const char* msg = e.what();
      FINROC_LOG_PRINT(USER, "Saving failed: ", msg);
      throw std::runtime_error(msg);
    }
  }
  saving_thread = nullptr;
}

std::vector<std::string> tFinstructable::ScanForCommandLineArgs(const std::string& finroc_file)
{
  std::vector<std::string> result;
  try
  {
    rrlib::xml::tDocument doc(core::GetFinrocXMLDocument(finroc_file, false));
    try
    {
      FINROC_LOG_PRINT_STATIC(DEBUG, "Scanning for command line options in ", finroc_file);
      rrlib::xml::tNode& root = doc.RootNode();
      ScanForCommandLineArgsHelper(result, root);
      FINROC_LOG_PRINTF_STATIC(DEBUG, "Scanning successful. Found %zu additional options.", result.size());
    }
    catch (std::exception& e)
    {
      FINROC_LOG_PRINT_STATIC(WARNING, "FinstructableGroup", "Scanning failed: ", finroc_file, e);
    }
  }
  catch (std::exception& e)
  {}
  return result;
}

void tFinstructable::ScanForCommandLineArgsHelper(std::vector<std::string>& result, const rrlib::xml::tNode& parent)
{
  for (rrlib::xml::tNode::const_iterator node = parent.ChildrenBegin(); node != parent.ChildrenEnd(); ++node)
  {
    std::string name(node->Name());
    if (node->HasAttribute("cmdline") && (name == "staticparameter" || name == "parameter"))
    {
      result.push_back(node->GetStringAttribute("cmdline"));
    }
    ScanForCommandLineArgsHelper(result, *node);
  }
}

void tFinstructable::SerializeChildren(rrlib::xml::tNode& node, tFrameworkElement& current)
{
  for (auto child = current.ChildrenBegin(); child != current.ChildrenEnd(); ++child)
  {
    parameters::internal::tStaticParameterList* spl = child->GetAnnotation<parameters::internal::tStaticParameterList>();
    tConstructorParameters* cps = child->GetAnnotation<tConstructorParameters>();
    if (child->IsReady() && child->GetFlag(tFlag::FINSTRUCTED))
    {
      // serialize framework element
      rrlib::xml::tNode& n = node.AddChildNode("element");
      n.SetAttribute("name", child->GetName());
      tCreateFrameworkElementAction* cma = tCreateFrameworkElementAction::GetConstructibleElements()[spl->GetCreateAction()];
      n.SetAttribute("group", cma->GetModuleGroup().ToString());
      //if (boost::ends_with(cma->GetModuleGroup(), ".so"))
      //{
      AddDependency(cma->GetModuleGroup());
      //}
      n.SetAttribute("type", cma->GetName());
      if (cps != nullptr)
      {
        rrlib::xml::tNode& pn = n.AddChildNode("constructor");
        cps->Serialize(pn, true);
      }
      if (spl != nullptr)
      {
        rrlib::xml::tNode& pn = n.AddChildNode("parameters");
        spl->Serialize(pn, true);
      }

      // serialize its children
      if (!child->GetFlag(tFlag::FINSTRUCTABLE_GROUP))
      {
        SerializeChildren(n, *child);
      }
    }
  }
}

void tFinstructable::SetFinstructed(tFrameworkElement& fe, tCreateFrameworkElementAction& create_action, tConstructorParameters* params)
{
  assert(!fe.GetFlag(tFlag::FINSTRUCTED) && (!fe.IsReady()));
  parameters::internal::tStaticParameterList& list = parameters::internal::tStaticParameterList::GetOrCreate(fe);
  auto& element_list = tCreateFrameworkElementAction::GetConstructibleElements();
  for (size_t i = 0, n = element_list.Size(); i < n; ++i)
  {
    if (element_list[i] == &create_action)
    {
      list.SetCreateAction(i);
      break;
    }
  }
  fe.SetFlag(tFlag::FINSTRUCTED);
  if (params)
  {
    fe.AddAnnotation<tConstructorParameters>(*params);
  }
}

void tFinstructable::StaticInit()
{
  startup_loaded_finroc_libs = GetLoadedFinrocLibraries();
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
