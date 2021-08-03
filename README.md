# Windows xargs clone: PPX2

Rescued source code and binary for *Windows xargs clone: PPX2*, which
was available at
[http://www.pirosa.co.uk:80/demo/wxargs/wxargs.html](http://web.archive.org/web/20201111193202/http://www.pirosa.co.uk:80/demo/wxargs/wxargs.html)

## Compile

(Cross)compile `ppx2.exe` Windows binary on Linux/MacOS/Windows with [zig](https://ziglang.org):

```
# 32-bit binary.
zig cc -target i386-windows-gnu -O2 -Wall -pthread -o ppx2_32bit.exe ppx2.c

# 64-bit binary.
zig cc -target x86_64-windows-gnu -O2 -Wall -pthread -o ppx2_64bit.exe ppx2.c
```

On Windows, it can be compiled with gcc too:
```
gcc -O2 -Wall -pthread -o ppx2.exe ppx2.c
```


## Original webpage

> There already exists a Windows xargs clone at the FindUtils for Windows GnuWin32 page.
> However at the time of last investigation I don't believe this had support for the -P <n> switch which allowed parallel processing of arguments.
>
> So I wrote a simple clone in C and compiled using MinGW.
>
> This clone is called PPX2 and is designed to function similarly to xargs but is a simplified version. No responsibility is taken for any problems you may have using this tool.
>
>
> ### Download
>
> You can get a copy of the tool here: ppx2.exe (16KB).
>
> The source code is made publically available here: [ppx2.c](http://web.archive.org/web/20200106135502/http://www.pirosa.co.uk:80/demo/wxargs/ppx2.c) (17KB) and is entirely my own work.
>
>
> ### Examples
>
> I primarily wrote this to take advantage of my multi-CPU Windows desktop so I could process videos/images in bulk using all my processors.
>
> #### Calculating SHA1 Sums
>
> As a basic example one can calculate the SHA1 of all the files in a directory tree by running:
>
```
dir C:\ /s /b | ppx2 -I {} -P 4 -L 1 sha1sum "{}"
```
>
>  - The `-I {}` is not necessary as it is configured that way by default.
>  - The `-P 4` allows a maximum of 4 simultaneous processes.
>  - The `-L 1` instructs the tool to split by line rather than by whitespace.
>
>
> #### Converting Videos
>
> Some versions of ffmpeg are already multi-processor capable. Of those that are there are some limitations depending on the codec used.
>
> To convert mpg to mp4 I did the following:
>
```
dir /b *.mpg | ppx2 -P 4 -L 1 ffmpeg.exe -i "{}" -quality:v 1 "{}.mp4"
```
>
> It is necessary to surround the `{}` in double quotes as Windows filenames often contain spaces.
>
