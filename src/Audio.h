/***************************************************************************
                         Audio.h  -  description
                             -------------------
    begin                : Tue May 8 2007
    copyright            : (C) 2007-2017 by Giampiero Salvi
    email                : giampi@kth.se

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 ***************************************************************************/

#ifndef _AUDIO_H_
#define _AUDIO_H_

//#include "AudioErrorCodes.h"

//using std::string; 
/* Global audio related functions */
const char* audio_getversion();
/* falls back on Pa_GetErrorText in case */
const char* audio_geterrortext(int errorCode);
int audio_initialize();
//int audio_jackinit();
int audio_terminate();
int audio_getnumapis();
const char* audio_getapiname(int api);
int audio_getnumdevs();

/* This can be easily used in Tcl by assigning the result to an array, for examople:
    array set a [audio_getdevinfo 1]
  puts $a(name)...
*/
const char* audio_getdevinfo(int dev);

int audio_getdevinchs(int dev);
int audio_getdevoutchs(int dev);
const char* audio_getdevname(int dev);
int audio_getdevapi(int dev);

#endif // _AUDIO_H_
