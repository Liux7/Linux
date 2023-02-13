set i=50
:L1
echo %i% 
..\bin\omptest.exe %i% >> res.txt
set /a i=(%i%)+50
if %i% LEQ 1000 goto L1