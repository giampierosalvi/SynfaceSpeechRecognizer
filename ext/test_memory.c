#include <stdio.h>
#include "RTSim.h"

int main() {
  Net *net;
  FILE *filep=NULL;
  RTSimulator *sim;
  int numOutputStreams, *outputStreamSizes;
  char *filename = "../../../share/swedish/rnn.rtd";
  
  if(1) {
  filep = fopen(filename, "rb");
  if(filep==NULL) {
      printf("failed to open file");
      return -1;
  }
  net = ReadNet(filep);
  fclose(filep);
  } else {
    net = LoadNet(filename);
  }
  
  if(1) {
  sim = CompileRTSim(net);
  printf("number of inputs streams: %d\n",
	    sim->num_input_streams);
  for(int i=0;i<sim->num_input_streams;i++)
    printf("   input stream %d: size: %d\n",
	      i,sim->input_stream_size[i]);
  RTSimOutputSize(sim, &(numOutputStreams), &(outputStreamSizes));
  printf("number of outputs streams: %d\n",
	    numOutputStreams);
  for(int i=0;i<numOutputStreams;i++)
    printf("  output stream %d: size: %d\n",
	      i,outputStreamSizes[i]);
  }
 FreeNet(net);

 if(1) {
 FreeNetWork(sim->net);
 FreeRTSim(sim);
 }
 return 0;
}

/*
 Problems so far:
 * RTDNN.c, FreeStream: missing FREE(str->Filter);
 * RTDNN.c, FreeNet: missing
 *  FREE(net->Name);
 *  FREE(net);
 * Simulation.c FreeNetWork: missing
 *  FREE(net->Constants)
 *  FREE2D(net->stream_comp_name, net->NumExt);
 *  FREE2D(net->ConLink, net->NumUnits);
 *  FREE(net->stream_filter) should be FREE2D(net->stream_filter, net->NumStreams); 
 *  FREE(net);
 * RTSim.c FreeRTSim: missing:
 *  FREE(work->input_component_name);
 *  FREE(work->output_component_name);
 */