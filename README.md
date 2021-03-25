# Building

`make`

# Running

`./main VIDEO_TS 1`

# Post

```
for FILE in *.vob
do
  ffmpeg -i "${FILE}" -map 0:1 -map 0:3 "${FILE%vob}mp4" < /dev/null
done
```
