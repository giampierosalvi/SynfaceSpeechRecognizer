/***************************************************************************
                         BinaryBuffer.h  -  description
                             -------------------
    begin                : Tue May 13 2003
    copyright            : (C) 2003-2017 by Giampiero Salvi
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

/* These are functions that imitate the functionality of the Read...
   functions in NICO System.h when the source is a memory buffer
   rather than a stream */

#include "RTDNN.h"

typedef struct BinaryBuffer {
  unsigned char *data;
  long int pos;
  long int size;
} BinaryBuffer;

/* Load a RTDNN def file from a memory buffer */
Net *ParseNet(BinaryBuffer *buf);

BinaryBuffer* BinaryBuffer_Create(unsigned char * data, int size);
void BinaryBuffer_Free(BinaryBuffer* buf);
short ParseShort(BinaryBuffer *buf);
long ParseLong(BinaryBuffer *buf);
int ParseLongs(long *i, int n, BinaryBuffer *buf);
char *ParseString(BinaryBuffer *buf);
float ParseFloat(BinaryBuffer *buf);
int ParseFloats(float *f, int n, BinaryBuffer *buf);

