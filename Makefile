EXECUTABLE_NAME = main

# Build application(Default).
build:
 @echo Build application...
 cl src/main.c src/image_file_libpng.c lib/onnxruntime.lib lib/libpng16_static.lib lib/zlibstatic.lib /Iinclude
 move *exe bin
 @echo OK

run:
 @echo run application..
 .\bin\main.exe .\bin\candy.onnx .\bin\000000000285_720.png .\bin\output.png

# Clean
clean:
 @echo Clean...
 @if exist *.obj del *.obj
 @if exist .\bin\*.exe del .\bin\*.exe
 @echo OK