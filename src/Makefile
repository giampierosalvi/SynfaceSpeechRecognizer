CC	= gcc
OPT	= -O -ffast-math 
CFLAGS	= $(OPT) -Wall -I../ext/nico-1.1.2/lib/

modules = Audio.o Common.o FeatureExtraction.o Feedback.o LikelihoodGen.o \
	nrg.o RecMath.o Recognizer.o RingBuffer.o SoundProc.o \
	SoundSource_pa.o ViterbiDecoder.o

tests = test_audio.o test_portaudio.o test_r.oognizer2.o test_r.oognizer.o

all: librec.a

librec.a: $(modules) 
	ar rv librec.a $(modules)

clean:
	rm -f *.o
	rm -f librec.a


