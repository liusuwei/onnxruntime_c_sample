EXECUTABLE_NAME = main

# Build application(Default).
build:
 @echo Build application...
 cl src/main.c lib/onnxruntime.lib /Iinclude
 move *exe bin
 @echo OK

run:
 @echo run application..
 .\bin\main.exe

# Clean
clean:
 @echo Clean...
 @if exist *.obj del *.obj
 @if exist .\bin\*.exe del .\bin\*.exe
 @echo OK