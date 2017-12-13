############################################################################
#                         vit.tcl  -  description
#                             -------------------
#    begin                : Tue February 24 2004
#    copyright            : (C) 2004 by Giampiero Salvi
#    email                : giampi@speech.kth.se
############################################################################

############################################################################
#                                                                          #
#   This program is free software; you can redistribute it and/or modify   #
#   it under the terms of the GNU General Public License as published by   #
#   the Free Software Foundation; either version 2 of the License, or      #
#   (at your option) any later version.                                    #
#                                                                          #
############################################################################

package provide vit

namespace eval vit {
  variable Info
  set Info(options) "-symbols -phoneprior -grammar -gramfact -insweight -lookahead -obsisbin -obsisact"
  set Info(dir)        [file dirname [info script]]
  #set Info(datadir) [file normalize [file join $Info(appdir) .. .. data]]
  set Info(grammar)     {}
  set Info(symbols)     {}
  set Info(phoneprior)  {}
  set Info(lookahead)   0
  set Info(gramfact)    1
  set Info(insweight)   0
  set Info(obsisbin) 0
  set Info(obsisact) 0
  set Info(tracel) 0
}

source [file join $vit::Info(dir) mlf.tcl]
source [file join $vit::Info(dir) fio.tcl]
source [file join $vit::Info(dir) gram.tcl]

proc vit::create {w args} {
  variable Info
  namespace eval [namespace current]::$w {
    variable data
  }

  fio::trace $Info(tracel) 1 "vit.tcl: ...creating C part of the recognizer"
  vit create $w.vit

  fio::trace $Info(tracel) 1 "vit.tcl: ...creating object command"
  proc ::$w {cmd args} "eval [namespace current]::invoke $w \$cmd \$args"

  fio::trace $Info(tracel) 1 "vit.tcl: ...configuring recognizer"
  eval configure $w $args
  return $w
}

proc vit::destroy {w} {
  $w.vit destroy
  rename ::$w {}
}

proc vit::configure {w args} {
  variable Info
  upvar [namespace current]::${w}::data d
  set updateviterbiconf 0
  
  if {[llength $args]==0} {
    set res ""
    foreach opt $Info(options) {
      lappend res [$w cget $opt]
    }
    return $res
  }
  
  foreach {opt val} $args {
    switch -- $opt {
      -symbols {
	#puts "vit.tcl: ...configure -symbols"
	set Info(symbols) [fio::readdata $val]
      }
      -phoneprior {
	#puts "vit.tcl: ...configure -phoneprior"
	set Info(phPrior) [fio::readdata $val]
	$w.vit configure -phprior $Info(phPrior)
      }
      -grammar {
	#puts "vit.tcl: ...configure -grammar"
	if {[llength $val] != 3} {
	  error "-grammar \{transmat stateprior fisstatemap\}"
	}
	set Info(grammar) $val
	loadgram $w $val
      }
      -gramfact {
	#puts "vit.tcl: ...configure -gramfact"
	set Info(gramfact) $val
	if {$Info(grammar)!={}} {loadgram $w $Info(grammar)}
      }
      -insweight {
	#puts "vit.tcl: ...configure -insweigth"
	set Info(insweight) $val
	if {$Info(grammar)!={}} {loadgram $w $Info(grammar)}
      }
      -lookahead {
	#puts "vit.tcl: ...configure -lookahead"
	set Info(lookahead) $val
	$w.vit configure -lookahead $val
      }
      -obsisbin {
	set Info(obsisbin) $val
      }
      -obsisact {
	set Info(obsisact) $val
      }
      default {
	if [catch {
	  $w.vit configure $opt $val
	}
	   ] {
	  error "unknown option \"$opt\""
	}
      }
    }
  }
  # this is to handle setting only part of the viterbi parameters
  if {$updateviterbiconf} {
    if {$d(decoder) == "map"} {
      puts "warning: setting decoder to \"viterbi\""
      set d(decoder) "viterbi"
    }
    createViterbi $w
    set updateviterbiconf 0
  }
}

proc vit::cget {w opt} {
  upvar [namespace current]::${w}::data d
  variable Info

  switch -- $opt {
    -symbols {
      return $Info(symbols)
    }
    -phoneprior {
      return [$w getphoneprior]
    }
    -lookahead {
      return $Info(lookahead)
    }
    -grammar {
      puts "-grammar {transmat stateprior fisstatemap}"
    }
    -gramfact {
      return $Info(gramfact)
    }
    -insweigth {
      return $Info(insweight)
    }
    -obsisbin {
      return $Info(obsisbin)
    }
    -obsisact {
      return $Info(obsisact)
    }
    default {
      error "unknown option \"$opt\""
    }
  }
}

# processfile - process one observation file
proc vit::processfile {w input res} {
  $w loadobs $input
  return [$w processobs]
}

# processobs - process a previously loaded observation
proc vit::processobs {w} {
  #upvar $res r
  #set r(n) 0
  $w.vit processobs
#[$w.vit processobs]
  set path [idx2sym [state2idx [$w.vit getpath]]]
  #set r(entropy) {}
  #[$w.vit getentropy]
  #if {$n != [llength $path] || $n != [llength $entropy]} {
  #  error "something wrong with \"processobs\""
  #}
  return $path
}

# fbfmap - returns the maximum a posteriori result for each frame
proc vit::fbfmap {w} {
  $w.vit fbfmap
  # this should be changed if the ANN output is not phones (gmm)
  return [idx2sym [$w.vit getpath]]
}

# loadobs - loads an observation file
proc vit::loadobs {w filename} {
  variable Info
  if {$Info(obsisbin)} {
    set obs [fio::readbinmat $filename [llength $Info(symbols)]]
  } else {
    set obs [fio::readdata $filename]
  }

  $w.vit setobs $obs $Info(obsisact)
  return [llength $obs]
}

# setgram - set grammar from data structures
proc vit::setgram {w data} {
  variable Info

  set Info(fisstatemap) [lindex $data 2]
  $w.vit configure -grammar $data
}

# loadgram - loads all grammar definition files
proc vit::loadgram {w filelist} {
  variable Info
  set transmatFn [lindex $filelist 0]
  set statepriorFn [lindex $filelist 1]
  set fisstatemapFn [lindex $filelist 2]
  #set transmat [gram::applygfiw \
#		  [gram::parsetransmat [eval fio::readdata $transmatFn] \
#		     $Info(gramfact) $Info(insweight)]]
  set transmat [gram::parsetransmat [eval fio::readdata $transmatFn] \
		     $Info(gramfact) $Info(insweight)]
  set stateprior [gram::parsestprior [fio::readdata $statepriorFn]]
  set fisstatemap [gram::parsefisstateid [fio::readdata $fisstatemapFn]]
  # check consistency
  set Info(fisstatemap) $fisstatemap
  $w.vit configure -grammar [list $transmat $stateprior $fisstatemap]
}

# state2idx - convert grammar state indexes to symbol indexes
proc vit::state2idx {list} {
  variable Info
  set idx {}
  foreach state $list {
    lappend idx [lindex $Info(fisstatemap) $state]
    if {[lindex $idx end] == {}} {
      puts "st2idx $state"}
  }
  return $idx
}

# idx2symbol - converts indexes to phonetic symbols
proc vit::idx2sym {list} {
  variable Info
  set sym {}
  foreach idx $list {
    if {$idx == {}} {error "unknown index"}
    #puts "-- $idx [lindex $Info(symbols) $idx]"
    lappend sym [lindex $Info(symbols) $idx]
  }
  return $sym
}

proc vit::invoke {w cmd args} {

 set ns [namespace current]
 if {[info command ${ns}::$cmd]!=""} {
  eval uplevel [list ${ns}::$cmd $w $args]
 } else {
  eval uplevel [list $w.vit $cmd $args]
 }
}
