jalv.select
===========

![Puplic Domain](http://freedomdefined.org/upload/2/20/Pd-button.png)

A little gtkmm GUI to select lv2 plugs from a list
and run them with jalv. 

Features:
- select jalv interpreter from combo box,
- select LV2 plugin from list,
- select preset to load from menu
- search plugins by regex or plugin class,
- reload lilv world to catch new installed plugins or presets,
- load plugin with selected preset.
- minimize app to systray (global Hotkey SHIFT+ESCAPE)
- wake up app from systray (global Hotkey SHIFT+ESCAPE)
 - left mouse click on systray to show or hide app
 - right mouse click to show quit menu item
- command-line start-up options:
```
  -s, --systray       start minimized in systray
  -H, --high=HIGH     start with given high in pixel
```
- command-line runtime options
```
  echo quit > /tmp/jalv.select.fifo$UID
  echo show > /tmp/jalv.select.fifo$UID
  echo hide > /tmp/jalv.select.fifo$UID
```
- keyboard shortcuts
  |   Command         | ==  |   Action                      |
  | ----------------  |:---:|:----------------------------- |
  |<ALT>+q or <CTRL>+q|==   |quit|
  |<ALT>+r or <CTRL>+r|==   |refresh plugin list|
  |<ESCAPE>           |==   |deselect preset menu|
  |<CTRL>+w           |==   |hide (minimize to systray icon)|
  |<ENTER> or <SPACE> |==   |select|
  |<UP>, <DOWN>       |==   |select plugin in list|
  |<PG_UP>, <PG_DOWN> |==   |scroll plugin list|


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
