# Virtual UDP Streaming Joypad

The start of a project with the intention of creating a virtual joypad to be used while streaming games to a friend in a similar way to steam remote play.

At the moment this can be coupled with a friends project:

https://github.com/TRACTOR-MAN/JOYPAD_UDP_TRANSMITTER


Port Info
---------
By default it uses UDP port 1235, Make sure router port forwarding is setup correctly and also some Linux distros may require the port to be allowed in software using :

firewall-cmd --add-port=1235/udp

Note: If you want to make the firewall-cmd permanent you need to add --permanent (NOT RECOMMENDED).

Streaming
---------
~~Currently this has been tested using ffmpeg to stream video using the following command:~~

~~ffmpeg -f x11grab -video_size 1920x1080 -framerate 60 -i :0.0 -threads 8 -preset ultrafast -vcodec libx264 -tune zerolatency -b:v 4M -x264opts intra-refresh=1 -f mpegts udp://TARGET_IP_HERE:1234?pkt_size=~~

~~Note: set TARGET_IP_HERE accordingly.~~

~~On the receiving end it is best to use MPV as it seems to provide the lowest latency:~~

~~mpv udp://127.0.0.1:1234 --no-cache --untimed --no-demuxer-thread --vd-lavc-threads=1 --profile=low-latency --hwdec=auto~~

The following project provides far superior latency than ffmpeg:
https://github.com/raspi/kaukosohva

General
-------
Over time I hope to integrate ffmpeg into the program to make it an all in one solution but for the time being having it independant appears to work well.
