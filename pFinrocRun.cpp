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
/*!\file    plugins/runtime_construction/pFinrocRun.cpp
 *
 * \author  Max Reichardt
 *
 * \date    2013-04-28
 *
 * \b pFinrocRun
 *
 * The finroc_run tool.
 * Instantiates and executes modules specified in a .finroc file.
 *
 */
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <chrono>
#include "rrlib/getopt/parser.h"
#include "rrlib/logging/configuration.h"
#include "core/file_lookup.h"
#include "core/tRuntimeEnvironment.h"
#include "plugins/structure/main_utilities.h"
#include "plugins/structure/tTopLevelThreadContainer.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "plugins/runtime_construction/tFinstructable.h"

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
const std::string cFINROC_FILE_EXTENSION = ".finroc";
const std::string cPROGRAM_DESCRIPTION = "This program instantiates and executes modules specified in one or more " + cFINROC_FILE_EXTENSION + " files.";
const std::string cCOMMAND_LINE_ARGUMENTS = "<" + cFINROC_FILE_EXTENSION + "-files>";
const std::string cADDITIONAL_HELP_TEXT = "To set a group name use <name>:<" + cFINROC_FILE_EXTENSION + "-file>. Otherwise the filename is used as group name";
bool make_all_port_links_unique = true;

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
  tTopLevelThreadContainer<>* thread_container;

  tFinrocFile(const std::string &argument) :
    thread_container(nullptr)
  {
    if (argument.find(':') != std::string::npos)
    {
      this->file_name = argument.substr(argument.rfind(':') + 1);
      this->main_name = argument.substr(0, argument.rfind(':'));
    }
    else
    {
      this->file_name = argument;

      // Main name specified in .finroc file?
      if (FinrocFileExists(this->file_name))
      {
        try
        {
          rrlib::xml::tDocument doc(GetFinrocXMLDocument(this->file_name, false));
          rrlib::xml::tNode& root = doc.RootNode();
          if (root.HasAttribute("defaultname"))
          {
            this->main_name = root.GetStringAttribute("defaultname");
          }
        }
        catch (std::exception& e)
        {
          FINROC_LOG_PRINT(ERROR, "Error scanning file: ", this->file_name);
        }
      }

      // If not, use name of .finroc file
      if (this->main_name.length() == 0)
      {
        this->main_name = this->file_name;
        if (argument.find("/") != std::string::npos)
        {
          this->main_name = argument.substr(argument.rfind("/") + 1); // cut off path
        }
        this->main_name = this->main_name.substr(0, this->main_name.length() - 7); // cut off .finroc
      }
    }
  }
};

rrlib::time::tDuration cycle_time = std::chrono::milliseconds(40);
std::vector<std::string> finroc_file_extra_args;
std::vector<tFinrocFile> finroc_files;

//----------------------------------------------------------------------
// CycleTimeHandler
//----------------------------------------------------------------------
bool CycleTimeHandler(const rrlib::getopt::tNameToOptionMap &name_to_option_map)
{
  rrlib::getopt::tOption time_option(name_to_option_map.at("cycle-time"));
  if (time_option->IsActive())
  {
    std::string new_time_string = rrlib::getopt::EvaluateValue(time_option);
    rrlib::time::tDuration new_time;
    if (new_time_string.find('.') != std::string::npos)
    {
      new_time = std::chrono::nanoseconds(static_cast<int64_t>(std::atof(new_time_string.c_str()) * 1000000));
    }
    else
    {
      new_time = std::chrono::milliseconds(std::atoi(new_time_string.c_str()));
    }
    if (new_time.count() <= 0 || new_time > std::chrono::seconds(10))
    {
      FINROC_LOG_PRINT(ERROR, "Invalid cycle time '", new_time_string, "'. Using default: ", cycle_time);
    }
    else
    {
      FINROC_LOG_PRINT(DEBUG, "Setting main thread cycle time to ", std::chrono::duration_cast<std::chrono::duration<double>>(new_time).count() * 1000, " ms.");
      cycle_time = new_time;
    }
  }

  return true;
}

//----------------------------------------------------------------------
// FinrocFileArgHandler
//----------------------------------------------------------------------
bool FinrocFileArgHandler(const rrlib::getopt::tNameToOptionMap &name_to_option_map)
{
  for (size_t i = 0; i < finroc_file_extra_args.size(); i++)
  {
    rrlib::getopt::tOption extra_option(name_to_option_map.at(finroc_file_extra_args[i].c_str()));
    if (extra_option->IsActive())
    {
      finroc::core::tRuntimeEnvironment::GetInstance().AddCommandLineArgument(finroc_file_extra_args[i], rrlib::getopt::EvaluateValue(extra_option));
    }
  }

  return true;
}

//----------------------------------------------------------------------
// main
//----------------------------------------------------------------------
int main(int argc, char **argv)
{
  if (!finroc::structure::InstallSignalHandler())
  {
    FINROC_LOG_PRINT(ERROR, "Error installing signal handler. Exiting...");
    return EXIT_FAILURE;
  }

  rrlib::logging::default_log_description = basename(argv[0]);
  rrlib::logging::SetLogFilenamePrefix(basename(argv[0]));

  finroc::structure::RegisterCommonOptions();
  rrlib::getopt::AddValue("cycle-time", 't', "Cycle time of main thread in ms (default is 40)", &CycleTimeHandler);

  for (int i = 1; i < argc; ++i)
  {
    std::string argument(argv[i]);
    if (argument.length() > cFINROC_FILE_EXTENSION.length() && argument.find(cFINROC_FILE_EXTENSION) != std::string::npos)
    {
      finroc_files.push_back(tFinrocFile(argument));

      // Scan for additional command line arguments (possibly specified in .finroc file)
      finroc_file_extra_args = finroc::runtime_construction::tFinstructable::ScanForCommandLineArgs(finroc_files.back().file_name);
      for (size_t i = 0; i < finroc_file_extra_args.size(); i++)
      {
        rrlib::getopt::AddValue(finroc_file_extra_args[i].c_str(), 0, "", &FinrocFileArgHandler);
      }
    }
    if (argument == "--port-links-are-not-unique")
    {
      make_all_port_links_unique = false;
      FINROC_LOG_PRINT(DEBUG, "Port links will be unique");
    }
    if (argument == "--profiling")
    {
      finroc::scheduling::SetProfilingEnabled(true);
    }
  }

  // Create thread containers
  for (auto it = finroc_files.begin(); it != finroc_files.end(); ++it)
  {
    it->thread_container = new tTopLevelThreadContainer<>(it->main_name, it->file_name, true, make_all_port_links_unique);
    it->thread_container->GetAnnotation<finroc::runtime_construction::tFinstructable>()->SetMainName(it->main_name);
    if (it == finroc_files.begin())
    {
      it->thread_container->InitiallyShowInTools(); // show the first thread container in tools
    }
  }

  std::vector<std::string> remaining_arguments = rrlib::getopt::ProcessCommandLine(argc, argv, cPROGRAM_DESCRIPTION, cCOMMAND_LINE_ARGUMENTS, cADDITIONAL_HELP_TEXT);

  if (finroc_files.size() != remaining_arguments.size())
  {
    FINROC_LOG_PRINT_STATIC(WARNING, "Something unintended happened while parsing the command line arguments of this program.");
    FINROC_LOG_PRINT_STATIC(WARNING, "Is there an option that takes a ", cFINROC_FILE_EXTENSION, "-file as value?");
    FINROC_LOG_PRINT_STATIC(WARNING, "In that case the ", cFINROC_FILE_EXTENSION, "-file was accidently instantiated and will be started.");
  }

  if (finroc_files.empty())
  {
    FINROC_LOG_PRINT(ERROR, "No ", cFINROC_FILE_EXTENSION, "-file specified! See ", basename(argv[0]), " --help for more information.");
    return EXIT_FAILURE;
  }

  for (auto it = finroc_files.begin(); it != finroc_files.end(); ++it)
  {
    it->thread_container->SetCycleTime(cycle_time);
  }

  finroc::structure::InstallCrashHandler();
  finroc::structure::ConnectTCPPeer(finroc_files[0].main_name);

  return finroc::structure::InitializeAndRunMainLoop(basename(argv[0]));
}
