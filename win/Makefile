build: main.exe

main.exe: main.obj so_stdio.lib
	link /out:main.exe main.obj so_stdio.lib

so_stdio.lib: so_stdio.dll

so_stdio.dll: so_stdio.obj
	link /nologo /dll /out:so_stdio.dll /implib:so_stdio.lib so_stdio.obj

so_stdio.obj: so_stdio.c
	cl /c /DDLL_EXPORTS so_stdio.c

main.obj: main.c
	cl /c main.c

clean:
	del *.obj
	del *.lib
	del *.dll
	del *.exe
	del *.exp