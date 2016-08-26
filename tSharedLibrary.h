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
/*!\file    plugins/runtime_construction/tSharedLibrary.h
 *
 * \author  Max Reichardt
 *
 * \date    2013-10-20
 *
 * \brief   Contains tSharedLibrary
 *
 * \b tSharedLibrary
 *
 * This class stores the name of shared library.
 * It can provide the platform-dependent and platform-independent name.
 * For serialization, the platform-independent name is used.
 *
 */
//----------------------------------------------------------------------
#ifndef __plugins__runtime_construction__tSharedLibrary_h__
#define __plugins__runtime_construction__tSharedLibrary_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <string>

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
//! Shared Library Reference
/*!
 * This class stores the name of shared library.
 * It can provide the platform-dependent and platform-independent name.
 * For serialization, the platform-independent name is used.
 */
class tSharedLibrary
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  /*!
   * \param name Name of shared library. Can be platform-dependent (lib*.so on Linux) or platform-independent.
   */
  tSharedLibrary(const std::string& name);
  tSharedLibrary(const char* name);

  tSharedLibrary();

  /*!
   * \return Path name if the name provided to the constructor included a path (otherwise empty string)
   */
  std::string GetPath() const
  {
    return path;
  }

  /*!
   * \return Whether this object contains valid information on a shared library
   */
  bool IsValid() const
  {
    return name.length();
  }

  /*!
   * \param platform_dependent Return platform-dependent name of shared library? (lib*.so on Linux)
   * \return Returns file name of shared library (without path)
   */
  std::string ToString(bool platform_dependent = false) const;

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  // more efficient, if operator is a friend
  friend bool operator==(const tSharedLibrary& lhs, const tSharedLibrary& rhs);
  friend bool operator<(const tSharedLibrary& lhs, const tSharedLibrary& rhs);

  /*! Platform-independent name */
  std::string name;

  /*! Path name if the name provided to the constructor included a path (otherwise empty string) */
  std::string path;
};


inline bool operator==(const tSharedLibrary& lhs, const tSharedLibrary& rhs)
{
  return lhs.name == rhs.name;
}

inline bool operator!=(const tSharedLibrary& lhs, const tSharedLibrary& rhs)
{
  return !(lhs == rhs);
}

inline bool operator<(const tSharedLibrary& lhs, const tSharedLibrary& rhs)
{
  return lhs.name < rhs.name;
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
