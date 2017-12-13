# fio.tcl
# - file and channel I/O of formatted data
# (c) Jonas Beskow 2002

namespace eval fio {}

# readdata - read delimited ascii elements from file to list (of lists)
proc fio::readdata {fn args} {
 set f [open $fn]
 set data [eval readdatachan [list $f] $args]
 close $f
 return $data
}

# readdatachan - read delimited ascii elements from channel to list (of lists)
# options: -relim      <row delimiter>
#          -cdelim     <column delimiter>
#          -allowempty <bool> (if false, empty elements will be removed)

proc fio::readdatachan {f args} {
 array set a {-rdelim \n -cdelim " \t" -allowempty 1}
 array set a $args
 set data {}
 
 foreach line [split [string trim [read $f]] $a(-rdelim)] {
  set nline [split [string trim $line] $a(-cdelim)]
  # discard all empty elements if allowempty is 0
  if {!$a(-allowempty)} {
   set nnline [list]
   foreach elem $nline {if {[string length $elem]!=0} {lappend nnline $elem}}
   set nline $nnline
  }
  if {$a(-allowempty) || [llength $nline]>0} {
   lappend data $nline
  }
 }
 return $data
}
 
# appenddata - write list of lists to file as delimited ascii data
proc fio::appenddata {fn data args} {
 set f [open $fn a]
 eval writedatachan [list $f] [list $data] $args
 close $f
}

# writedata - write list of lists to file as delimited ascii data
proc fio::writedata {fn data args} {
 set f [open $fn w]
 eval writedatachan [list $f] [list $data] $args
 close $f
}

# writedatachan - write list of lists to channel as delimited ascii data
proc fio::writedatachan {f data args} {
 array set a {-rdelim \n -cdelim \t -format ""}
 array set a $args
 set outdata ""
 foreach row $data {
  if {$a(-format) != ""} {
   set frow [list]
   foreach elem $row {lappend frow [format $a(-format) $elem]}
   set row $frow
  }
  lappend outdata [join $row $a(-cdelim)]
 }
 puts $f [join $outdata $a(-rdelim)]
}

# readbinmat - read binary data into a matix
proc fio::readbinmat {file ncols} {
 binary scan [readbinary $file] f* buf
 set nrows [expr [llength $buf]/$ncols]
 if {$nrows != [expr [llength $buf]./$ncols]} {
  error "incompatible number of elements"
 }

 set data {}
 for {set count 0} {$count<$nrows} {incr count} {
  set line [lrange $buf [expr $count*$ncols] \
	[expr ($count+1)*$ncols-1]]
  lappend data $line
 }
 return $data
}

# readbinary - read binary data from file into a binary string
proc fio::readbinary {file} {
 set f [open $file]
 fconfigure $f -translation binary -encoding binary
  set data [read $f]
 close $f
 return $data
}

# writebinary - write binary string to file
proc fio::writebinary {file data} {
 set f [open $file w]
 fconfigure $f -translation binary -encoding binary
 puts -nonewline $f $data
 close $f
}

# processfiles - loop through a list of files and evaluate a script 
#
# synopsis: 
# fio::procssfiles ?-dir dir? ?-ext ext? ?-verbose? ifilevar ofilevar ifilelist script
#
# the functionality is similar to foreach, except it always iuses two loop variables,
# ifile and ofile, where ofile is derived from ifile using 'dir' and 'ext' options:
#   ofile = <dir>[basename of ifile].<ext>
# if -dir and -ext are omitted, ofile will be equal to ifile
# if -verbose is specified, a progress printout of the form "ifile -> ofile" will be produced
# for each file pair

proc fio::processfiles {args} {
 package require cmdline

 array set a [cmdline::getoptions args {
  {dir.arg "" "output directory"}
  {ext.arg "" "output extension"}
  {verbose "show filenames on stdout"}
 }
 ]
 
 set ifilevar [lindex $args 0]
 set ofilevar [lindex $args 1]
 set filelist [lindex $args 2]
 set script   [lindex $args 3]

 upvar $ifilevar ifile
 upvar $ofilevar ofile

 foreach ifile $filelist {
  set ofile $ifile
  if {$a(dir) != ""} {
   set ofile [file join $a(dir) [file tail $ofile]]
  }
  if {$a(ext) != ""} {
   set ofile [file root $ofile].$a(ext)
  }
  if $a(verbose) {
   puts $ifile\t->\t$ofile
  }
  uplevel $script
 }
}

# trace - trace report
proc fio::trace {tracelevel level msg} {
  if {$tracelevel > $level} {
    puts stderr $msg
  }
}
