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
/*!\file    plugins/runtime_construction/dynamic_loading.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2012-12-02
 *
 */
//----------------------------------------------------------------------
#include "plugins/runtime_construction/dynamic_loading.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <fstream>
#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include "rrlib/xml/tNode.h"
#include "core/tRuntimeEnvironment.h"
#include "plugins/parameters/tConfigurablePlugin.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tFinstructable.h"
#include "plugins/runtime_construction/tAdministrationService.h"

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

namespace internal
{

class tRuntimeConstructionPlugin : public parameters::tConfigurablePlugin
{
public:
  tRuntimeConstructionPlugin() : tConfigurablePlugin("runtime_construction") {}

  virtual void Init(rrlib::xml::tNode* config_node) override
  {
    tFinstructable::StaticInit();

    /*! Port that receives administration requests */
    tAdministrationService::CreateAdministrationPort();

    /*! Load plugins from config file that have not been loaded yet (TODO: move to some other plugin?) */
    rrlib::xml::tNode* root_node = parameters::tConfigurablePlugin::GetConfigRootNode();
    if (root_node)
    {
      for (auto it = root_node->ChildrenBegin(); it != root_node->ChildrenEnd(); ++it)
      {
        if (it->Name() == "plugin")
        {
          try
          {
            std::string name = it->GetStringAttribute("name");
            std::string library_name = "finroc_plugins_" + name;
            if (!core::internal::tPlugins::GetInstance().IsPluginLoaded(name))
            {
              std::vector<tSharedLibrary> loadable_libraries = GetLoadableFinrocLibraries();
              for (tSharedLibrary & shared_library : loadable_libraries)
              {
                if (shared_library.ToString() == library_name)
                {
                  FINROC_LOG_PRINT(DEBUG, "Loading plugin '" + name + "'");
                  try
                  {
                    DLOpen(shared_library);
                  }
                  catch (const std::exception& exception)
                  {
                    FINROC_LOG_PRINT(ERROR, "Loading plugin '" + name + "' failed: ", exception.what());
                  }
                  break;
                }
              }
            }
          }
          catch (const rrlib::xml::tException& e)
          {
            FINROC_LOG_PRINT(WARNING, "Config file contains plugin entry without 'name' attribute. This will be ignored.");
          }
        }
      }
    }
  }
};

static tRuntimeConstructionPlugin plugin;

}

// closes dlopen-ed libraries
class tDLCloser
{
public:
  std::vector<void*> loaded;

  tDLCloser() : loaded() {}

  ~tDLCloser()
  {
    core::tRuntimeEnvironment::Shutdown();
    for (size_t i = 0; i < loaded.size(); i++)
    {
      dlclose(loaded[i]);
    }
  }
};

static inline unsigned int GetLongevity(tDLCloser*)
{
  return 0xFFFFFFFF; // unload code after everything else
}


typedef rrlib::design_patterns::tSingletonHolder<tDLCloser, rrlib::design_patterns::singleton::Longevity> tDLCloserInstance;

void DLOpen(const tSharedLibrary& shared_library)
{
  void* handle = dlopen(shared_library.ToString(true).c_str(), RTLD_NOW | RTLD_GLOBAL);
  if (handle)
  {
    tDLCloserInstance::Instance().loaded.push_back(handle);
    core::internal::tPlugins::GetInstance().InitializeNewPlugins();
    return;
  }
  throw std::runtime_error(std::string("Error from dlopen: ") + dlerror());
}

std::set<tSharedLibrary> GetAvailableFinrocLibraries()
{
  // this implementation searches in path of libfinroc_core.so and in path <$FINROC_HOME>/export/<$TARGET>/lib
  std::vector<std::string> paths;
  tSharedLibrary core_lib = GetBinary((void*)GetBinary);
  if (core_lib.GetPath().length() > 0)
  {
    paths.push_back(core_lib.GetPath());
  }

  char* finroc_home = getenv("FINROC_HOME");
  char* target = getenv("FINROC_TARGET");
  if (finroc_home == NULL || target == NULL)
  {
    FINROC_LOG_PRINT(WARNING, "FINROC_HOME/FINROC_TARGET not set.");
  }
  else
  {
    std::string local_path = std::string(finroc_home) + "/export/" + std::string(target) + "/lib";
    if (local_path != core_lib.GetPath())
    {
      FINROC_LOG_PRINTF(DEBUG, "Searching for finroc modules in %s and %s.", core_lib.GetPath().c_str(), local_path.c_str());
      paths.push_back(local_path);
    }
  }

  std::set<tSharedLibrary> result;
  for (size_t i = 0; i < paths.size(); i++)
  {
    // platform-specific code
    DIR* dir = opendir(paths[i].c_str());
    if (dir != NULL)
    {
      while (dirent* dir_entry = readdir(dir))
      {
        std::string file(dir_entry->d_name);
        if ((file.substr(0, 10).compare("libfinroc_") == 0 || file.substr(0, 9).compare("librrlib_") == 0) && file.substr(file.length() - 3, 3).compare(".so") == 0)
        {
          result.insert(file);
        }
      }
      closedir(dir);
    }
  }
  return result;
}


tSharedLibrary GetBinary(void* addr)
{
  Dl_info info;
  dladdr(addr, &info);
  return tSharedLibrary(info.dli_fname);
}


std::set<tSharedLibrary> GetLoadedFinrocLibraries()
{
  // this implementation looks in /proc/<pid>/maps for loaded .so files

  // get process id
  __pid_t pid = getpid();

  // scan for loaded .so files
  std::stringstream mapsfile;
  mapsfile << "/proc/" << pid << "/maps";
  std::set<tSharedLibrary> result;
  std::ifstream maps(mapsfile.str().c_str());
  std::string line;
  while (!maps.eof())
  {
    std::getline(maps, line);
    if (line.find("/libfinroc_") != std::string::npos && line.substr(line.length() - 3, 3).compare(".so") == 0)
    {
      tSharedLibrary loaded = line.substr(line.find("/libfinroc_") + 1);
      if (result.find(loaded) == result.end())
      {
        FINROC_LOG_PRINT(DEBUG_VERBOSE_1, "Found loaded finroc library: ", loaded.ToString(true));
        result.insert(loaded);
      }
    }
    else if (line.find("/librrlib_") != std::string::npos && line.substr(line.length() - 3, 3).compare(".so") == 0)
    {
      tSharedLibrary loaded = line.substr(line.find("/librrlib_") + 1);
      if (result.find(loaded) == result.end())
      {
        FINROC_LOG_PRINT(DEBUG_VERBOSE_1, "Found loaded finroc library: ", loaded.ToString(true));
        result.insert(loaded);
      }
    }
  }
  maps.close();
  return result;
}

std::vector<tSharedLibrary> GetLoadableFinrocLibraries()
{
  std::vector<tSharedLibrary> result;
  std::set<tSharedLibrary> available = GetAvailableFinrocLibraries();
  std::set<tSharedLibrary> loaded = GetLoadedFinrocLibraries();
  std::set<tSharedLibrary>::iterator it;
  for (it = available.begin(); it != available.end(); ++it)
  {
    if (loaded.find(*it) == loaded.end())
    {
      result.push_back(*it);
    }
  }
  return result;
}

tCreateFrameworkElementAction& LoadComponentType(const tSharedLibrary& shared_library, const std::string& name)
{
  // contains all dynamically loaded .so files
  static std::vector<tSharedLibrary> loaded;

  // try to find component type among loaded ones
  const std::vector<tCreateFrameworkElementAction*>& modules = tCreateFrameworkElementAction::GetConstructibleElements();
  for (size_t i = 0u; i < modules.size(); i++)
  {
    tCreateFrameworkElementAction* cma = modules[i];
    if (cma->GetModuleGroup() == shared_library && cma->GetName().compare(name) == 0)
    {
      return *cma;
    }
  }

  // Component type not found. Load shared library if this has not been done yet - and possibly try again.
  bool already_loaded = false;
  for (size_t i = 0; i < loaded.size(); i++)
  {
    if (loaded[i] == shared_library)
    {
      already_loaded = true;
      break;
    }
  }

  if (!already_loaded)
  {
    loaded.push_back(shared_library);
    std::set<tSharedLibrary> loaded = GetLoadedFinrocLibraries();
    if (loaded.find(shared_library) == loaded.end())
    {
      DLOpen(shared_library);
      return LoadComponentType(shared_library, name);
    }
  }

  throw std::runtime_error("No component type '" + name + "' available in '" + shared_library.ToString(true) + "'");
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
