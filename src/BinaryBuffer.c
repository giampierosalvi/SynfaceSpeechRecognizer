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
   rather than a stream. They were created in order to open the network
   from a tcl executable wrapped with tclkit. In this form, the file
   system is virtual and not accessible from C functions directly.
   The network file is therefore loaded first into memory through a
   tcl function and then passed to the C code through these function.
   All this is not necessary if we are running from C, or even from
   tcl (or any other scripting language) in native mode. */

#include <stdlib.h> // malloc, calloc
#include <string.h> // memcpy
#include <math.h> // floor, fabs, ...
#include "BinaryBuffer.h"
#include "System.h"

static void 
swap_2bytes(char *bytes, int n) {
  int i;
  char c, *b = bytes;

  for (i = 0; i < n; i++) {
    c = *b;
    *b = *(b + 1);
    b++;
    *b = c;
    b++; 
  }
}


static void 
swap_4bytes(char *bytes, int n) {
  int i;
  char c, *b = bytes;

  for (i = 0; i < n; i++) {
    /* swap first with last byte */
    c = *b;
    *b = *(b + 3);
    *(b + 3) = c;
    /* swap middle two bytes */
    b++;
    c = *b;
    *b = *(b + 1);
    *(b + 1) = c;

    b += 3;
  }
}

static short lowbyteisoneandhighbyteiszero = 1;
/* if the native byte order is little endian, we need to swap bytes*/
#define SWAP_BYTE_ORDER ((int)(*(char *)&lowbyteisoneandhighbyteiszero))

BinaryBuffer* BinaryBuffer_Create(unsigned char *data, int size) {
  BinaryBuffer* buf = (BinaryBuffer*) malloc(sizeof(BinaryBuffer));

  buf->data = (unsigned char *) malloc(size*sizeof(unsigned char));
  memcpy(buf->data, data, size);
  buf->pos = 0;
  buf->size = size;
  return(buf);
}

void BinaryBuffer_Free(BinaryBuffer* buf) {
  free(buf->data);
  free(buf);
}

int bufread(unsigned char *p, size_t size, size_t count, BinaryBuffer *buf) {

  /* end of buffer reached */
  if(buf->pos == buf->size || size == 0 || count == 0) return(0);
  /* not enough items */
  if(buf->pos+size*count > buf->size) {
    count = (buf->size-buf->pos)/size;
  }
  memcpy(p, &buf->data[buf->pos], size*count);
  buf->pos += size*count;

  return(count); /* insert check with buf->size */
}

short 
ParseShort(BinaryBuffer *buf) {
  short i;

  bufread((unsigned char *) &i, 2, 1, buf);

  if (SWAP_BYTE_ORDER) swap_2bytes((char *)&i, 1);

  return i;
}

long 
ParseLong(BinaryBuffer *buf) {
  int32_t i;

  bufread((unsigned char *) &i, 4, 1, buf);

  if (SWAP_BYTE_ORDER) swap_4bytes((char *)&i, 1);

  return (long) i;
}

int 
ParseLongs(long *i, int n, BinaryBuffer *buf) {
  int m, k;
  int32_t i32[n];

  m = bufread((unsigned char *) i, 4, n, buf);

  if (m != n) {
    ErrorExit(SYSTEM_ERR_EXIT, "ReadLongs tried to read %d longs, only %d read.", n, m);
  }

  if (SWAP_BYTE_ORDER) swap_4bytes((char *)i32, n);

  for(k=0;k<m;k++) i[k] = (long) i32[k];

  return m;
}

char *
ParseString(BinaryBuffer *buf) {
  unsigned char size0;
  int size;
  char s[256], *r;

  bufread(&size0, sizeof(unsigned char), 1, buf);
  //if (size0 > 256) ErrorExit(SYSTEM_ERR_EXIT, "Attempt to read string > 256 characters");
  size = (int)size0;

  if (size == 0) return NULL;

  bufread((unsigned char *) s, sizeof(char), size, buf);

  STRDUP(r, s);
  return r;
}

float 
ParseFloat(BinaryBuffer *buf) {
  float f;

  bufread((unsigned char *) &f, 4, 1, buf);

  if (SWAP_BYTE_ORDER) swap_4bytes((char *)&f, 1);

  return f;
}

int
ParseFloats(float *f, int n, BinaryBuffer *buf) {
  int m = bufread((unsigned char *) f, 4, n, buf);

  if (m != n) {
    ErrorExit(SYSTEM_ERR_EXIT, "ReadFloats tried to read %d floats, only %d read.", n, m);
  }

  if (SWAP_BYTE_ORDER) swap_4bytes((char *)f, n);

  return m;
}

static Unit *ParseUnit(Net *net, BinaryBuffer *buf) {
  Unit *u;

  CALLOC(u, 1, Unit);

  u->Name = ParseString(buf);

  u->id = ParseLong(buf);
  u->type = ParseLong(buf);
  u->backtype = ParseLong(buf);
  u->backward_prune_thresh = ParseFloat(buf);
  u->link = ParseLong(buf);
  u->pos = ParseLong(buf);

  if (net->Version != 0.5) {
    u->NumParents = ParseLong(buf);
  }

  return u;
}

static Group *ParseGroup(BinaryBuffer *buf) {
  long n;
  Group *g;

  CALLOC(g, 1, Group);

  g->Name = ParseString(buf);
  g->id = ParseLong(buf);
  g->type = ParseLong(buf);
  g->NumParents = ParseLong(buf);

  g->NumMem = ParseLong(buf);
  if (g->NumMem == 0) n = 1; else n = g->NumMem;
  CALLOC(g->MemTab, n, long)
  ParseLongs(g->MemTab, g->NumMem, buf);

  return g;
}

static Stream *ParseStream(BinaryBuffer *buf) {
  Stream *str;
  long i;

  CALLOC(str, 1, Stream);

  str->Name = ParseString(buf);
  str->id = ParseLong(buf);
  str->ext = ParseString(buf);
  str->path = ParseString(buf);
  str->size = ParseLong(buf);
  str->format = ParseLong(buf);
  str->type = ParseLong(buf);

  CALLOC(str->a, str->size, float)
  CALLOC(str->b, str->size, float)

  ParseFloats(str->a, str->size, buf);
  ParseFloats(str->b, str->size, buf);

  str->NumParents = ParseLong(buf);

  CALLOC(str->CompName, str->size, char *)
  for (i = 0; i < str->size; i++) str->CompName[i] = ParseString(buf);

  str->Filter = ParseString(buf);

  return str;
}

static Connection *
ParseConnection(Net *net, BinaryBuffer *buf, long long_format) {
  Connection *c;
  char x; 
  unsigned short y;

  CALLOC(c, 1, Connection);

  if (net->Version == 0.5) ParseLong(buf); /* read past the id entry */

  c->w = ParseFloat(buf);
  c->plast = ParseFloat(buf);

  if (long_format) {
    c->to = ParseLong(buf);
    c->from = ParseLong(buf);
  }
  else {
    y = ParseShort(buf); c->to =   (long)y;
    y = ParseShort(buf); c->from = (long)y;
  }

  bufread((unsigned char *) &x, sizeof(char), 1, buf);  c->delay =  x;

  return c;
}

LogBook *ParseLogBook(BinaryBuffer *buf) {
  LogBook *book;

  CALLOC(book, 1, LogBook)

  book->size = ParseLong(buf);
  CALLOC(book->book, book->size, char)
  bufread((unsigned char *) book->book, sizeof(char) * book->size, 1, buf);

  return book;
}

/* Parse a RTDNN def file from a memory buffer */
Net *ParseNet(BinaryBuffer *buf) {
  long i, n, magic;
  Net *net;

  magic =  ParseLong(buf);
  if (magic != RTDNN_MAGIC) {
    EmitWarning("Magic number does not match for network definition ");
    return NULL;
  }
  CALLOC(net, 1, Net)
  net->Name = ParseString(buf);

  net->Version = ParseFloat(buf);

  if (fabs(net->Version - 0.5) < 0.01) {
    /* Read an old version 0.5 network */
    EmitWarning("Old NICO-version (%.2f), converting to %.2f",
		net->Version, RTDNN_VERSION);
    net->Version = 0.5;
  }
  else if (floor(net->Version) - floor(RTDNN_VERSION) != 0) {
    EmitWarning("Can not read this NICO-version (%2.2f)", net->Version);
    return NULL;
  }

  net->NumUnits = ParseLong(buf);
  net->NumInput = ParseLong(buf);
  net->NumTanhyp = ParseLong(buf);
  net->NumArctan = ParseLong(buf);
  net->NumLinear = ParseLong(buf);
  net->NumOutput = ParseLong(buf);
  net->NumGroups = ParseLong(buf);
  net->NumConnections = ParseLong(buf);
  net->NumStreams = ParseLong(buf);
  net->MaxDelay = ParseLong(buf);
  net->NumId = ParseLong(buf);

  net->NumSortedCons = 0;

  if (net->NumId == 0) net->AllocatedId = 1;
  else net->AllocatedId = net->NumId;
  CALLOC(net->IdTab, net->AllocatedId, IdEntry)

  if (net->Version == 0.5) {
    for (i = 0; i < net->NumId; i++) {
      net->IdTab[i].type = DELETED;
    }
  }

  if (net->NumUnits == 0) n = 1; else n = net->NumUnits;
  CALLOC(net->UTab, n, Unit *)
  for (i = 0; i < net->NumUnits; i++) {
    net->UTab[i] = ParseUnit(net, buf);
    net->IdTab[net->UTab[i]->id].ptr  = (void *)net->UTab[i];
    net->IdTab[net->UTab[i]->id].Name = net->UTab[i]->Name;
    net->IdTab[net->UTab[i]->id].type = UNIT;
    if (net->UTab[i]->type == BIAS) net->biasunit = net->UTab[i];
  }

  if (net->NumGroups == 0) n = 1; else n = net->NumGroups;
  CALLOC(net->GrTab, n, Group *)
  for (i = 0; i < net->NumGroups; i++) {
    net->GrTab[i] = ParseGroup(buf);
    net->IdTab[net->GrTab[i]->id].ptr  = (void *)net->GrTab[i];
    net->IdTab[net->GrTab[i]->id].Name = net->GrTab[i]->Name;
    net->IdTab[net->GrTab[i]->id].type = GROUP;
  }
  net->rootgroup = net->GrTab[0];

  if (net->NumConnections == 0) n = 1; else n = net->NumConnections;
  CALLOC(net->CTab, n, Connection *)
  net->NumAllocatedCons = n;

  if (net->NumId >= 65536) { /* Long format */
    for (i = 0; i < net->NumConnections; i++) {
      net->CTab[i] = ParseConnection(net, buf, 1);
    }
  }
  else {
    for (i = 0; i < net->NumConnections; i++) {
      net->CTab[i] = ParseConnection(net, buf, 0);
    }
  }
  

  if (net->Version == 0.5) {
    /* This is not really necessary, but kept to fix old nets 
       that didn't have their connections sorted */ 
    SortConnections(net); 
  }

  if (net->NumStreams == 0) n = 1; else n = net->NumStreams;
  CALLOC(net->StrTab, n, Stream *);
  for (i = 0; i < net->NumStreams; i++) {
    net->StrTab[i] = ParseStream(buf);
    net->IdTab[net->StrTab[i]->id].ptr  = (void *)net->StrTab[i];
    net->IdTab[net->StrTab[i]->id].Name = net->StrTab[i]->Name;
    net->IdTab[net->StrTab[i]->id].type = STREAM;
  }


  if (net->Version == 0.5) {
    /* Fix the numParents entry in units */
    for (i = 0; i < net->NumGroups; i++) {
      Group *g;
      long j;
      
      g = net->GrTab[i];
      for (j = 0; j < g->NumMem; j++) {
	if (GetIdType(net, g->MemTab[j]) == UNIT) {
	  GETPTR(net, g->MemTab[j], Unit)->NumParents++;
	}
      }
    }
  }

  if (net->Version == 0.5) {
    ReHash(net); 
  }

  net->logbook = ParseLogBook(buf);

  net->Version = RTDNN_VERSION;
  return net;
}
