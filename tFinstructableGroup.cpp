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
/*!\file    plugins/runtime_construction/tFinstructableGroup.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-02
 *
 */
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tFinstructableGroup.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <set>
#include "core/file_lookup.h"
#include "core/tFrameworkElementTags.h"
#include "core/tRuntimeEnvironment.h"
#include "core/internal/tLinkEdge.h"
#include "plugins/parameters/internal/tParameterInfo.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
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

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------

tStandardCreateModuleAction<tFinstructableGroup> tFinstructableGroup::cCREATE_ACTION("Finstructable Group");

/*! Thread currently saving finstructable group */
static rrlib::thread::tThread* saving_thread = NULL;

/*! Temporary variable for saving: .so files that should be loaded prior to instantiating this group */
static std::set<std::string> dependencies_tmp;

/*! Number of types at startup */
static int startup_type_count = 0;

/*! Loaded finroc libraries at startup */
static std::set<std::string> startup_loaded_finroc_libs;

tFinstructableGroup::tFinstructableGroup(tFrameworkElement* parent, const std::string& name, tFlags flags) :
  tFrameworkElement(parent, name, flags | tFlag::FINSTRUCTABLE_GROUP, -1),
  xml_file("XML file", this, ""),
  connect_tmp(),
  link_tmp(""),
  save_parameter_config_entries(false),
  main_name()
{
  core::tFrameworkElementTags::AddTag(*this, "group");
}

tFinstructableGroup::tFinstructableGroup(tFrameworkElement* parent, const std::string& name, const std::string& xml_file_, tFlags flags) :
  tFrameworkElement(parent, name, flags | tFlag::FINSTRUCTABLE_GROUP, -1),
  xml_file("XML file", this, ""),
  connect_tmp(),
  link_tmp(""),
  save_parameter_config_entries(false),
  main_name()
{
  core::tFrameworkElementTags::AddTag(*this, "group");
  try
  {
    this->xml_file.Set(xml_file_);
  }
  catch (const std::exception& e)
  {
    FINROC_LOG_PRINT(ERROR, e);
  }
}

void tFinstructableGroup::AddDependency(const std::string& dependency)
{
  if (&rrlib::thread::tThread::CurrentThread() == saving_thread && startup_loaded_finroc_libs.find(dependency.c_str()) == startup_loaded_finroc_libs.end())
  {
    dependencies_tmp.insert(dependency);
  }
}

void tFinstructableGroup::AddDependency(const rrlib::rtti::tType& dt)
{
  if (dt.GetUid() >= startup_type_count)
  {
    std::string tmp(dt.GetBinary());
    if (tmp.length() > 0)
    {
      if (tmp.find('/') != std::string::npos)
      {
        tmp = tmp.substr(tmp.rfind('/') + 1);
      }
      AddDependency(tmp);
    }
  }
}

void tFinstructableGroup::OnStaticParameterChange()
{
  if (xml_file.HasChanged() && xml_file.Get().length() > 0)
  {
    //if (this.childCount() == 0) { // TODO: original intension: changing xml files to mutliple existing ones in finstruct shouldn't load all of them
    if (core::FinrocFileExists(xml_file.Get()))
    {
      LoadXml(xml_file.Get());
    }
    else
    {
      FINROC_LOG_PRINT(DEBUG, "Cannot find XML file ", xml_file.Get(), ". Creating empty group. You may edit and save this group using finstruct.");
    }
  }
}

core::tAbstractPort* tFinstructableGroup::GetChildPort(const std::string& link)
{
  if (link[0] == '/')
  {
    return GetRuntime().GetPort(link);
  }
  ::finroc::core::tFrameworkElement* fe = GetChildElement(link, false);
  if (fe != NULL && fe->IsPort())
  {
    return static_cast<core::tAbstractPort*>(fe);
  }
  return NULL;
}

std::string tFinstructableGroup::GetEdgeLink(const std::string& target_link)
{
  if (boost::starts_with(target_link, link_tmp))
  {
    return target_link.substr(link_tmp.length());
  }
  return target_link;
}

std::string tFinstructableGroup::GetEdgeLink(core::tAbstractPort& ap)
{
  core::tFrameworkElement* alt_root = ap.GetParentWithFlags(tFlag::ALTERNATIVE_LINK_ROOT);
  if (alt_root && alt_root->IsChildOf(*this))
  {
    return ap.GetQualifiedLink();
  }
  return ap.GetQualifiedName().substr(link_tmp.length());
}

void tFinstructableGroup::Instantiate(const rrlib::xml::tNode& node, tFrameworkElement* parent)
{
  try
  {
    std::string name = node.GetStringAttribute("name");
    std::string group = node.GetStringAttribute("group");
    std::string type = node.GetStringAttribute("type");

    // find action
    tCreateFrameworkElementAction* action = internal::LoadModuleType(group, type);
    if (action == NULL)
    {
      FINROC_LOG_PRINT(WARNING, "Failed to instantiate element. No module type ", group, "/", type, " available. Skipping...");
      return;
    }

    // read parameters
    rrlib::xml::tNode::const_iterator child_node = node.ChildrenBegin();
    const rrlib::xml::tNode* parameters = NULL;
    const rrlib::xml::tNode* constructor_params = NULL;
    std::string p_name = child_node->Name();
    if (boost::equals(p_name, "constructor"))
    {
      constructor_params = &(*child_node);
      ++child_node;
      p_name = child_node->Name();
    }
    if (boost::equals(p_name, "parameters"))
    {
      parameters = &(*child_node);
      ++child_node;
    }

    // create mode
    tFrameworkElement* created = NULL;
    tConstructorParameters* spl = NULL;
    if (constructor_params != NULL)
    {
      spl = action->GetParameterTypes()->Instantiate();
      spl->Deserialize(*constructor_params, true);
    }
    created = action->CreateModule(parent, name, spl);
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
      if (boost::equals(name2, "element"))
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
    FINROC_LOG_PRINT(WARNING, "Failed to instantiate element. Skipping...");
    LogException(e);
  }
  catch (const std::exception& e)
  {
    FINROC_LOG_PRINT(WARNING, "Failed to instantiate element. Skipping...");
    FINROC_LOG_PRINT(WARNING, e);
  }
}

bool tFinstructableGroup::IsResponsibleForConfigFileConnections(tFrameworkElement& ap) const
{
  return parameters::internal::tParameterInfo::IsFinstructableGroupResponsibleForConfigFileConnections(*this, ap);
}

void tFinstructableGroup::LoadXml(const std::string& xml_file_)
{
  {
    tLock lock2(GetStructureMutex());
    try
    {
      FINROC_LOG_PRINT(DEBUG, "Loading XML: ", xml_file_);
      rrlib::xml::tDocument doc(core::GetFinrocXMLDocument(xml_file_, false));
      rrlib::xml::tNode& root = doc.RootNode();
      link_tmp = GetQualifiedName() + "/";
      if (main_name.length() == 0 && root.HasAttribute("defaultname"))
      {
        main_name = root.GetStringAttribute("defaultname");
      }

      // load dependencies
      if (root.HasAttribute("dependencies"))
      {
        std::vector<std::string> deps;
        boost::split(deps, root.GetStringAttribute("dependencies"), boost::is_any_of(","));
        for (size_t i = 0; i < deps.size(); i++)
        {
          std::string dep = boost::trim_copy(deps[i]);
          std::vector<std::string> loadable = internal::GetLoadableFinrocLibraries();
          bool loaded = false;
          for (size_t i = 0; i < loadable.size(); i++)
          {
            if (boost::equals(loadable[i], dep))
            {
              internal::DLOpen(dep.c_str());
              loaded = true;
              break;
            }
          }
          if (!loaded)
          {
            std::set<std::string> loaded_libs = internal::GetLoadedFinrocLibraries();
            if (loaded_libs.find(dep) == loaded_libs.end())
            {
              FINROC_LOG_PRINT(WARNING, "Dependency ", dep, " is not available.");
            }
          }
        }
      }

      for (rrlib::xml::tNode::const_iterator node = root.ChildrenBegin(); node != root.ChildrenEnd(); ++node)
      {
        std::string name = node->Name();
        if (boost::equals(name, "staticparameter"))
        {
          parameters::internal::tStaticParameterList& spl = parameters::internal::tStaticParameterList::GetOrCreate(*this);
          spl.Add(*new parameters::internal::tStaticParameterImplementationBase(node->GetStringAttribute("name"), rrlib::rtti::tType(), false, true));
        }
        else if (boost::equals(name, "element"))
        {
          Instantiate(*node, this);
        }
        else if (boost::equals(name, "edge"))
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
            dest_port->ConnectTo(QualifyLink(src), core::tAbstractPort::tConnectDirection::AUTO, true);
          }
          else if (dest_port == NULL || dest_port->GetFlag(tFlag::VOLATILE))    // destination volatile
          {
            src_port->ConnectTo(QualifyLink(dest), core::tAbstractPort::tConnectDirection::AUTO, true);
          }
          else
          {
            src_port->ConnectTo(*dest_port, core::tAbstractPort::tConnectDirection::AUTO, true);
          }
        }
        else if (boost::equals(name, "parameter"))
        {
          std::string param = node->GetStringAttribute("link");
          core::tAbstractPort* parameter = GetChildPort(param);
          if (parameter == NULL)
          {
            FINROC_LOG_PRINT(WARNING, "Cannot set config entry, because parameter is not available: ", param);
          }
          else
          {
            parameters::internal::tParameterInfo* pi = parameter->GetAnnotation<parameters::internal::tParameterInfo>();
            bool outermost_group = GetParent() == &(core::tRuntimeEnvironment::GetInstance());
            if (pi == NULL)
            {
              FINROC_LOG_PRINT(WARNING, "Port is not parameter: ", param);
            }
            else
            {
              if (outermost_group && node->HasAttribute("cmdline") && (!IsResponsibleForConfigFileConnections(*parameter)))
              {
                pi->SetCommandLineOption(node->GetStringAttribute("cmdline"));
              }
              else
              {
                pi->Deserialize(*node, true, outermost_group);
              }
              try
              {
                pi->LoadValue();
              }
              catch (std::exception& e)
              {
                FINROC_LOG_PRINT(WARNING, "Unable to load parameter value: ", param, ". ", e);
              }
            }
          }
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
      FINROC_LOG_PRINT(WARNING, "Loading XML failed: ", xml_file_);
      LogException(e);
    }
  }
}

void tFinstructableGroup::LogException(const std::exception& e)
{
  const char* msg = e.what();
  FINROC_LOG_PRINT(ERROR, msg);
}

std::string tFinstructableGroup::QualifyLink(const std::string& link)
{
  if (link[0] == '/')
  {
    return link;
  }
  return link_tmp + link;
}

void tFinstructableGroup::SaveXml()
{
  {
    tLock lock2(GetStructureMutex());
    saving_thread = &rrlib::thread::tThread::CurrentThread();
    dependencies_tmp.clear();
    std::string save_to = core::GetFinrocFileToSaveTo(xml_file.Get());
    if (save_to.length() == 0)
    {
      std::string save_to_alt = core::GetFinrocFileToSaveTo(boost::replace_all_copy(xml_file.Get(), "/", "_"));
      FINROC_LOG_PRINT(USER, "There does not seem to be any suitable location for: '", xml_file.Get(), "' . For now, using '", save_to_alt, "'.");
      save_to = save_to_alt;
    }
    FINROC_LOG_PRINT(USER, "Saving XML: ", save_to);
    rrlib::xml::tDocument doc;
    try
    {
      rrlib::xml::tNode& root = doc.AddRootNode("FinstructableGroup");

      // serialize default main name
      if (main_name.length() > 0)
      {
        root.SetAttribute("defaultname", main_name);
      }

      // serialize proxy parameters
      parameters::internal::tStaticParameterList* spl = GetAnnotation<parameters::internal::tStaticParameterList>();
      if (spl != NULL)
      {
        for (size_t i = 0; i < spl->Size(); i++)
        {
          parameters::internal::tStaticParameterImplementationBase& sp = spl->Get(i);
          if (sp.IsStaticParameterProxy())
          {
            rrlib::xml::tNode& proxy = root.AddChildNode("staticparameter");
            proxy.SetAttribute("name", sp.GetName());
          }
        }
      }

      // serialize framework elements
      SerializeChildren(root, *this);

      // serialize edges
      link_tmp = GetQualifiedName() + "/";
      for (auto it = SubElementsBegin(); it != SubElementsEnd(); ++it)
      {
        if ((!it->IsPort()) || (!it->IsReady()))
        {
          continue;
        }
        core::tAbstractPort& ap = static_cast<core::tAbstractPort&>(*it);

        // first pass
        for (auto it = ap.OutgoingConnectionsBegin(); it != ap.OutgoingConnectionsEnd(); ++it) // only outgoing edges => we don't get any edges double
        {
          core::tAbstractPort& ap2 = *it;
          if (!ap.IsEdgeFinstructed(ap2))
          {
            continue;
          }

          // save edge?
          // check1: different finstructed elements as parent?
          if (ap.GetParentWithFlags(tFlag::FINSTRUCTED) == ap2.GetParentWithFlags(tFlag::FINSTRUCTED))
          {
            // TODO: check why continue causes problems here
            // continue;
          }

          // check2: their deepest common finstructable_group parent is this
          core::tFrameworkElement* common_parent = ap.GetParent();
          while (!ap2.IsChildOf(*common_parent))
          {
            common_parent = common_parent->GetParent();
          }
          core::tFrameworkElement* common_finstructable_parent = common_parent->GetFlag(tFlag::FINSTRUCTABLE_GROUP) ? common_parent : common_parent->GetParentWithFlags(tFlag::FINSTRUCTABLE_GROUP);
          if (common_finstructable_parent != this)
          {
            continue;
          }

          // check3: only save non-volatile connections in this step (finstruct creates link edges for volatile ports)
          if (ap.GetFlag(tFlag::VOLATILE) || ap2.GetFlag(tFlag::VOLATILE))
          {
            continue;
          }

          // save edge
          rrlib::xml::tNode& edge = root.AddChildNode("edge");
          edge.SetAttribute("src", GetEdgeLink(ap));
          edge.SetAttribute("dest", GetEdgeLink(ap2));
        }

        // serialize link edges
        if (ap.link_edges)
        {
          for (size_t i = 0u; i < ap.link_edges->size(); i++)
          {
            core::internal::tLinkEdge* le = (*ap.link_edges)[i];
            if (!le->IsFinstructed())
            {
              continue;
            }
            if (le->GetSourceLink().length() > 0)
            {
              // save edge
              rrlib::xml::tNode& edge = root.AddChildNode("edge");
              edge.SetAttribute("src", GetEdgeLink(le->GetSourceLink()));
              edge.SetAttribute("dest", GetEdgeLink(ap));
            }
            else
            {
              // save edge
              rrlib::xml::tNode& edge = root.AddChildNode("edge");
              edge.SetAttribute("src", GetEdgeLink(ap));
              edge.SetAttribute("dest", GetEdgeLink(le->GetTargetLink()));
            }
          }
        }
      }

      // Save parameter config entries
      for (auto it = SubElementsBegin(); it != SubElementsEnd(); ++it)
      {
        if ((!it->IsPort()) || (!it->IsReady()))
        {
          continue;
        }
        core::tAbstractPort& ap = static_cast<core::tAbstractPort&>(*it);

        // second pass?
        bool outermostGroup = this->GetParent() == &(core::tRuntimeEnvironment::GetInstance());
        parameters::internal::tParameterInfo* info = ap.GetAnnotation<parameters::internal::tParameterInfo>();

        if (info != NULL && info->HasNonDefaultFinstructInfo())
        {
          if (!IsResponsibleForConfigFileConnections(ap))
          {

            if (outermostGroup && info->GetCommandLineOption().length() > 0)
            {
              rrlib::xml::tNode& config = root.AddChildNode("parameter");
              config.SetAttribute("link", GetEdgeLink(ap));
              config.SetAttribute("cmdline", info->GetCommandLineOption());
            }

            continue;
          }

          rrlib::xml::tNode& config = root.AddChildNode("parameter");
          config.SetAttribute("link", GetEdgeLink(ap));
          info->Serialize(config, true, outermostGroup);
        }
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
          s << (*it);
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

std::vector<std::string> tFinstructableGroup::ScanForCommandLineArgs(const std::string& finroc_file)
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
      FINROC_LOG_PRINTF_STATIC(DEBUG, "Scanning successful. Found %d additional options.", result.size());
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

void tFinstructableGroup::ScanForCommandLineArgsHelper(std::vector<std::string>& result, const rrlib::xml::tNode& parent)
{
  for (rrlib::xml::tNode::const_iterator node = parent.ChildrenBegin(); node != parent.ChildrenEnd(); ++node)
  {
    std::string name(node->Name());
    if (node->HasAttribute("cmdline") && (boost::equals(name, "staticparameter") || boost::equals(name, "parameter")))
    {
      result.push_back(node->GetStringAttribute("cmdline"));
    }
    ScanForCommandLineArgsHelper(result, *node);
  }
}

void tFinstructableGroup::SerializeChildren(rrlib::xml::tNode& node, tFrameworkElement& current)
{
  for (auto child = current.ChildrenBegin(); child != current.ChildrenEnd(); ++child)
  {
    parameters::internal::tStaticParameterList* spl = child->GetAnnotation<parameters::internal::tStaticParameterList>();
    tConstructorParameters* cps = child->GetAnnotation<tConstructorParameters>();
    if (child->IsReady() && child->GetFlag(tFlag::FINSTRUCTED))
    {
      // serialize framework element
      rrlib::xml::tNode& n = node.AddChildNode("element");
      n.SetAttribute("name", child->GetCName());
      tCreateFrameworkElementAction* cma = tCreateFrameworkElementAction::GetConstructibleElements()[spl->GetCreateAction()];
      n.SetAttribute("group", cma->GetModuleGroup());
      if (boost::ends_with(cma->GetModuleGroup(), ".so"))
      {
        AddDependency(cma->GetModuleGroup());
      }
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

void tFinstructableGroup::SetFinstructed(tFrameworkElement& fe, tCreateFrameworkElementAction* create_action, tConstructorParameters* params)
{
  assert(!fe.GetFlag(tFlag::FINSTRUCTED));
  parameters::internal::tStaticParameterList& list = parameters::internal::tStaticParameterList::GetOrCreate(fe);
  const std::vector<tCreateFrameworkElementAction*>& v = tCreateFrameworkElementAction::GetConstructibleElements();
  list.SetCreateAction(static_cast<int>(std::find(v.begin(), v.end(), create_action) - v.begin()));
  fe.SetFlag(tFlag::FINSTRUCTED);
  if (params)
  {
    fe.AddAnnotation<tConstructorParameters>(*params);
  }
}

void tFinstructableGroup::StaticInit()
{
  startup_type_count = rrlib::rtti::tType::GetTypeCount();
  startup_loaded_finroc_libs = internal::GetLoadedFinrocLibraries();
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
