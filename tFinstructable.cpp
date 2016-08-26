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
#include "core/internal/tByStringConnector.h"
#include "plugins/parameters/internal/tParameterInfo.h"

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
static rrlib::thread::tThread* saving_thread = NULL;

/*! Temporary variable for saving: .so files that should be loaded prior to instantiating this group */
static std::set<tSharedLibrary> dependencies_tmp;

/*! Loaded finroc libraries at startup */
static std::set<tSharedLibrary> startup_loaded_finroc_libs;

/*! We do not want to have this prefix in XML file names, as this will not be found when a system installation is used */
static const char* cUNWANTED_XML_FILE_PREFIX = "sources/cpp/";

tFinstructable::tFinstructable(const std::string& xml_file) :
  main_name(),
  xml_file(xml_file)
{
}

void tFinstructable::AddDependency(const tSharedLibrary& dependency)
{
  if (&rrlib::thread::tThread::CurrentThread() == saving_thread && startup_loaded_finroc_libs.find(dependency) == startup_loaded_finroc_libs.end())
  {
    dependencies_tmp.insert(dependency);
  }
}

void tFinstructable::AddDependency(const rrlib::rtti::tType& dt)
{
  tSharedLibrary shared_library = GetDataTypeDependency(dt);
  if (shared_library.IsValid())
  {
    AddDependency(shared_library);
  }
}

void tFinstructable::AnnotatedObjectInitialized()
{
  if (!GetFrameworkElement()->GetFlag(tFlag::FINSTRUCTABLE_GROUP))
  {
    throw std::logic_error("Any class using tFinstructable must set tFlag::FINSTRUCTABLE_GROUP in constructor");
  }
}

core::tAbstractPort* tFinstructable::GetChildPort(const std::string& link)
{
  if (link[0] == '/')
  {
    return core::tRuntimeEnvironment::GetInstance().GetPort(link);
  }
  tFrameworkElement* fe = GetFrameworkElement()->GetChildElement(link, false);
  if (fe != NULL && fe->IsPort())
  {
    return static_cast<core::tAbstractPort*>(fe);
  }
  return NULL;
}

std::string tFinstructable::GetEdgeLink(const std::string& target_link, const std::string& this_group_link)
{
  if (target_link.compare(0, this_group_link.length(), this_group_link) == 0)
  {
    return target_link.substr(this_group_link.length());
  }
  return target_link;
}

std::string tFinstructable::GetEdgeLink(core::tAbstractPort& ap, const std::string& this_group_link)
{
  tFrameworkElement* alt_root = ap.GetParentWithFlags(tFlag::ALTERNATIVE_LINK_ROOT);
  if (alt_root && alt_root->IsChildOf(*GetFrameworkElement()))
  {
    return ap.GetQualifiedLink();
  }
  return ap.GetQualifiedName().substr(this_group_link.length());
}

std::string tFinstructable::GetLogDescription() const
{
  return GetFrameworkElement() ? GetFrameworkElement()->GetQualifiedName() : "Unattached Finstructable";
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
    const rrlib::xml::tNode* parameters = NULL;
    const rrlib::xml::tNode* constructor_params = NULL;
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
    tFrameworkElement* created = NULL;
    tConstructorParameters* spl = NULL;
    if (constructor_params != NULL)
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
    FINROC_LOG_PRINT(WARNING, "Port is not a parameter: '", parameter_port.GetQualifiedName(), "'. Parameter entry is not loaded.");
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
      FINROC_LOG_PRINT(WARNING, "Unable to load parameter value for '", parameter_port.GetQualifiedName(), "'. ", e);
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
      std::string link_tmp = GetFrameworkElement()->GetQualifiedName() + "/";
      if (main_name.length() == 0 && root.HasAttribute("defaultname"))
      {
        main_name = root.GetStringAttribute("defaultname");
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
              editable_interfaces->LoadInterfacePorts(*node);
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
          std::string src = node->GetStringAttribute("src");
          std::string dest = node->GetStringAttribute("dest");
          core::tAbstractPort* src_port = GetChildPort(src);
          core::tAbstractPort* dest_port = GetChildPort(dest);
          if (src_port == NULL && dest_port == NULL)
          {
            FINROC_LOG_PRINT(WARNING, "Cannot create edge because neither port is available: ", src, ", ", dest);
          }
          else if (src_port == NULL || src_port->GetFlag(tFlag::VOLATILE))    // source volatile
          {
            dest_port->ConnectTo(QualifyLink(src, link_tmp), core::tConnectionFlag::FINSTRUCTED | core::tConnectionFlag::RECONNECT);
          }
          else if (dest_port == NULL || dest_port->GetFlag(tFlag::VOLATILE))    // destination volatile
          {
            src_port->ConnectTo(QualifyLink(dest, link_tmp), core::tConnectionFlag::FINSTRUCTED | core::tConnectionFlag::RECONNECT);
          }
          else
          {
            src_port->ConnectTo(*dest_port, core::tConnectionFlag::FINSTRUCTED);
          }
        }
        else if (name == "parameter") // legacy parameter info support
        {
          std::string param = node->GetStringAttribute("link");
          core::tAbstractPort* parameter = GetChildPort(param);
          if (!parameter)
          {
            FINROC_LOG_PRINT(WARNING, "Cannot set config entry, because parameter is not available: ", param);
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
        FINROC_LOG_PRINT(WARNING, "Cannot find '", element.GetQualifiedLink(), "/", name, "'. Parameter entries below are not loaded.");
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
        FINROC_LOG_PRINT(WARNING, "Cannot find parameter '", element.GetQualifiedLink(), "/", name, "'. Parameter entry is not loaded.");
      }
    }
  }
}

std::string tFinstructable::QualifyLink(const std::string& link, const std::string& this_group_link)
{
  if (link[0] == '/')
  {
    return link;
  }
  return this_group_link + link;
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
    saving_thread = &rrlib::thread::tThread::CurrentThread();
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

      // serialize any editable interfaces
      tEditableInterfaces* editable_interfaces = GetFrameworkElement()->GetAnnotation<tEditableInterfaces>();
      if (editable_interfaces)
      {
        editable_interfaces->SaveAllNonEmptyInterfaces(root);
      }

      // serialize framework elements
      SerializeChildren(root, *GetFrameworkElement());

      // serialize edges (sorted by port links (we do not want changes in finstruct files depending on whether or not an optional/volatile connector exists))
      std::string this_qualified_link = GetFrameworkElement()->GetQualifiedName() + "/";
      std::map<std::pair<std::string, std::string>, std::pair<core::tConnector*, core::internal::tByStringConnector*>> connector_map;
      std::set<core::tConnector*> skip_connectors;
      bool this_is_outermost_composite_component = GetFrameworkElement()->GetParentWithFlags(tFlag::FINSTRUCTABLE_GROUP) == nullptr;

      // First pass: Get ByStringConnectors and fill skip_connectors list
      for (auto it = GetFrameworkElement()->SubElementsBegin(); it != GetFrameworkElement()->SubElementsEnd(); ++it)
      {
        if ((!it->IsPort()) || (!it->IsReady()))
        {
          continue;
        }
        core::tAbstractPort& port = static_cast<core::tAbstractPort&>(*it);

        // Get ByStringConnectors first
        if (port.by_string_connectors)
        {
          tFrameworkElement* port_parent_group = port.GetParentWithFlags(tFlag::FINSTRUCTABLE_GROUP);
          for (auto & connector : (*port.by_string_connectors))
          {
            if (!connector->Flags().Get(core::tConnectionFlag::FINSTRUCTED))
            {
              continue;
            }
            if (connector->GetConnection())
            {
              skip_connectors.insert(connector->GetConnection());
            }

            // connectors should be saved in innermost composite component that contains both ports (common parent); if there is no such port, then save in outermost composite component
            bool source_link = connector->GetPortReferences()[0].link.length();
            std::string link = source_link ? connector->GetPortReferences()[0].link : connector->GetPortReferences()[1].link;
            tFrameworkElement* common_parent = port_parent_group;
            while (common_parent && (!rrlib::util::StartsWith(link, common_parent->GetQualifiedName())))
            {
              common_parent = common_parent->GetParentWithFlags(tFlag::FINSTRUCTABLE_GROUP);
            }
            if (common_parent != GetFrameworkElement() && (!(this_is_outermost_composite_component && common_parent == nullptr)))
            {
              // save connector in another group
              continue;
            }

            std::string this_port_link = GetEdgeLink(port, this_qualified_link);
            std::pair<std::string, std::string> key(source_link ? link : this_port_link, source_link ? this_port_link : link);
            std::pair<core::tConnector*, core::internal::tByStringConnector*> value(connector->GetConnection(), connector.get());
            connector_map.emplace(key, value);
          }
        }
      }

      // Second pass: Plain connectors
      for (auto it = GetFrameworkElement()->SubElementsBegin(); it != GetFrameworkElement()->SubElementsEnd(); ++it)
      {
        if ((!it->IsPort()) || (!it->IsReady()))
        {
          continue;
        }
        core::tAbstractPort& port = static_cast<core::tAbstractPort&>(*it);
        tFrameworkElement* port_parent_group = port.GetParentWithFlags(tFlag::FINSTRUCTABLE_GROUP);

        for (auto it = port.OutgoingConnectionsBegin(); it != port.OutgoingConnectionsEnd(); ++it) // only outgoing edges => we don't get any edges twice
        {
          // possibly save connector?

          // only save finstructed edges
          if (!it->Flags().Get(core::tConnectionFlag::FINSTRUCTED))
          {
            continue;
          }
          // only save edges that are not attached to a tByStringConnector => we don't get any edges twice
          if (skip_connectors.find(&(*it)) != skip_connectors.end())
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

          std::pair<std::string, std::string> key(GetEdgeLink(port, this_qualified_link), GetEdgeLink(it->Destination(), this_qualified_link));
          std::pair<core::tConnector*, core::internal::tByStringConnector*> value(&(*it), nullptr);
          connector_map.emplace(key, value);
        }
      }

      for (auto & entry : connector_map)
      {
        rrlib::xml::tNode& edge = root.AddChildNode("edge"); // TODO: "connector"?
        edge.SetAttribute("src", entry.first.first);
        edge.SetAttribute("dest", entry.first.second);
        // TODO: save more info  (flags optional & reconnect)
      }

      // Save parameter config entries
      SaveParameterConfigEntries(root.AddChildNode("parameter_links"), *GetFrameworkElement());

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
  saving_thread = NULL;
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
      if (cps != NULL)
      {
        rrlib::xml::tNode& pn = n.AddChildNode("constructor");
        cps->Serialize(pn, true);
      }
      if (spl != NULL)
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
  const std::vector<tCreateFrameworkElementAction*>& v = tCreateFrameworkElementAction::GetConstructibleElements();
  list.SetCreateAction(static_cast<int>(std::find(v.begin(), v.end(), &create_action) - v.begin()));
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
