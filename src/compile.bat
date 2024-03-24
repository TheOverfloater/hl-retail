@ECHO OFF
:Mapname
REM Change dir first
cd .\compile

SET /P mapname=Map to compile: 
IF "%mapname%"=="" GOTO Error

REM Check if the BSP file exists
IF NOT exist %mapname%.map GOTO Error_NoMAP

ECHO Compiling %mapname%...
hlcsg -chart -estimate %mapname%.map
hlbsp -nobrink -chart -estimate %mapname%.map

REM Fail if we have a leak pointfile
IF exist %mapname%.lin GOTO Error_Leak

hlvis -chart -estimate %mapname%.map
hlrad -chart -blur 1.0 -smooth 150 -smooth2 60 -notexscale -minlight 1 -estimate %mapname%.map
GOTO Exit

:Error
ECHO Error: No map specified!
GOTO Mapname

:Error_NoMAP
ECHO Error: File %mapname%.map is missing
GOTO Exit

:Error_Leak
ECHO Error: Leak detected. Ceasing compile.
GOTO Exit

:Exit
set /p DUMMY=Hit ENTER to exit...