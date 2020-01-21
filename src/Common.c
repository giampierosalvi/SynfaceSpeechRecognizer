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

#include "Common.h"

/*
void DebugReport(Tcl_Interp *interp, char *msg) {
  if(interp!=NULL) {
    char *cmd;
    int cmdlen = strlen("report ")+strlen(msg)+1;
    cmd = (char *) ckalloc(cmdlen);
    strcpy(cmd,"report ");
    strcat(cmd,msg);
    Tcl_Eval(interp,cmd);
    ckfree(cmd);
  }
  else fprintf(stderr,msg);
}
*/

