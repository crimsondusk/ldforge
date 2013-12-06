LDForge
=======

Parts author's CAD for LDraw System of Tools. LDForge is part-oriented rather than model-oriented.
Its goal is to ease up the process of creation and modification of LDraw parts and help level the
learning curve involved by allowing polygon drawing, primitive selection, quick, direct access to
the LDraw code of individual objects, drawing overlays of part photos and providing interfaces to
commonly used utilities.

Features:
* List view ala MLCAD with multi-selection. One object per line, one line per object. Items not colored main or edge color (16/24) have their color reflected in the list view for identifying.
* Support for multiple open files (development version only)
* Parse error recovery, if a line/object cannot be parsed properly it will be displayed as an errorneous object. This object can be selected and its contents edited and have it reparsed, so you can fix these errors within LDForge.
* 6 camera modes plus a free-angle one.
* Drawing mode that allows you to literally draw polygons and lines into the screen.
* Circle drawing mode as an extension to drawing mode to allow easy circle and ring placement. (development version only)
* A simple primitive generator
* Object hiding
* Select by color or type
* Quick edge-lining, takes any number of polygons and creates edgelines around them
* Ability to edit object's LDraw code directly
* Inlining, plus deep inlining which grinds down to polygons only
* Auto-coloring (sets color to the first found unused color), uncoloring (sets colors to main/edge color based on type)
* Coordinate rounding, inverting, coordinate replacing, flipping, quad splitting
* Screenshotting
* Vertex object for coordinate storage
* LDConfig.ldr parsing for color information
* Ability to launch Philo's utilities and automatically merge in output
* BFC red/green view (incomplete)
* Wireframe mode, axis drawing
* Image overlays for getting part data from pictures

Forum thread: http://forums.ldraw.org/read.php?24,8711
