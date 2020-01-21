/***************************************************************************
                          Common.h  -  description
                             -------------------
    begin                : Mon Feb 27 2006
    copyright            : (C) 2006-2017 by Giampiero Salvi
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

#ifndef _COMMON_H_
#define _COMMON_H_

/* just for debugging */
#include <string.h>
#include <stdio.h>

#define MAX_PATH_LEN 512

#if DEBUG
#define DBGPRINTF(format, args...) {fprintf(stderr, "%s: ", __func__); fprintf(stderr,format, ## args); fflush(stderr);}
#define DBGPRINTF2(format, args...) {fprintf(stderr,format, ## args); fflush(stderr);}
#else
#define DBGPRINTF(format,args...) ;
#define DBGPRINTF2(format,args...) ;
#endif

#endif /* _COMMON_H_ */

