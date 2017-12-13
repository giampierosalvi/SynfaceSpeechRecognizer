############################################################################
#                          gram.tcl  -  description
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

namespace eval gram {
  variable Info
}
source [file join [file dirname [info script]] fio.tcl]
package require math

# parsetransmat - parse transmat (C-style indexes and modified weights)
proc gram::parsetransmat {data gramfact insweight} {
  set from {}
  set to {}
  set type {}
  set weight {}
  foreach line $data {
    # -1 converts from matlab to C style indexes
    lappend from [matlab2c [lindex $line 0]]
    lappend to   [matlab2c [lindex $line 1]]
    set tmptype [lindex $line 3]
    lappend type $tmptype
    set tmpw [lindex $line 2]
    if {$tmpw == "inf"} {
      set tmpw "-inf"
    } else {
      set tmpw -$tmpw
      if {$tmptype == 1} { set tmpw [expr $tmpw*$gramfact+$insweight] }
    }
    lappend weight $tmpw
  }
  return [list $from $to $weight $type]
}

# applygfiw: applies grammar factor and insertion weight
# NOT USED ANYMORE
proc gram::applygfiw {transmat gramfact insweight} {
  set from [lindex $transmat 0]
  set to [lindex $transmat 1]
  set weight [lindex $transmat 2]
  set type [lindex $transmat 3]
  set count 0
  foreach w $weight t $type {
    if {$w != "Ninf" && $t == 1} {
      lreplace $weight $count $count [expr $w*$gramfact+$insweight]
    }
    incr count
  }
  return [list $from $to $weight $type]
}

# parsefisstateid - parse fisstateid (C-style indexes)
proc gram::parsefisstateid {data} {
  return [matlab2c $data]
}

# parsestprior - inverst values including Inf
proc gram::parsestprior {data} {
  set res {}
  foreach line $data {
    if {$line == "inf"} {
      lappend res "-inf"
    } else {
      lappend res -$line
    }
  }

  return $res
}

# matlab2c - converts matlab style indexes to C style
proc gram::matlab2c {data} {
  set res {}
  foreach line $data {
    lappend res [expr $line-1]
  }
  return $res
}

# c2matlab - converts C style indexes to matlab style
proc gram::c2matlab {data} {
  set res {}
  foreach line $data {
    lappend res [expr $line+1]
  }
  return $res
}

# generate a grammar that is a linear combination of two contributions:
# 1) the normal free loop of phonemes,
# 2) the known phone sequence (forced alignment)
# algorithm:
# - first find how many different states are needed:
#   - count the number N of symbols
#   - count the number of times a symbol is repeated in the transcriptions
#   - take the sum.
#   example:
#     list of symbols = a b c d e f
#     transcription = a c a b d c a e
#   the original number of symbols is 6 but a repeats 3 times and c twice ->
#   the number of independent symbols needed is 6+2+1
#   A simple way of doing this is:
#   a b c d e f -> a0 b0 c0 d0 e0 f0
#   a c a b d c a e -> a0 c0 a1 b0 d0 c1 a2 e0
#   then merge and unique
# - then compute the free loop grammar
#   example:
#      a0      a1      a2      b0  c0      c1      d0  e0  f0
#   a0 1/(6*3) 1/(6*3) 1/(6*3) 1/6 1/(6*2) 1/(6*2) 1/6 1/6 1/6
# - then compute the forced alignment grammar
#   just follow the transcription
# - the final grammar is alpha*fl+(1-alpha)*fa
proc gram::trans2gram {symbols trans alpha} {
  set sym {}
  set tra {}
  foreach s $symbols {
    lappend sym ${s}_0
    set count($s) 0
  }
#  array count
  foreach l $trans {
    if {! [info exist count($l)]} {
      error "transcription symbol not in list $l"
    }
    if {$count($l) != 0} { lappend sym ${l}_$count($l) }
    lappend tra ${l}_$count($l)
    incr count($l)
  }
  puts $sym
  puts $tra
}

# mlfb2gram: generate grammar from mlf block
proc gram::mlfb2gram {symbols mlfblock phtranspref param how gramfact insweight} {
  set trans {}
  foreach line $mlfblock {
    set sym [lindex $line 2]
    if {$sym == "sp"} {set sym "sil"}
    lappend trans $sym
  }
  switch -- $how {
    alpha {
      return [trans2plfagram $symbols $trans $phtranspref $param \
		$gramfact $insweight]
    }
    wordlen {
      return [trans2wlgram $symbols $trans $phtranspref $param \
		$gramfact $insweight]
    }
    default { error "grammar $how not supported" }
  } 
}

proc gram::trans2plfagram {symbols trans phtranspref alpha gramfact insweight} {
  upvar $phtranspref phtransp

  # first find symbol position (for the map)
  set count 0
  foreach sym $symbols {
    set pos($sym) $count
    incr count
  }
  unset count
  set M [llength $trans]
  set N 0
  foreach l $trans {
    if {! [info exist count($l)]} {
      set count($l) 1
      incr N
    } else { incr count($l) }
  }
  # define (alpha*ergodic+(1-alpha)*alignment) transition probs
  for {set i 0} {$i<$M} {incr i} {
    for {set j 0} {$j<$M} {incr j} {
      # the next makes a real uniform distribution
      #set transp($i,$j) [expr $alpha*1.0 / ($count([lindex $trans $j])*$N)]
      set transp($i,$j) [expr $alpha*1.0 / $M]
      if ($j==[expr $i+1]) {
	# forced alignment is now 1/M instead of 1
	set transp($i,$j) [expr $transp($i,$j)+(1-$alpha)/$M]
      }
      if {$transp($i,$j) != 0} {
	set transp($i,$j) [expr log($transp($i,$j))*$gramfact + $insweight]
      } else {set transp($i,$j) "Ninf"}
    }
    # the next makes a real uniform distribution
    #set priorp($i) [expr $alpha*1.0 / ($count([lindex $trans $i])*$N)]
    set priorp($i) [expr $alpha*1.0 / $M]
    if {$i == 0} { set priorp($i) [expr $priorp($i)+1-$alpha] }
    if {$priorp($i) != 0} {
      set priorp($i) [expr log($priorp($i))]
    } else {set priorp($i) "Ninf"}
  }
  # construct grammar structures
  set map {}
  set prior {}
  set transmat {}
  for {set i 0} {$i<$M} {incr i} {
    set ph [lindex $trans $i]
#puts "phone $ph position $pos($ph)"
    lappend map $pos($ph) $pos($ph) $pos($ph) $pos($ph)
    lappend prior $priorp($i) Ninf Ninf Ninf
    if {! [info exists phtransp($ph,1,1)]} { 
      puts "damn\! $ph not in mmf"
      continue }
    lappend transmat [list [expr $i*4] [expr $i*4] $phtransp($ph,1,1) 0]
    lappend transmat [list [expr $i*4] [expr $i*4+1] $phtransp($ph,1,2) 0]
    lappend transmat [list [expr $i*4+1] [expr $i*4+1] $phtransp($ph,2,2) 0]
    lappend transmat [list [expr $i*4+1] [expr $i*4+2] $phtransp($ph,2,3) 0]
    lappend transmat [list [expr $i*4+2] [expr $i*4+2] $phtransp($ph,3,3) 0]
    lappend transmat [list [expr $i*4+2] [expr $i*4+3] $phtransp($ph,3,4) 0]
    for {set j 0} {$j<$M} {incr j} {
      if {! [info exists phtransp([lindex $trans $j],1,1)]} {
	puts "destination phone not avail: [lindex $trans $j]"
	continue 
      }
      if {$transp($i,$j) != "Ninf"} {
	lappend transmat [list [expr $i*4+3] [expr $j*4] $transp($i,$j) 1]
      }
    }
  }
  set transmat [transpose $transmat]
  return [list $transmat $prior $map]
}

proc gram::trans2wlgram {symbols trans phtranspref wrdlen gramfact insweight} {

  set M [llength $trans]

  # create words
  set words {}
  for {set i 0} {$i<=$M/$wrdlen} {incr i} {
    set word [lrange $trans [expr $i*$wrdlen] [expr ($i+1)*$wrdlen-1]]
    if {[llength $word] == 0} { continue }
    #if {[llength $word] == 1} {lappend words [list $word]
    #} else { }
    lappend words $word
  }
  return [gram::buildwordloop $symbols $words $phtranspref $gramfact $insweight]
}

# buildwordloop: build a loop of words
proc gram::buildwordloop {phones words phtranspref gramfact insweight} {
  upvar $phtranspref phtransp

  # first extract info on lengths and positions
  set count 0
  foreach ph $phones {
    set pos($ph) $count
    if {$ph != "sp"} {
      set l 0
      set entries [array names phtransp "$ph,*,*"]
      if {$entries == ""} {error "panic: info for ph: $ph not found"}
      foreach entry $entries {
	set els [split $entry ","]
	set l [::math::max $l [lindex $els 1] [lindex $els 2]]
      }
      if {$l == 0} {error "panic: zero length phone $ph"}
      # this is not $l+1 because els are in matlab style
      set len($ph) $l
    }
    incr count
  }
  set M [llength $phones]

  set start 0
  foreach word $words {
    lappend wordstart $start
    set l 0
    foreach ph $word {incr l $len($ph)}
    incr start $l
  }
  set N [llength $words]
  set transp [expr log(1.0/$N)*$gramfact+$insweight]
  # the within word transitions have prob 1/M
  set transph [expr log(1.0/$M)*$gramfact+$insweight]
  # construct grammar structures
  set map {}
  set prior {}
  set transmat {}
  foreach word $words wstart $wordstart {
#puts stderr "word starts"
    set phstart $wstart
    set phcount 0
    foreach ph $word {
#puts stderr "phone: $ph"
      for {set j 0} {$j<$len($ph)} {incr j} {lappend map $pos($ph)}
      if {$phcount == 0} {
	lappend prior $transp
	for {set j 1} {$j<$len($ph)} {incr j} {lappend prior Ninf}
      } else {
	for {set j 0} {$j<$len($ph)} {incr j} {lappend prior Ninf}
      }
      foreach entry [array names phtransp "$ph,*,*"] {
#puts stderr "entry: $entry"
	set prob $phtransp($entry)
	set els [split $entry ","]
	set st [expr [lindex $els 1]-1]
	set end [expr [lindex $els 2]-1]
	lappend transmat [list [expr $phstart+$st] \
			    [expr $phstart+$end] $prob 0]
      }
      if {$phcount < [expr [llength $word]-1]} {
	lappend transmat [list [expr $phstart+$len($ph)-1] \
			    [expr $phstart+$len($ph)] $transph 1]
      }
      incr phstart $len($ph)
      incr phcount
    }
    foreach towstart $wordstart {
      lappend transmat [list [expr $phstart-1] $towstart $transp 1]
    }
  }
  set transmat [transpose $transmat]
  return [list $transmat $prior $map]
}

proc gram::transpose {list} {
  set n [llength [lindex $list 0]]
  for {set i 0} {$i<$n} {incr i} {
    set col($i) {}
  }
  foreach row $list {
    for {set i 0} {$i<$n} {incr i} {
      lappend col($i) [lindex $row $i]
    }
  }
  set res {}
  for {set i 0} {$i<$n} {incr i} {
    lappend res $col($i)
  }

  return $res
}

# mmfgettransp: reads an mmf (htk) file and extracts transition probs
proc gram::mmfgettransp {mmffn res} {
  upvar $res r

  set linen 0
  set ph ""
  foreach line [fio::readdata $mmffn -cdelim "\n"] {
    if {[regexp {~h \"(.*)\"} $line dummy ph]} {
      continue }
    if {[regexp {\<TRANSP\> 5} $line]} {set linen 1; continue }
    if {$linen == 1} {
      if {$ph != "sp"} {
	incr linen
	continue
      } else { set linen 0 }
    }
    if {$linen>1 && $linen<5} {
      set transp [split [lindex $line 0]]
      set i [expr $linen-1]
      set j $linen
      set r($ph,$i,$i) [expr log([lindex $transp $i])]
      set r($ph,$i,$j) [expr log([lindex $transp $j])]
      incr linen
    }
  }

}

# writetransmat - 
proc gram::writetransmat {fn transmat} {
  foreach line $transmat {
    # -1 converts from matlab to C style indexes
    set from [c2matlab [lindex $line 0]]
    set to [c2matlab [lindex $line 1]]
    set tmpw [lindex $line 2]
    set type [lindex $line 3]
    if {$tmpw == "-inf"} { set tmpw "inf"
    } else {set tmpw [expr -$tmpw]}
    lappend out [list $from $to $tmpw $type]
  }
  fio::writedata $fn $out
}

# randompath: generates a random path in the grammar
proc gram::randompath {grammar n} {
  set transmat [linex $grammar 0]
  set stprior [lindex $grammar 1]
  set count $n
  set path [randbin $stprior 1]
  while {$n>0 && 1} {
    lappend path [randbin ]
  }

}

# randbin: return len symbols from a distribution binprob
proc randbin {binprob len} {
  set sum 0
  set step {}
  set res {}
  foreach v $binprob {
    set sum [expr $sum+$v]
    lappend step $sum
  }
  set outcome [::math::statistics::random-uniform 0 $sum $len]
  foreach out $outcome {
    set place 0
    while {$out>[lindex $step $place]} { incr place }
    lappend res $place
  }

  return $res
}

return

# this is a test with the swedish models
source ../tcl/gram.tcl
set symbols [fio::readdata /space2/space2/synface/prototype/viterbi_data/sv/phones_sv.lis]
set nospsym [lreplace $symbols 5 5]
gram::mmfgettransp /space2/space2/synface/prototype/viterbi_data/sv/mono_1_7.mmf phtransp
gram::trans2gram $symbols $symbols phtransp 1 grammar
fio::writedata netsimple_tcl.map [gram::c2matlab $grammar(fisstateid)]
fio::writedata netsimple_tcl.prior $grammar(stprior)
gram::writetransmat netsimple_tcl.transmat $grammar(transmat)
