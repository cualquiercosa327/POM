#! /bin/sh

echo IUAAAVAT3BAAAQABCK5QUAAAAEFAAAACBAAJHQYVZIAACEQ45VOQAAAAAB0CEDAAAAAAAAAQCEJBGFAVCYLRQGI0DMOB0HQ5EAQSEIZEEUTCOKBJFIVSYLJOF2YDCMRTGQ0TMN0TRHVAE/ | cwpcm -w 16 | sox -e unsigned-integer -r 44100 -b 8 -t raw - text.wav
sox text.wav -r 22050 -c 1 -b 16 -e signed-integer text2.wav
sox text2.wav --bits 16 --encoding signed-integer --endian big text3.raw
