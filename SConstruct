# -*- python -*-

e = Environment()

e.AppendUnique(CPPPATH=["/unsup/llvm-3.3/amd64_rhel6/include",
                        "/usr/lib/llvm-3.3/amd64_rhel6/include"])
e.AppendUnique(CPPDEFINES={"__STDC_LIMIT_MACROS": 1,
                           "__STDC_CONSTANT_MACROS": 1})
e.AppendUnique(CCFLAGS=['-g', '-fno-rtti'])

e.SharedLibrary('nwa-output', 'nwa-output.cc')
