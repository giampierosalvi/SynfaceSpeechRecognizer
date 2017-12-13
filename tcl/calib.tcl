# functions to create the calibration GUI

proc rec::createCalibItem {w wid chan spvar silvar minspeech maxsil} {
  set width 300
  #set spvar 0
  #set silvar 0
  set boldfont [font create -weight bold -size 8]
  set normalfont [font create -weight normal -size 8]
  frame $wid.ctrl
  frame $wid.ctrl.disp
  set disp $wid.ctrl.disp
  canvas $disp.m -width 150 -height 100
  createMeter $disp.m $minspeech $maxsil
  frame $wid.ctrl.but
  set but $wid.ctrl.but
#  button $but.b0 -foreground red -font $normalfont \
#    -text "speech calibration" \
#    -command [namespace code [list after 3000 [namespace code \
#    [list updateSpeechEnergy $w $chan $disp.m $spvar]]]]
  button $but.b0 -foreground red -font $normalfont \
    -text "speech calibration" \
    -command [namespace code \
     [list speechButtonCallback $but.b0 $w $chan $disp.m $spvar]]
  pack $but.b0
  message $but.v0 -width $width -font $normalfont -textvariable $spvar
  pack $but.v0
#  button $but.b1 -foreground blue -font $normalfont \
#    -text "silence calibration" \
#    -command [namespace code [list after 3000 \
#    [namespace code [list updateSilEnergy $w $chan $disp.m $silvar]]]]
puts "about the create sil button"
  button $but.b1 -foreground blue -font $normalfont \
    -text "silence calibration" \
    -command [namespace code \
     [list silButtonCallback $but.b1 $w $chan $disp.m $silvar]]
  pack $but.b1
  message $but.v1 -width $width -font $normalfont -textvariable $silvar
  pack $but.v1
  pack $but $disp -side left -expand true -fill x
  pack $wid.ctrl

}

proc rec::speechButtonCallback {b w chan m var} {
  $b configure -state disable
  after 3000 [namespace code [list updateSpeechEnergy $b $w $chan $m $var]]
}

proc rec::silButtonCallback {b w chan m var} {
  $b configure -state disable
  after 3000 [namespace code [list updateSilEnergy $b $w $chan $m $var]]
}

proc rec::destroyCalibTool {w} {
 # how do we destroy the window?
}

proc rec::updateThEnergy {w meter which} {
#  upvar [namespace current]::${w}::data($which) var
  upvar [namespace current]::${w}::data d
#  variable Info
  set d($which) [expr ($d(speechEnergyLocal)+$d(silEnergyLocal))/2]
  set d(simplexThreshold) [expr pow(10, $d($which))]
  configureMeter $meter $which $d($which)
}

proc rec::updateSilEnergy {b w channel meter var} {

  set $var [expr 10*log10([$w getavenergy $channel 300])]; 
  configureMeter $meter silence $$var
  $b configure -state normal
}

proc rec::updateSpeechEnergy {b w channel meter var} {

  set $var [expr 10*log10([$w getavenergy $channel 300])]; 
  configureMeter $meter speech $$var
  $b configure -state normal
}

# w:       recognizer object
# channel: left (0) right (1)
# meter:   meter object to update
# which:   both tag of the meter line and name of the variable holding
#          the value can be [speech,sil]Energy[Remote,Local]
proc rec::updateEnergy {w channel meter which} {
  upvar [namespace current]::${w}::data($which) var

  # note the $ because is a vraiable reference
  set var [expr 10*log10([$w getavenergy $channel 300])]; 
  configureMeter $meter $which $var
}

proc rec::createMeter {m minspeech maxsil} {

  if {$minspeech < $maxsil} {
    set minspeech -30
    set maxsil -55
  }
  set x 40
  set xx [expr $x+20]
  set y 10
  set yy1 [expr $y-$minspeech]
  set yy2 [expr $y-$maxsil]
  set yy3 [expr $y+80]
  $m create rectangle $x $y $xx $yy1 -fill gray70 -width 0
  $m create rectangle $x $yy1 $xx $yy2 -fill gray80 -width 0
  $m create rectangle $x $yy2 $xx $yy3 -fill gray60 -width 0
  $m create rectangle $x $y $xx $yy3
  $m create text [expr $x-7] [expr $y-5] -anchor ne -text "0 dB"
  $m create text [expr $x-7] [expr $yy3-5] -anchor ne -text "-80 dB"
  for {set i 10} {$i <100} {incr i 10} {
    $m create line [expr $x-5] $i $x $i
  }
  $m create text [expr $xx+5] 20 -anchor nw -text "speech region"
  $m create text [expr $xx+5] 70 -anchor nw -text "slience region"
  $m create line [expr $x-5] 10 [expr $xx+5] 10 -width 0 \
	  -fill red -tags speech
  $m create line [expr $x-5] 10 [expr $xx+5] 10 -width 0 \
	  -fill blue -tags silence
  pack $m

}

# now which is either speech or silence
proc configureMeter {m which level} {
  set level [expr int([expr -$level+10])]
  set coo [$m coords $which]
  set coo [list [lindex $coo 0] $level [lindex $coo 2] $level]
  $m coords $which $coo
  $m itemconfigure $which -width 2
}

# not used anymore
proc rec::calibrate {w} {
  upvar [namespace current]::${w}::data d

  set E 0
  for {set i 0} {$i < 300} {incr i} {

    set E [expr $E + [$w.rec getenergy]]
  }
  set E [expr 10*log10($E/300)]

  return $E
}
