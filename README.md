# SynfaceSpeechRecognizer
Speech recognizer for the Synface project. The recognizer uses recurrent neural networks (RNNs) to classify speech into phonetic classes. Though this modelling is very common nowadays, in this case, special care has been put to ensure **real-time** and especially **low latency** processing.


### Main publications
There is a number of publications describing the system as a whole and experiments with hearing impaired users that can be found [here](http://www.kth.se/profile/giampi/publications/) searching for the term SynFace. However, the publications where the recognizer is described are mainly the ones below:

**Dynamic behaviour of connectionist speech recognition with strong latency constraints**  
*Giampiero Salvi*  
Speech Communication, volume 48, issue 7, July 2006, pages 802-818  
DOI: [10.1016/j.specom.2005.05.005](https://doi.org/10.1016/j.specom.2005.05.005)

**Segment boundary detection via class entropy measurements in connectionist phoneme recognition**  
*Giampiero Salvi*  
Speech Communication, volume 48, issue 12, December 2006, pages 1666-1676  
DOI: [10.1016/j.specom.2006.07.009](https://doi.org/10.1016/j.specom.2006.07.009)

## Synface Architecture
The Synface application uses speech recognition to animate a synthetic face in real time and with low latency for teleconference applications. The advantage over video conference is that any sound source can be animated this way. In teleconference settings, the participants do not need a camera for this to work.

![alt text](https://github.com/giampierosalvi/SynfaceSpeechRecognizer/blob/master/doc/synface_architecture.png "Synface Architecture")

The main goal in the [Teleface](http://www.speech.kth.se/teleface/) and [Synface](http://www.speech.kth.se/synface/) projects was to use this technology to improve telephone communication for hearing impaired persons. The technology has been commercialized between 2006 and 2016 by the Swedish company Synface AB. It is now used as a software module in [Furhat Robotics](https://www.furhatrobotics.com/). The blocks with colored background are implemented by the code in this repository. Not included here is the code to train the RNN-HMM models.

## Instructions
The code can be built on GNU/Linux, Windows (mingw) and Mac. The following instructions are for Ubuntu GNU/Linux. CMake files with cross platform build instructions may be added in the future.

The code is divided into the following parts:
* `src`: C files defining the recognizer
* `ext`: external dependecies
* `tcl`: extensions to build a Tcl package
* `share`: model and configuration files for a number of languages

### Dependencies
The code depends on portaudio:  
```
sudo apt install portaudio19-dev
```

The NICO toolkit:
```
cd ext
make nico
```

### Building the C code
If you installed all the dependencies, your should be able to run:
```
cd src
make
```
