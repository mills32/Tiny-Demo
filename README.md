ENG:

Tiny Demo (VGA Text mode)
--------------------------------------------

Another demo with a very original name. This time I wanted to see if a text mode could be useful for demos or games, and it is.

![plot](https://raw.githubusercontent.com/mills32/Tiny-Demo/master/tinydemo.png)

I used 40x25 text mode, (tweaked to be 40x30), and changed text cells to 8x8 pixels, (2 colors per cell). It kind of looks like
ZX Spectrum, color clash included, but this is a full tile/map mode, just like many systems of the era (NES, SNES, MD-Genesis... ). 
In this mode, VGA helps a lot the cpu, by drawing the entire screen for us, we just have to update the hardware scroll registers
or change some bytes (tiles) and a lot of stuff is possible, even simulate sprites by updating some tile data.

I tried to make it work on EGA, but I could not make it work on real hardware, only on emulators. Also there are issues that can't
be solved on EGA, such as the split screen shaking when scrolling.

Music used in this demo is called "Orgasmic Chipper 2" by Anthrox. I converted it to s3m, and edited the instruments in Adlib Tracker II.

Requirements:
- CPU 8088 4.77 Mhz
- RAM 320 Kb
- GRAPHICS VGA (EGA)
- SOUND Adlib or compatible

ESP:

Tiny Demo (Modo texto VGA)
--------------------------------------------

Otra demo con nombre super original... Esta vez quería ver si el modo texto de VGA podría ser capaz de mover demos o juegos, y vaya
si puede.

![plot](https://raw.githubusercontent.com/mills32/Tiny-Demo/master/tinydemo.png)

He usado el modo texto 40x25, (modificado para que sea 40x30), cambiando el tamaño de las celdas a 8x8 (dos colores por celda), y se 
parece un poco al ZX Spectrum, con color clash incluido, aunque se trata mas bien de un modo "mapa compuesto por celdas", exáctamente igual al de otros sistemas
de la epoca (NES, SNES, MD-Genesis...). En este modo la tarjeta VGA ayuda bastante a la CPU, basicamente haciendo casi todo por 
nosotros, sólamente tenemos que preocuparnos de mover los registros de scroll, o modificar unos cuantos bytes (tiles) para conseguir
cosas llamativas.

He intentado hacer que funcione en EGA, pero no conseguí gran cosa en hardware real, solo en emuladores. Además, hay problemas que 
no se pueden solucionar en EGA, como por ejemplo la imposibilidad de dejar estática la parte inferior de la pantalla, si utilizamos
la función "split screen" a la vez que hacemos scroll.

La música es "orgasmic chipper 2" de Anthrox, la convertí a s3m y la edite en Adlib Tracker II para usar instrumentos OPL2 en lugar de samples.


Requisitos:
- CPU 8088 4.77 Mhz
- RAM 320 Kb
- GRAFICOS VGA (EGA)
- SONIDO Adlib o compatible

