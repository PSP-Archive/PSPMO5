
    Welcome to PSPMO5

Original Author of MO5

  Daniel Coulom   (see http://dcmo5.free.fr/)

Author of the PSP port version 

  Ludovic Jacomme also known as Zx-81 (see http://zx81.zx81.free.fr/)


1. INTRODUCTION
   ------------

DCMO5 is one of the best emulator of the Thomson MO5 home computer running on
MacOS, Windows and Unix. The emulator faithfully imitates the MO5 model.

PSPMO5 is a port on PSP of the version 11 of DCMO5.

Special thanks to Danzel and Jeff Chen for their virtual keyboard, and to all
PSPSDK developpers.

Thanks to Raven for the eboot icon's stuff.

Eboot music by NeXuS (see from http://www.jamendo.com/en/artist/nexus)

This package is under GPL Copyright, read COPYING file for more information
about it.


2. INSTALLATION
   ------------

Unzip the zip file, and copy the content of the directory fw5.x or fw1.5
(depending of the version of your firmware) on the psp/game, psp/game150, or
psp/game5xx if you use custom firmware 5.xx-M33.

It has been developped on linux for Firmware 5.01-m33 and i hope it works also
for all others M33 firmwares.

For any comments or questions on this version, please visit
http://zx81.zx81.free.fr or http://zx81.dcemu.co.uk


3. CONTROL
   ------------

3.1 - Virtual keyboard

In the MO5 emulator window, there are three different mapping (standard, left
trigger, and right Trigger mappings).  You can toggle between while playing
inside the emulator using the two PSP trigger keys.

    -------------------------------------
    PSP        MO5          (standard)
  
    Square     Eff
    Triangle   Return
    Circle     Fire
    Cross      Space
    Up         Up
    Down       Down
    Left       Left 
    Right      Right

    Analog     Joystick

    -------------------------------------
    PSP        MO5   (left trigger)
  
    Square     FPS
    Triangle   Return
    Circle     Swap analog / digital
    Cross      Eff
    Up         Up
    Down       Down
    Left       Render
    Right      Render

    -------------------------------------
    PSP        MO5   (right trigger)
  
    Square     Fire
    Triangle   Reset
    Circle     Eff
    Cross      Auto-fire
    Up         Up
    Down       Down
    Left       Dec fire 
    Right      Inc fire
  
    Analog     Joystick
    
    Press Start  + L + R   to exit and return to eloader.
    Press Select           to enter in emulator main menu.
    Press Start            open/close the On-Screen keyboard

  In the main menu

    RTrigger   Reset the emulator

    Triangle   Go Up directory
    Cross      Valid
    Circle     Valid
    Square     Go Back to the emulator window

  The On-Screen Keyboard of "Danzel" and "Jeff Chen"

    Use Analog stick to choose one of the 9 squares, and
    use Triangle, Square, Cross and Circle to choose one
    of the 4 letters of the highlighted square.

    Use LTrigger and RTrigger to see other 9 squares 
    figures.

3.2 - IR keyboard

You can also use IR keyboard. Edit the pspirkeyb.ini file to specify your IR
keyboard model, and modify eventually layout keyboard files in the keymap
directory.

  The following mapping is done :

  IR-keyboard   PSP

  Cursor        Digital Pad

  Tab           Start
  Ctrl-W        Start

  Escape        Select
  Ctrl-Q        Select

  Ctrl-E        Triangle
  Ctrl-X        Cross
  Ctrl-S        Square
  Ctrl-F        Circle
  Ctrl-Z        L-trigger
  Ctrl-C        R-trigger

In the emulator window you can use the IR keyboard to enter letters, special
characters and digits.

4. LOADING ROM FILES (ROM)
   ------------

If you want to load rom image in your emulator, you have to put your rom file
(with .zip or .rom file extension) on your PSP memory stick in the 'roms'
directory. 

Then, while inside MO5 emulator, just press SELECT to enter in the emulator main
menu, choose "Load ROM", and then using the file selector choose one rom file to
load in your emulator.

You can use the virtual keyboard in the file requester menu to choose the
first letter of the game you search (it might be useful when you have tons of
games in the same folder). Entering several time the same letter let you
choose sequentially files beginning with the given letter. You can use the Run
key of the virtual keyboard to launch the rom.

To eject the ROM you will need to select Hard Reset option in the main menu.

5. LOADING TAPE FILES (K7)
------------

If you want to load tape image in the virtual drive of your emulator, you have
to put your tape file (with .k7 file extension) on your PSP memory stick in the
'k7' directory. 

Then, while inside MO5 emulator, just press SELECT to enter in the emulator main
menu, choose "Load K7", and then using the file selector choose one tape file to
load in your emulator.

To run the game of your tape file, you have to use
the virtual keyboard (press START key) and type the
MO5 command 'LOADM"",,R' followed by ENTER
(Triangle).

You can also use directly the shortcut in the emulator menu (Command LOADM
option)

To RUN BASIC program, you have to type the MO5 command 'RUN""' followed by ENTER
(Triangle)

You can also use directly the shortcut in the emulator menu (Command RUN option)

It may happen that you need to rewind the tape using the Rewind K7 menu ...

The command 'LOAD""' (without M) is sometimes needed, when LOADM command doesn't
work.

If you use the MS Disk Basic (disk mode enabled), then you will have to put the
string "CASS:" in your 'LOADM', 'LOAD' or 'RUN' command as follows :

  LOADM"CASS:",,R 
  LOAD"CASS:",,R
  RUN"CASS:"

6. LOADING DISK FILES (FD)
------------

If you want to load floppy image in the virtual disk drive of your emulator, you
have to put your disk file (with .fd file extension) on your PSP memory stick in
the 'disc' directory. 

Then, while inside the emulator, just press SELECT to enter in the emulator main
menu, choose "Load Disc" and then using the file selector choose one disc file
to load in your emulator.

Use the same commands as described in the "Load tape" section.

The command AUTO is useful to run automatically the RUN"AUTO.BAT" command used
to most of all disk games.

If you want to specify the command to run for given games then you can do it in
the run.txt, using the following syntax :

  tapename=LOAD"CASS:RunName",,R
  tapename=LOADM"CASS:RunName",,R
  tapename=RUN"CASS:RunName"
  diskname=LOAD"RunName",,R
  diskname=LOADM"RunName",,R
  diskname=RUN"RunName"

7. LOADING SNAPSHOT FILES (STZ)
------------

I've modified original MO5 emulator to add a save state feature. The save state
format is specific to PSPMO5, but it might be useful to run previously loaded
games (using K7 and disk menu).

8. CHEAT CODE (.CHT)
   ----------

You can use cheat codes with MO5 emulator.  You can add your own cheat
codes in the cheat.txt file and then import them in the cheat menu.  

All cheat codes you have specified for a game can be save in a CHT file in
'cht' folder.  Those cheat codes would then be automatically loaded when you
start the game.

The CHT file format is the following :
#
# Enable, Address, Value, Comment
#
1,36f,3,Cheat comment

Using the Cheat menu you can search for modified bytes in RAM between current
time and the last time you saved the RAM. It might be very usefull to find
"poke" address by yourself, monitoring for example life numbers.

To find a new "poke address" you can proceed as follow :

Let's say you're playing Glouton and you want to find the memory address where
"number lives" is stored.

. Start a new game in glouton
. Enter in the cheat menu. 
. Choose Save Ram to save initial state of the memory. 
. Specify the number of lives you want to find in
  "Scan Old Value" field.
  (for Glouton the initial lives number is 3)
. Go back to the game and loose a life.
. Enter in the cheat menu. 
. Specify the number of lives you want to find in
  "Scan New Value" field.
  (for Glouton the lives number is now 2)
. In Add Cheat you have now one matching Address
. Specify the Poke value you want (for example 3) 
  and add a new cheat with this address / value.

The cheat is now activated in the cheat list and you can save it using the
"Save cheat" menu.

Let's enjoy Glouton with infinite life !!

9. COMMENTS
   ------------

You can write your own comments for games using the "Comment" menu.  The first
line of your comments would then be displayed in the file requester menu while
selecting the given file name (roms, keyboard, settings).

10. SETTINGS
   ------------

You can modify several settings value in the settings
menu of this emulator.  The following parameters are
available :

  Sound enable : 
    enable or disable the sound

  Display fps : 
    display real time fps value 

  Speed limiter :
    limit the speed to a given fps value

  Skip frame : 
    to skip frame and increase emulator speed

  Render mode : 
    many render modes are available with different
    geometry that should covered all games
    requirements

  Delta Y : 
    move the center of the screen vertically

  Vsync : 
    wait for vertical signal between each frame displayed

  Swap Analog/Cursor : 
    swap key mapping between PSP analog pad and PSP
    digital pad

  Auto fire period : 
    auto fire period

  Auto fire mode : 
    auto fire mode active or not

  Active Joystick : 
    Joystick player, it could be 1 or 2

  Display LR led : 
    draw a small blue rectangle in top of the screen
    when trigger keys are pressed

  Clock frequency : 
    PSP clock frequency, by default the value is set
    to 222Mhz, and should be enough for most of all
    games.

  Disk Mode : 
    Floppy disk is present yes or no.

11. LOADING KEY MAPPING FILES
 ------------

For given games, the default keyboard mapping between PSP Keys and MO5 keys, is
not suitable, and the game can't be played on PSPMO5.

To overcome the issue, you can write your own mapping file. Using notepad for
example you can edit a file with the .kbd extension and put it in the kbd
directory.

For the exact syntax of those mapping files, have a look on files generated in
the kbd directory (default.kbd etc ...).

After writting such keyboard mapping file, you can load them using the main menu
inside the emulator.

If the keyboard filename is the same as the rom file (.rom) then when you load
this rom file, the corresponding keyboard file is automatically loaded.

You can now use the Keyboard menu and edit, load and save your keyboard mapping
files inside the emulator. The Save option save the .kbd file in the kbd
directory using the "Game Name" as filename.  The game name is displayed on the
right corner in the emulator menu.

   
11. COMPILATION
   ------------

It has been developped under Linux using gcc with PSPSDK.  To rebuild the
homebrew run the Makefile in the src archive.

In the src/doc directory you will find original readme and license files of
DCMO5.
