package require Tk
package require math
package require math::statistics

namespace eval display {
  variable Info
  if {![info exists Info(Initialized)]} {
    set Info(Initialized) 1
    set Info(feature,data) {}
    set Info(feature,length) 13
    set Info(feature,range) [list -30.0 80.0]
    set Info(feature,range) [list -5.0 5.0]
    set Info(likelihood,data) {}
    set Info(likelihood,length) 50
    set Info(likelihood,range) [list 0 1]
    set Info(displaystopped) 1
  }
}

proc display::createtoplevel {r args} {
    set t .display
    set w $t.plot
    if {![winfo exists $t]} {
	toplevel $t
	display::create $w r args
	pack $w -expand 1 -fill both
    } else {
	wm deiconify $t
    }
    return $w
}

# not finished
proc display::create {w r args} {
  variable Info

  set Info(rec) $r
#  set Info(feature,length) [$Info(rec) getfeaturelen]
#  set Info(likelihood,length) [$Info(rec) getlikelihoodlen]

  frame $w
  set Info(displaystopped) 0
  canvas $w.feature
  canvas $w.likelihood
  pack $w.feature $w.likelihood
  eval ::math::statistics::plot-scale $w.feature 1 $Info(feature,length) $Info(feature,range)
  eval ::math::statistics::plot-scale $w.likelihood 1 $Info(likelihood,length) $Info(likelihood,range)
#  ::math::statistics::plot-xyline .c [list 1 2 3 4 5 6 7 8] [list 9 1 8 2 7 3 6 4]
#  startloop $w
  return $w
}

proc display::startloop {w} {
  variable Info
  set Info(displaystopped) 0

  loopproc $w
}

proc display::stoploop {w} {
  variable Info
  after cancel [namespace code [list loopproc $w]]
  set Info(displaystopped) 1
}

proc display::loopproc {w} {
  variable Info
  if $Info(displaystopped) return

  show $w

  after 100 [namespace code [list loopproc $w]]
}

proc display::show {w} {
  variable Info

  set Info(feature,data) [$Info(rec) getfeaturevec]
  set Info(likelihood,data) [$Info(rec) getlikelihoodvec]
  $w.feature delete feature
  $w.likelihood delete likelihood
  ::math::statistics::plot-tline $w.feature $Info(feature,data) feature
  ::math::statistics::plot-tline $w.likelihood $Info(likelihood,data) likelihood
}

proc display::destroy {w} {
  variable Info
  stoploop $w
  ::destroy $w
}

proc rangenorm {A B} {
  set outmin -1
  set outmax 1
  set resmin [expr {($outmin+$A)*$B}]
  set resmax [expr {($outmax+$A)*$B}]
  return [list $resmin $resmax]
} 
