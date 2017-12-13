/***************************************************************************
                         rec.h  -  Tcl rec object
                             -------------------
    begin                : Tue Nov 10 2009
    copyright            : (C) 2009 by Giampiero Salvi
    email                : giampi@kth.se
 ***************************************************************************/

/* To be wrapped into a tcl object: used to map tcl commands into rec objects */

#include "rec.h"

Rec::Rec() {
  r = Recognizer_Create();
}

Rec::~Rec() {
  return Recognizer_Free(r);
}

/* this is used within tclkit as a workaround to the problem of reading
   files from C/C++ in the virtual file system.
   NOTE that this function exploits the fact that elements in a C++ vector
   occupy contiguous memory addresses and thus &net[0] can be used as a 
   pointer to an array of chars. */
int Rec::loadnet() {
  int res;
  if(ann.size()==0) return -1;

  BinaryBuffer *buf = BinaryBuffer_Create((unsigned char *) &ann[0],
					  ann.size());

  res = LGLoadANN(r->lg, buf)
  BinaryBuffer_Free(buf);

  return res;
}

/* Setting all parameters is delayed to this method: whenever the parameters are
   configured from the target language, only a copy is made in the object. When
   the recognizer is started, this method is called, and all the parameters are set.
   This is tedious, but it makes it possible to use SWIG automatic generation of
   accessory methods to set and get public member variables.
*/
int Rec::applysettings() {
  int err;

  /* fbdealy */
  SoundSource_SetFBDelay(r->s, fbdelay);
  /* fbflag */
  r->s->feedback_flag = fbflag;
  /* indev */
  SoundSource_SetInDevice(r->s, indev);
  /* outdev */
  SoundSource_SetOutDevice(r->s, outdev);
  /* lmsflag */
  r->s->lms_flag = lmsflag;
  /* simplexflag */
  r->s->simplex_flag = simplexflag;
  /* debug */
  Recognizer_SetDebug(r, debug);
  /* asrinputlevel */
  if(abs(asrinputlevel-1.0) < 0.01) asrinputlevel = 1.0;
  r->asr_in_level_ratio = (float) level;
  r->asr_max_input_level = (short) SHRT_MAX/level;
  /* fe:inputrate */
  if(fe:inputrate % 8000 != 0) return -1;
  r->fe->inputrate = fe:inputrate;
  /* fe:numfilters */
  r->fe->numfilters = fe:numfilters;
  /* fe:numceps */
  r->fe->numceps = fe:numceps;
  /* fe:lowfreq */
  r->fe->lowfreq = (float) fe:lowfreq;
  /* fe:hifreq */
  r->fe->hifreq = (float) fe:hifreq;
  /* fe:framestep */
  r->fe->framestep = (float) fe:framestep;
  /* fe:framelen */
  r->fe->framelen = (float) fe:framelen;
  /* fe:preemph */
  r->fe->preemph = (float) fe:preemph;
  /* fe:lifter */
  r->fe->lifter = (float) fe:lifter;
  /* audiorate */
  if(audiorate % 8000 != 0) return -1;
  SoundSource_SetSamplingRate(r->s, audiorate);
  /* ann */
  if(err = loadnet()) return err;
  /* -phprior ????? */
  Vector *pp = AllocVector(phprior.size());
  for(int i=0; i<phprior.size(); i++) pp->data[i] = (float) phprior[i];
  LikelihoodGen_SetPhPrior(r->lg,pp); /* sets pointers, do not free pp */
  /* vt:lookahead */
  Recognizer_SetLookahead(r,vt:lookahead);
  /* decoder */
  if(decoder.compare("viterbi")==0) {
    if(r->vd == NULL) r->vd = ViterbiDecoder_Create();
  } else {
    if(r->vd != NULL) ViterbiDecoder_Free(r->vd);
  }
  /* -grammar ????? */

  return 0;
}

string Rec::getqueue() {
    Result *res;
    stringstream results;
    results << "{ ";
    while((res = Recognizer_GetResult(r))) {
      results << "{ " << r->ph[res->idx] << " ";
      results << res->frame_time << " ";
      results << res->playback_time << " }";
      free(res);
    }
    results << " }";
    return results.str();
}

int Rec::getoutpos() {
  return SoundSource_GetOutPos(r->s);
}

int Rec::getinpos() {
  return SoundSource_GetInPos(r->s);
}

double Rec::getstreamtime() {
  return SoundSource_GetStreamTime(r->s);
}

int Rec::start() {
  // here do all the configurations
  applysettings();

  return Recognizer_Start(r);
}

int Rec::stop() {
  return Recognizer_Stop(r);
}

//configure
//destroy
double Rec::getpeakdb() {
  double peakdb;
  nrg_ReturnPeakDB(r->s->nrgY,&peakdb);
  return peakdb;
}

/* note that:
   nrgY <-> channel 0 (mix of local and remote voices)
   nrgX <-> channel 1 (local voice) */
double Rec::getxenergy() {
  return nrg_ReturnEnergy(r->s->nrgX);
}

double Rec::getyenergy() {
  return nrg_ReturnEnergy(r->s->nrgY);
}

double Rec::getyavenergy(int len) {
  return nrg_ReturnAvEnergy(r->s->nrgY, len);
}

double Rec::getxavenergy(int len) {
  if(r->s->inputParameters.channelCount < 2) return 0.0;
  return nrg_ReturnAvEnergy(r->s->nrgX, len);
}

int Rec::getannoutsize() {
  return LikelihoodGen_GetOutSize(r->lg);
}

//getlikelihoodvec
//getfeaturevec
//grammargen
//testgrammar
//grammarduplicate
//grammarequal
//dumpviterbiinfo
//donothing
//create
