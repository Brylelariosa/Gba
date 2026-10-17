"""Item icons (8x8), quest markers (8x16) and minimap blips, all in ONE shared 15-colour sprite palette (bank 14).

Bank 14 used to hold just the red damage-number colours (idx 1-2); those two stay where they are, the other thirteen
entries are the icon colours, so the numerals keep working untouched.

key:  o outline   s/m/d steel light/mid/dark   b/B wood light/dark   g/G gold light/dark   r/R red / dark red
      u/l blue / light blue   n green   p purple   w white
"""
from common import Canvas, pal16

ICON_PAL = pal16(['#ff5050', '#401020', '#e4ecf8', '#a4b0c8', '#66728e', '#d09858', '#80502c', '#fff088', '#e0a020',
                  '#a02038', '#4a86f0', '#a8d8ff', '#58c850', '#b060f0', '#ffffff'])
KEY = dict(r=1, o=2, s=3, m=4, d=5, b=6, B=7, g=8, G=9, R=10, u=11, l=12, n=13, p=14, w=15)

ICONS = {
    'potion': ["..oBBo..", "...oo...", "..ollo..", ".orrrro.", ".orwrro.", ".orrrro.", "..oRRo..", "........"],
    'hipotion': ["..oBBo..", "...oo...", "..ollo..", ".orrrro.", "orrwrrro", "orrrrrRo", ".oRRRRo.", "..oooo.."],
    'ether': ["..oBBo..", "...oo...", "..ollo..", ".ouuuuo.", ".ouwuuo.", ".ouuuuo.", "..oooo..", "........"],
    'elixir': ["..oGGo..", "...oo...", "..ollo..", ".opppno.", ".opwpno.", ".oppnno.", "..oooo..", "........"],
    'key': ["..ooo...", ".ogggo..", ".og.go..", ".ogggo..", "..og....", "..ogGo..", "..og....", "..oGGo.."],
    'skullkey': ["..ooo...", ".owwwo..", ".owowo..", ".owwwo..", "..ow....", "..owwo..", "..ow....", "..oww..."],
    'sword': ["......os", ".....osm", "....osmo", "...osmo.", "o.osmo..", "oGsmo...", ".oGo....", "oBo....."],
    'staff': ["..ouuo..", ".oulluo.", ".oulluo.", "..ouuo..", "...oBo..", "...oBo..", "...oBo..", "...ooo.."],
    'bow': ["..oo....", ".obbo...", "ob..os..", "ob...os.", "ob...os.", "ob..os..", ".obbo...", "..oo...."],
    'dagger': ["......os", ".....osm", "....osmo", "...osmo.", "..oGmo..", "..oGo...", ".oBo....", ".oo....."],
    'armor': [".oo..oo.", "osmoosmo", "osssmsmo", ".osmmmo.", ".osmmmo.", ".odmmdo.", ".odddmo.", "..oooo.."],
    'helm': ["..oooo..", ".osssmo.", "osssmmmo", "osmmmddo", "osoooodo", "osmoomdo", ".od..do.", "..oo.oo."],
    'boots': ["........", "..oooo..", "..obbo..", "..obbo..", ".oobbo..", "obbbbbo.", "oBBBBBBo", ".oooooo."],
    'ring': ["..oggo..", ".og..go.", ".og..go.", "..oggo..", "...oo...", "..olno..", "...oo...", "........"],
    'pendant': ["..o..o..", "...oo...", "...oo...", "..oggo..", ".ogllgo.", ".oglugo.", "..oggo..", "...oo..."],
    'clover': ["..on.no.", ".onnonno", "onnnonnn", ".onnnnn.", "onnnonnn", ".onnonn.", "...on...", "..oBo..."],
    'heartcharm': ["........", ".or.ro..", "orrrrro.", "orwrrro.", ".orrrro.", "..orro..", "...oo...", "........"],
    'shield': [".oooooo.", "ossmmmdo", "osmggmdo", "osmggmdo", ".osmmdo.", ".osmmdo.", "..osdo..", "...oo..."],
    'eye': ["........", "..oooo..", ".ouwwuo.", "ouwppwuo", ".ouwwuo.", "..oooo..", "........", "........"],
    'feather': ["......ow", ".....oww", "....owsw", "...owsw.", "..owsw..", ".owsw...", "oBoo....", "oo......"],
    'crystal': ["...oo...", "..olwo..", ".olluuo.", ".olluuo.", ".oluuuo.", "..ouuo..", "...oo...", "........"],
    'gel': ["........", "...oo...", "..onno..", ".onwnno.", ".onnnnno", "onnnnnno", ".oooooo.", "........"],
    'wing': ["o.......", "oo....oo", "oRo..oRo", "oRRooRRo", "oRRRRRRo", "oRoRRoRo", "oR.oo.Ro", "o......."],
    'pelt': ["..oooo..", ".obbbbo.", "obBbBbbo", "obbBbbBo", "obBbbBbo", ".obbbbo.", "..oBBo..", "...oo..."],
    'bone': ["oo....oo", "owo..owo", ".owooow.", "..owwo..", "..owwo..", ".owooow.", "owo..owo", "oo....oo"],
    'ember': ["...oo...", "..orRo..", ".orggRo.", ".orgwgRo", ".orggRo.", "..orRo..", "...oo...", "........"],
    'iron': ["........", "..oooo..", ".osmmdo.", "osmsmmdo", "odmmsmdo", ".oddddo.", "..oooo..", "........"],
    'amber': ["...oo...", "..ogGo..", ".ogwgGo.", "oggwgggo", "ogggggGo", ".ogGGGo.", "..oooo..", "........"],
    'cinderheart': ["..oo.oo.", ".orrorro", "orwrrrro", "orrrgrro", ".orrgro.", "..orro..", "...oo...", "........"],
}
ICON_ORDER = ['potion', 'hipotion', 'ether', 'elixir', 'key', 'skullkey', 'sword', 'staff', 'bow', 'dagger', 'armor',
              'helm', 'boots', 'ring', 'pendant', 'clover', 'heartcharm', 'shield', 'eye', 'feather', 'crystal', 'gel',
              'wing', 'pelt', 'bone', 'ember', 'iron', 'amber', 'cinderheart']


def icon(name):
    rows = ICONS[name]
    assert len(rows) == 8 and all(len(r) == 8 for r in rows), (name, [len(r) for r in rows])
    c = Canvas(8, 8)
    c.art(rows, 0, 0, KEY)
    return c


# ----------------------------------------------------------------------------------------------- quest markers
# 8x16 sprites (two tiles, top then bottom) drawn above an NPC's head
MARKS = {
    'new': ["..oooo..", ".oggggo.", ".oggggo.", ".oggggo.", "..oggo..", "..oggo..", "..oggo..", "...oo...",
            "...oo...", "..oggo..", ".oggggo.", "..oggo..", "...oo...", "........", "........", "........"],
    'ready': ["..oooo..", ".oggggo.", "oggooggo", "oggooggo", ".oooggo.", "...oggo.", "..oggo..", "..oggo..",
              "..oggo..", "...oo...", "...oo...", "..oggo..", "..oggo..", "...oo...", "........", "........"],
    'busy': ["........", "........", "........", "........", "........", "........", "........", "........",
             "........", "........", "oo.oo.oo", "odoodood", "oo.oo.oo", "........", "........", "........"],
}
MARK_ORDER = ['new', 'ready', 'busy']


def marker(name):
    rows = MARKS[name]
    assert len(rows) == 16 and all(len(r) == 8 for r in rows), (name, [len(r) for r in rows])
    c = Canvas(8, 16)
    c.art(rows, 0, 0, KEY)
    return c


# ----------------------------------------------------------------------------------------------- minimap blips
def blip(kind):
    c = Canvas(8, 8)
    if kind == 'player':          # white diamond with a dark rim
        c.art([".o....", "owo...", ".o...."], 0, 0, KEY) if False else None
        c.art(["..oo..", ".owwo.", ".owwo.", "..oo.."], 1, 1, KEY)
    else:                         # 'dot': gold pip
        c.art([".oo.", "ogGo", ".oo."], 2, 2, KEY)
    return c


if __name__ == '__main__':
    from common import sheet
    fr = [icon(n) for n in ICON_ORDER]
    sheet(fr, ICON_PAL, 10, scale=14).save('/tmp/prev/icons.png')
    sheet([marker(n) for n in MARK_ORDER] + [blip('player'), blip('dot')], ICON_PAL, 5, scale=14).save('/tmp/prev/marks.png')
    print(len(fr), 'icons')
