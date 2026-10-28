#!/usr/bin/env bash
# Renders the CAD previews committed to the repo:
#   cad/preview/*.png  pictures for the README
#   cad/stl/*.stl      printable parts; GitHub shows them in a 3D viewer
# Run after changing anything in cad/. Needs openscad, plus xvfb-run on a
# machine without a display.
set -euo pipefail

cd "$(dirname "$0")/../cad"
mkdir -p preview stl

OPENSCAD=(openscad)
if [[ -z "${DISPLAY:-}" ]] && command -v xvfb-run >/dev/null; then
  OPENSCAD=(xvfb-run -a openscad)
fi

png() {  # name, camera rotation "rx,ry,rz", file, extra args...
  local name=$1 rot=$2 file=$3
  shift 3
  "${OPENSCAD[@]}" -q -o "preview/$name.png" --imgsize=1200,900 --viewall --autocenter \
    --colorscheme=Tomorrow --camera="0,0,0,$rot,0" "$file" "$@"
}

stl() {  # name, file, extra args...
  local name=$1 file=$2
  shift 2
  openscad -q --export-format binstl -o "stl/$name.stl" "$@" "$file"
}

stl shell enclosure.scad -D 'part="shell"'
stl base enclosure.scad -D 'part="base"'
stl disc-embedded-tag disc.scad -D 'embed_tag=true'
stl disc-open-pocket disc.scad -D 'embed_tag=false'
stl knob knob.scad

# Single parts are pictured from their exported STL, so they render in one
# flat colour instead of OpenSCAD's preview colours for cut faces.
part_png() {  # name, camera rotation, stl, colour
  local tmp
  tmp=$(mktemp --suffix=.scad)
  echo "color(\"$4\") import(\"$PWD/stl/$3.stl\");" > "$tmp"
  png "$1" "$2" "$tmp"
  rm -f "$tmp"
}

png assembly 70,0,335 enclosure.scad
png cutaway 65,0,35 enclosure.scad -D 'part="cutaway"'
part_png disc 40,0,20 disc-embedded-tag Gold
part_png knob 50,0,20 knob DimGray

ls -la preview stl
