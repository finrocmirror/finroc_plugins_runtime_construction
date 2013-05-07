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
/*!\file    tools/finroc_run/pFinrocRun.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2013-04-28
 *
 * \brief Contains mFinrocRun
 *
 * \b pFinrocRun
 *
 * The finroc_run tool.
 * Instantiates and executes modules specified in a .finroc file.
 *
 */
//----------------------------------------------------------------------
#include "plugins/structure/default_main_wrapper.h"

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <chrono>
#include "core/file_lookup.h"
#include "core/tRuntimeEnvironment.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Debugging
//----------------------------------------------------------------------
#include <cassert>

//----------------------------------------------------------------------
// Namespace usage
//----------------------------------------------------------------------
using namespace finroc::core;
using namespace finroc::structure;

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Const values
//----------------------------------------------------------------------
const char * const cPROGRAM_VERSION = "1.0";
const char * const cPROGRAM_DESCRIPTION = "This program instantiates and executes modules specified in a .finroc file.";

//----------------------------------------------------------------------
// Implementation
//----------------------------------------------------------------------

/*! Data on every .finroc file specified on command line */
struct tFinrocFile
{
  /*! .finroc file name */
  std::string file_name;

  /*! main name that was possibly specified */
  std::string main_name;

  /*! Thread container that was created for .finroc file */
  tThreadContainer* thread_container;
};

int cycle_time = 40;
std::vector<std::string> finroc_file_extra_args;
std::list<tFinrocFile> finroc_files;


bool CycleTimeHandler(const rrlib::getopt::tNameToOptionMap &name_to_option_map)
{
  rrlib::getopt::tOption port_option(name_to_option_map.at("cycle-time"));
  if (port_option->IsActive())
  {
    const char* time_string = boost::any_cast<const char *>(port_option->GetValue());
    int time = atoi(time_string);
    if (time < 1 || time > 10000)
    {
      FINROC_LOG_PRINT(ERROR, "Invalid cycle time '", time_string, "'. Using default: ", cycle_time, " ms");
    }
    else
    {
      FINROC_LOG_PRINT(DEBUG, "Setting main thread cycle time to ", time, " ms.");
      cycle_time = time;
    }
  }

  return true;
}

bool FinrocFileArgHandler(const rrlib::getopt::tNameToOptionMap &name_to_option_map)
{
  for (size_t i = 0; i < finroc_file_extra_args.size(); i++)
  {
    rrlib::getopt::tOption extra_option(name_to_option_map.at(finroc_file_extra_args[i].c_str()));
    if (extra_option->IsActive())
    {
      finroc::core::tRuntimeEnvironment::GetInstance().AddCommandLineArgument(finroc_file_extra_args[i], boost::any_cast<const char *>(extra_option->GetValue()));
    }
  }

  return true;
}

//----------------------------------------------------------------------
// StartUp
//----------------------------------------------------------------------
void StartUp()
{
  rrlib::getopt::AddValue("cycle-time", 't', "Cycle time of main thread in ms (default is 40)", &CycleTimeHandler);

  // Slightly ugly command line parsing... we start at the back and look how many .finroc files the user specified
  bool scanning_for_finroc_files = true;
  bool unique_links = true;
  for (int i = finroc_argc_copy - 1; i > 0; i--)
  {
    std::string arg(finroc_argv_copy[i]);
    if (scanning_for_finroc_files && arg.length() > 0 && boost::ends_with(arg, ".finroc"))
    {

      // Determine name of finroc file - and name of group to create in runtime
      struct tFinrocFile finroc_file;
      if (arg.find(':') != std::string::npos)
      {
        finroc_file.file_name = arg.substr(arg.rfind(':') + 1);
        finroc_file.main_name = arg.substr(0, arg.rfind(':'));
      }
      else
      {
        finroc_file.file_name = arg;

        // Main name specified in .finroc file?
        if (FinrocFileExists(finroc_file.file_name))
        {
          try
          {
            rrlib::xml::tDocument doc(GetFinrocXMLDocument(finroc_file.file_name, false));
            rrlib::xml::tNode& root = doc.RootNode();
            if (root.HasAttribute("defaultname"))
            {
              finroc_file.main_name = root.GetStringAttribute("defaultname");
            }
          }
          catch (std::exception& e)
          {
            FINROC_LOG_PRINT(ERROR, "Error scanning file: ", finroc_file.file_name);
          }
        }

        // If not, use name of .finroc file
        if (finroc_file.main_name.length() == 0)
        {
          finroc_file.main_name = finroc_file.file_name;
          if (arg.find("/") != std::string::npos)
          {
            finroc_file.main_name = arg.substr(arg.rfind("/") + 1); // cut off path
          }
          finroc_file.main_name = finroc_file.main_name.substr(0, finroc_file.main_name.length() - 7); // cut off .finroc
        }
      }
      finroc_files.push_front(finroc_file);

      // Scan for additional command line arguments (possibly specified in .finroc file)
      finroc_file_extra_args = finroc::runtime_construction::tFinstructableGroup::ScanForCommandLineArgs(finroc_file.file_name);
      for (size_t i = 0; i < finroc_file_extra_args.size(); i++)
      {
        rrlib::getopt::AddValue(finroc_file_extra_args[i].c_str(), 0, "", &FinrocFileArgHandler);
      }

      // Set peer name to name of first .finroc file
      finroc_peer_name = finroc_file.main_name;
    }
    else
    {
      scanning_for_finroc_files = false;
      if (arg.compare("port-links-are-not-unique") == 0)
      {
        unique_links = false;
      }

      break;
    }
  }

  // No file specified?
  if (finroc_files.size() == 0)
  {
    FINROC_LOG_PRINT_STATIC(USER, "No finstructable groups specified.");
    FINROC_LOG_PRINT_STATIC(USER, "Usage:  finroc_run [options] <.finroc-file 1> <.finroc-file2> ...");
    FINROC_LOG_PRINT_STATIC(USER, "To set group name use <name>:<.finroc-file>. Otherwise .finroc-file name is used as group name.");
    exit(-1);
  }

  // Create thread containers
  for (auto it = finroc_files.begin(); it != finroc_files.end(); ++it)
  {
    it->thread_container = new tThreadContainer(&tRuntimeEnvironment::GetInstance(), it->main_name, it->file_name, false,
        unique_links ? tFrameworkElementFlags(tFrameworkElementFlag::GLOBALLY_UNIQUE_LINK) : tFrameworkElementFlags());
    it->thread_container->SetMainName(it->main_name);
  }
}

//----------------------------------------------------------------------
// InitMainGroup
//----------------------------------------------------------------------
void InitMainGroup(tThreadContainer *main_thread, std::vector<char*> remaining_args)
{
  for (auto it = finroc_files.begin(); it != finroc_files.end(); ++it)
  {
    it->thread_container->SetCycleTime(cycle_time);
  }
}
