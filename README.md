jalv.select
===========

![Puplic Domain](http://freedomdefined.org/upload/2/20/Pd-button.png)

A little gtkmm GUI to select lv2 plugs from a list
and run them with jalv. 

Features:
- select jalv interpreter from combo box,
- select LV2 plugin from list,
- select preset to load from menu
- search plugins by regex or plgin class,
- reload lilv world to catch new installed plugins,
- simple and lightweight in old unix style,
- load plugin with selected preset.
- minimize to systray (Hotkey SHIFT+ESCAPE)
- wake up from systray (Hotkey SHIFT+ESCAPE)

Depends:
- lilv
- gtkmm-2.4
- Xlib

Install:
- make
- make install
- see makefile for more options, eg. build debian package

<p><p\>
![Alternativer Text](http://i60.tinypic.com/33k42ut.png)
