# +++++++++++++++++++++++++++++++++++++++++++++++++++
# Change to projects bin directory
# +++++++++++++++++++++++++++++++++++++++++++++++++++
cd "/media/ext/argos/misc/2Alejandro_2/BIN"
# +++++++++++++++++++++++++++++++++++++++++++++++++++
# Start the emulator server ...
# +++++++++++++++++++++++++++++++++++++++++++++++++++
set serverPid [ exec "xterm" "-hold" "-e"  "./as-mbEmulator" "-j" "../DAT/mbEmulator.json" & ]
# +++++++++++++++++++++++++++++++++++++++++++++++++++
# Start up 50 copies of the MODBUS test program
# The emulator should handle all of them but the 
# backlog is only 10 so you never know ...
# +++++++++++++++++++++++++++++++++++++++++++++++++++
for { set k  0 } { $k < 50 } { incr k } {
   lappend pidList [ exec "xterm"  "-hold" "-e" "./mbEmulatorTest" "-t" "-j" "../DAT/mbEmulator.json" & ]
   after 200
}
after 2000
foreach p $pidList {
  exec "kill" "-USR1" "$p"
}
after 1000
exec "kill" "-USR1" "$serverPid"
exec "pkill" "-USR1" "as-mbEmulator"

puts "**************************************************"
puts "*                 Seems Okay!                    *"
puts "**************************************************"


