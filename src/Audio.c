/***************************************************************************
                         Audio.c  -  description
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

#include "Audio.h"
#include <stdio.h>
#include <portaudio.h>

/* Global audio related functions */
const char* audio_getversion() {
  return Pa_GetVersionText();
}

const char* audio_geterrortext(int errorCode) {
  return Pa_GetErrorText(errorCode);
}


int audio_initialize() {
  return Pa_Initialize();
}
/*
int audio_jackinit() {
  PaUtilHostApiRepresentation **hostApi;
  PaHostApiIndex index = Pa_HostApiTypeIdToHostApiIndex(paJACK);
  PaUtil_GetHostApiRepresentation(hostApi, paJACK);
  return PaJack_Initialize(hostApi, index);
}
*/
int audio_terminate() {
  return Pa_Terminate();
}

int audio_getnumapis() {
  return Pa_GetHostApiCount();
}

const char* audio_getapiname(int api) {
  const PaHostApiInfo *info = Pa_GetHostApiInfo((PaHostApiIndex) api);
  if(info != NULL) return info->name;
  return "";
}

int audio_getnumdevs() {
  return Pa_GetDeviceCount();
}

const char* audio_getdevinfo(int dev) {
  const PaDeviceInfo *info = Pa_GetDeviceInfo(dev);
  char res[1024] = "";

  if(info!=NULL) {
    sprintf(res, "structVersion %d\n", info->structVersion);
    sprintf(res, "name \"%s\"\n", info->name);
    sprintf(res, "hostApi %d\n", info->hostApi);
    sprintf(res, "maxInputChannels %d\n", info->maxInputChannels);
    sprintf(res, "maxOutputChannels %d\n", info->maxOutputChannels);
    sprintf(res, "defaultLowInputLatency %f\n", info->defaultLowInputLatency);
    sprintf(res, "defaultLowOutputLatency %f\n", info->defaultLowOutputLatency);
    sprintf(res, "defaultHighInputLatency %f\n", info->defaultHighInputLatency);
    sprintf(res, "defaultHighOutputLatency %f\n", info->defaultHighOutputLatency);
    sprintf(res, "defaultSampleRate %f\n", info->defaultSampleRate);
  }
  // warning: address of stack memory associated with local variable 'res' returned [-Wreturn-stack-address]
  return res;
}

int audio_getdevinchs(int dev) {
  const PaDeviceInfo *info = Pa_GetDeviceInfo(dev);

  if(info==NULL) return 0;
  return info->maxInputChannels;
}

int audio_getdevoutchs(int dev) {
  const PaDeviceInfo *info = Pa_GetDeviceInfo(dev);

  if(info==NULL) return 0;
  return info->maxOutputChannels;
}

int audio_getdevapi(int dev) {
  const PaDeviceInfo *info = Pa_GetDeviceInfo(dev);

  if(info==NULL) return -1;
  return (int) info->hostApi;
}

const char* audio_getdevname(int dev) {
  const PaDeviceInfo *info = Pa_GetDeviceInfo(dev);

  if(info!=NULL) return info->name;

  return "";
}
