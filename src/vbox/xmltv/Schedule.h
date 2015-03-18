#pragma once
/*
*      Copyright (C) 2015 Sam Stenvall
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, write to
*  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
*  MA 02110-1301  USA
*  http://www.gnu.org/copyleft/gpl.html
*
*/

#include "Programme.h"
#include <vector>
#include <memory>

// Visual Studio can't handle type names longer than 255 characters in debug 
// mode, disable that warning since it's not important
#ifdef _MSC_VER
#pragma warning(disable : 4503)
#endif

namespace vbox {
  namespace xmltv {
    typedef std::vector<xmltv::ProgrammePtr> Schedule;
    typedef std::unique_ptr<Schedule> SchedulePtr;
  }
}
