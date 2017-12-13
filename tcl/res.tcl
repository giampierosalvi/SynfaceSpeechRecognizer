############################################################################
#                         res.tcl  -  result tools
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

namespace eval res {
  variable Info
}

# init: initialize results
proc res::init {symbols classes args} {
  variable Info
  array set Info {-nullclass ""}
  array set Info $args

  set Info(symbols) [list]
  foreach p1 $symbols {
    lappend Info(symbols) $p1
    foreach p2 $symbols {
      set Info(phconfusion,$p1,$p2) 0
    }
  }
  set Info(correct) 0
  set Info(incorrect) 0
  set Info(entropy,incorrect) {}
  set Info(entropy,correct) {}
  set Info(classes) [list]
  foreach class $classes {
    set firstel 1
    foreach el $class {
      if {$firstel} {
	set classname $el
	lappend Info(classes) $classname
	set firstel 0
      }
      set Info(class,$el) $classname
    }
  }
  foreach p1 $symbols {
    if {! [info exists Info(class,$p1)]} {
      set Info(class,$p1) $p1
      lappend Info(classes) $p1
    }
  }
  foreach c1 $Info(classes) {
    foreach c2 $Info(classes) {
      set Info(clconfusion,$c1,$c2) 0
    }
  }
}

# update: updates res accumulators and returns current results
proc res::update {refpath respath} {
  variable Info
  if {[llength $refpath] != [llength $respath]} {
    error "res::pathdiff: paths must have the same length"
  }
  set correct 0
  set incorrect 0
  foreach ref $refpath res $respath {
#puts stderr "ref: $Info(class,$ref) res: $Info(class,$res)"
    if {$Info(class,$ref)==$Info(-nullclass)} { continue }
    incr Info(phconfusion,$ref,$res)
    incr Info(clconfusion,$Info(class,$ref),$Info(class,$res))
    if {$Info(class,$ref) != $Info(class,$res)} {
      incr incorrect
    } else {
      incr correct
    }
  }
  incr Info(correct) $correct
  incr Info(incorrect) $incorrect
  return [list $correct $incorrect]
}

# report: returns a report with global results
proc res::report {args} {
  variable Info
  array set a {-confusion 0}
  array set a $args
  if {$a(-confusion)} {
    set report "total frames: [expr $Info(correct)+$Info(incorrect)]\n(correct $Info(correct) incorrect $Info(incorrect))\ncorr frame rate (%): [expr 100.0*$Info(correct)/($Info(incorrect)+$Info(correct))]\n"
    foreach p1 $Info(classes) {
      if {$p1 == $Info(-nullclass)} { continue }
      set report "$report\t$p1"
    }
    set report "$report\n"
    foreach c1 $Info(classes) {
      if {$c1 == $Info(-nullclass)} { continue }
      set report "$report$c1"
      foreach c2 $Info(classes) {
	if {$c2 == $Info(-nullclass)} { continue }
	set report "$report\t$Info(clconfusion,$c1,$c2)"
      }
      set report "$report\n"
    }
  } else {
    set report "total frames: [expr $Info(correct)+$Info(incorrect)]\n(correct $Info(correct) incorrect $Info(incorrect))\ncorr frame rate (%): [expr 100.0*$Info(correct)/($Info(incorrect)+$Info(correct))]\n"
  }
  return $report
}