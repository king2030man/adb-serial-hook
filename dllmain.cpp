Run $sources = Get-ChildItem -Path minhook\src -Filter *.c -Recurse | Select-Object -ExpandProperty FullName
Microsoft (R) C/C++ Optimizing Compiler Version 19.51.36252 for x86
Copyright (C) Microsoft Corporation.  All rights reserved.

dllmain.cpp
dllmain.cpp(208): error C2001: newline in string literal
dllmain.cpp(201): fatal error C1075: '{': no matching token found
Generating Code...
Compiling...
buffer.c
hook.c
trampoline.c
hde32.c
hde64.c
Generating Code...
Error: Process completed with exit code 1.
