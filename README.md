# Synface Speech Recognizer
This repository holds the code for the speech recognizer used in the [Synface European project](https://cordis.europa.eu/article/id/22557-eu-project-helps-the-hard-of-hearing-to-communicate-by-telephone) and later in the SynFace AB product called EyePhone. I am releasing it here for its legacy value and in the hope that it might be useful to someone. The recognizer is written in pure C and uses **recurrent neural networks (RNNs) and hidden Markov models (HMMs) to classify speech into phonetic classes**. This was a early use of this technology (development started in 1999) that has become mainstream with the advances in Deep Learning and was later surclassed by end-to-end systems. However, these kinds of hybrid DNN-HMM methods may still be relevant when the focus is on saving computational resources and providing low-latency responses. In the code provided here, the main focus was ensuring **real-time** and especially **low latency** processing with the hardware available at the time. The accuracy of this recognizer, however, is not comparable with that achievable with state-of-the-art systems today.

NOTE: The code is provided as is, and with no support.

## Synface Architecture
The Synface application uses speech recognition to animate a synthetic face in real time and with low latency for teleconference applications. The advantage over video conference is that any sound source can be animated this way. In teleconference settings, the participants do not need a camera for this to work.

![alt text](https://github.com/giampierosalvi/SynfaceSpeechRecognizer/blob/master/doc/synface_architecture.png "Synface Architecture")

The main goal in the [Teleface](http://www.speech.kth.se/teleface/) and [Synface](https://cordis.europa.eu/article/id/22557-eu-project-helps-the-hard-of-hearing-to-communicate-by-telephone) projects was to use this technology to improve telephone communication for hearing impaired persons. The technology has been commercialized between 2006 and 2016 by the Swedish company Synface AB that no longer exists. The blocks with colored background are implemented by the code in this repository. Not included here is the code to train the RNN-HMM models and to animate and render the 3D face model. Also, the code in this repository has been cleaned up from references to the EyePhone application, in particular the Tcl-tk interface used to configure the recognizer from a GUI and to connect it to the face animation software.

### Main publications
If you find this code useful, or if you would like to refer to it for historical reasons, please consider citing the following papers:

**Dynamic behavior of connectionist speech recognition with strong latency constraints**  
*Giampiero Salvi*  
Speech Communication, volume 48, issue 7, July 2006, pages 802-818  
DOI: [10.1016/j.specom.2005.05.005](https://doi.org/10.1016/j.specom.2005.05.005)

**Segment boundary detection via class entropy measurements in connectionist phoneme recognition**  
*Giampiero Salvi*  
Speech Communication, volume 48, issue 12, December 2006, pages 1666-1676  
DOI: [10.1016/j.specom.2006.07.009](https://doi.org/10.1016/j.specom.2006.07.009)

The BibTeX code for the above publications is below
```
@Article{gs:Salvi2006,
  author = 	 {Giampiero Salvi},
  title = 	 {Dynamic Behaviour of Connectionist Speech Recognition with Strong Latency Constraints},
  journal = 	 {Speech Communication},
  year = 	 {2006},
  volume = 	 {48},
  number = 	 {7},
  pages = 	 {802--818},
  month = 	 jul
  doi =          {10.1016/j.specom.2005.05.005}
}
@Article{gs:Salvi2006b,
  author = 	 {Giampiero Salvi},
  title = 	 {Segment Boundary Detection via Class Entropy Measurements in Connectionist Phoneme Recognition},
  journal = 	 {Speech Communication},
  year = 	 {2006},
  volume = 	 {48},
  number = 	 {12},
  pages = 	 {1666--1676},
  month = 	 dec,
  doi =          {10.1016/j.specom.2006.07.009}
}
```
If you would like to refer to the Synface system as a whole and to the experiments with hearing impaired users, please search for the term SynFace in the publications listed [here](https://orcid.org/0000-0002-3323-5311).

## Instructions
The code can be built on GNU/Linux, Windows (mingw) and Mac using the GNU compiler. The following instructions are for Ubuntu GNU/Linux (tested on Ubuntu 22.04). CMake files with cross platform build instructions have been currently removed from the repository until I have time to clean them from the dependencies from the EyePhone application.

The repository is divided into the following parts:
* `src`: C files defining the recognizer
* `ext`: external dependencies
* `tcl`: extensions to build a Tcl package (deprecated)
* `share`: model and configuration files for a number of languages
* `nbprojects`: Apache NetBeans project files if you want to use an IDE (not necessary and experimental).

### Building the code
The code depends on [portaudio](http://www.portaudio.com/) and the [NICO toolkit](http://nico.nikkostrom.com/). The test programs also make use of [libsndfile](http://www.mega-nerd.com/libsndfile/) to read audio files. The following commands will install the dependencies:  
```
sudo apt install portaudio19-dev libsndfile1-dev
cd ext
make nico
```
The last command downloads and build the NICO toolkit version 1.1.2. It also applies some patches to the toolkit to be able to build naively on a 64 bit system and to fix some memory leaks.

If you installed all the dependencies, your should be able to run:
```
cd ../src
make
```

### Running the test programs
In the `src` directory you can find a number of test programs to test various aspects of the code:
#### test_portaudio
lists all the available audio devices. Selecting a device, the program opens, closes, starts and stops streams a number of times in order to check possible memory leaks.
```
./test_portaudio
```
#### test_playback
plays back the sound from the default input device on the default output device. It can be used to test the minimum audio latency, but also the delay that can be added to synchronize the speech with the avatar generated articulation. **Run this program using a headset as default device to prevent audio feedback.**
```
./test_playback
```
#### test_recognizer_offline
Reads the models for that particular language and an input audio file and produces a phonetic transcription with time stamps.
```
./test_recognizer_offline <configuration file> <input audio file> <output label>
```
for example
```
./test_recognizer_offline ../share/swedish/parameters.conf <input audio file> <output label>
```
#### test_recognizer_live
Runs the recognizer live. It also plays back the audio with a delay that should correspond to delay of animation of the avatar, that is, if the results of the recognizer are fed into the animation system, the articulation should be synchronous with audio playback.
```
./test_recognizer_live
```
