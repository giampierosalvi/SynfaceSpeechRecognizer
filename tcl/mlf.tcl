############################################################################
#                         mlf.tcl  -  description
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

namespace eval mlf {
  variable Info
}

# open - open MLF in read or write mode handling header
proc mlf::open {fn mode} {
  set fp [::open $fn $mode]
  if {$mode == "w"} {
    puts $fp "\#\!MLF\!\#"
  } else {
    if {! [string match "\#\!MLF\!\#" [gets $fp] ]} {
      #error "wrong MLF header"
    }
  }
  return $fp
}

# close - close MLF file
proc mlf::close {fp} {
  ::close $fp
}

# writeblock - write an MLF block from frame-by-frame path
proc mlf::writeblock {fp blockname path} {
  puts $fp "\"*/$blockname\""
  foreach line [frame2label $path] {
    puts $fp $line
  }
  puts $fp "."
}

# writeblock2 - write an MLF block {name {lablist}} structure
proc mlf::writeblock2 {fp block} {
  foreach {name lab} $block break
  puts $fp $name
  foreach line $lab {
    puts $fp $line
  }
  puts $fp "."
}

# readblock - read an MLF block to label list
proc mlf::readblock {fp res} {
  upvar $res r
  # should get rid of quotes here
  set r(name) [gets $fp]
  if {[eof $fp]} {return -1}
  set label {}
  while {[set line [gets $fp]] != "."} {
    lappend label $line
  }
  set r(path) $label
  return 0
}
proc mlf::readblock {fp} {
  # get rid of name
  gets $fp
  if {[eof $fp]} {return ""}
  set label {}
  while {[set line [gets $fp]] != "."} {
    lappend label $line
  }

  return $label
}

# readblock - read an MLF block to {name {label list}}
proc mlf::readblock2 {fp} {
  # get rid of name
  set name [gets $fp]
  if {[eof $fp]} {return ""}
  set label {}
  while {[set line [gets $fp]] != "."} {
    lappend label $line
  }

  return [list $name $label]
}


# frame2label - converts frame-by-frame to MLF label
proc mlf::frame2label {frame} {
  set label {}
  set prev [lindex $frame 0]
  set start 0
  set end 0
  foreach sym $frame {
    if {$sym != $prev} {
      lappend label [list [expr $start*100000] [expr $end*100000] $prev]
      set prev $sym
      set start $end
    }
    incr end
  }
  lappend label [list [expr $start*100000] [expr $end*100000] $prev]

  return $label
}

# label2frame - converts MLF label to frame-by-frame
proc mlf::label2frame {label} {
  if {$label == ""} {return ""}
  set frame {}
  foreach line $label {
    set start [expr [lindex $line 0]/100000]
    set end [expr [lindex $line 1]/100000]
    set sym [lindex $line 2]
    for {set i $start} {$i<$end} {incr i} {
      lappend frame $sym
    }
  }

  return $frame
}
