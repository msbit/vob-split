# Building

`gcc -o main main.c -ldvdread`

# Running

`./main VIDEO_TS 1`

# Post

`ffmpeg -i out-0.vob -map 0:1 -map 0:3 out-0.mkv`

or

`ffmpeg -i out-0.vob -map 0:1 -map 0:3 out-0.mp4`
