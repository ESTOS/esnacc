rem This batch creates the files for the ROSE asn1 envelop used by the cpp and the typescript backends

..\output\bin\esnacc.exe -C -x -p -e -d -j SNACCROSE.asn1
if NOT %ERRORLEVEL% == 0 pause
move SNACCROSE.cpp ..\cpp-lib\src\SNACCROSE.cpp
move SNACCROSE.h ..\cpp-lib\include\SNACCROSE.h

..\output\bin\esnacc.exe -JT -j SNACCROSE.asn1
if NOT %ERRORLEVEL% == 0 pause
move SNACCROSE.ts ..\compiler\back-ends\ts-gen\gluecode\SNACCROSE.ts
move SNACCROSE_Converter.ts ..\compiler\back-ends\ts-gen\gluecode\SNACCROSE_Converter.ts
del *.ts

pause