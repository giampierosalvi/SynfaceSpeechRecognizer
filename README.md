# SynfaceSpeechRecognizer
Speech recognizer for the Synface project. The recognizer uses recurrent neural networks (RNNs) to classify speech into phonetic classes. Special care has been put to ensure real-time processing and especially **low latency**.


### Main publications
**Dynamic behaviour of connectionist speech recognition with strong latency constraints**  
*Giampiero Salvi*  
Speech Communication, 48(7), July 2006, 802-818  
DOI: [10.1016/j.specom.2005.05.005](https://doi.org/10.1016/j.specom.2005.05.005)

**Segment boundary detection via class entropy measurements in connectionist phoneme recognition**  
*Giampiero Salvi*  
Speech Communication, 48(12), December 2006, 1666-1676  
DOI: [10.1016/j.specom.2006.07.009](https://doi.org/10.1016/j.specom.2006.07.009)

## Synface Architecture
The Synface application uses speech recognition to animate a synthetic face in real time and with low latency for teleconference applications. The advantage over video conference is that any sound source can be animated this way. In teleconerence settings, the participants do not need a camera for this to work.

![alt text](https://github.com/giampierosalvi/SynfaceSpeechRecognizer/blob/master/doc/synface_architecture.png "Synface Architecture")

The main goal in the [Teleface](http://www.speech.kth.se/teleface/) and [Synface](http://www.speech.kth.se/synface/) projects was to use this technology to improve telephone communication for hearing impaired persons. The technology has been commercialized between 2006 and 2016 by the Swedish company Synface AB. It is now used by the Swedish company [Furhat Robotics](https://www.furhatrobotics.com/).
