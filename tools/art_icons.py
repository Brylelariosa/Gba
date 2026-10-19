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
def recolor(rows, mp):
    """swap key letters (outline 'o' and anything not listed stays) so one silhouette serves several tiers"""
    return [''.join(mp.get(ch, ch) for ch in r) for r in rows]


SWORD = ["......os", ".....osm", "....osmo", "...osmo.", "o.osmo..", "oGsmo...", ".oGo....", "oBo....."]
STAFF = ["..ouuo..", ".oulluo.", ".oulluo.", "..ouuo..", "...oBo..", "...oBo..", "...oBo..", "...ooo.."]
BOW = ["..oo....", ".obbo...", "ob..os..", "ob...os.", "ob...os.", "ob..os..", ".obbo...", "..oo...."]
DAGGER = ["......os", ".....osm", "....osmo", "...osmo.", "..oGmo..", "..oGo...", ".oBo....", ".oo....."]
ARMOR = [".oo..oo.", "osmoosmo", "osssmsmo", ".osmmmo.", ".osmmmo.", ".odmmdo.", ".odddmo.", "..oooo.."]
HELM = ["..oooo..", ".osssmo.", "osssmmmo", "osmmmddo", "osoooodo", "osmoomdo", ".od..do.", "..oo.oo."]
BOOTS = ["........", "..oooo..", "..obbo..", "..obbo..", ".oobbo..", "obbbbbo.", "oBBBBBBo", ".oooooo."]

ICONS.update({
    # swords: wooden, iron, ember-lit, moonsteel, cinder-forged
    'sword0': recolor(SWORD, dict(s='b', m='B', G='B')),
    'sword1': recolor(SWORD, dict(G='d')),
    'sword2': ["....rgos", ".....orR", "....orRo", "...orRo.", "o.orRo..", "oGrRo...", ".oGo....", "oBo....."],
    'sword3': ["......ow", ".....owl", "....owlo", "...owlo.", "ogowlo..", "ogglo...", ".ogo....", "ouo....."],
    'sword4': ["....g.og", ".....ogR", "....ogRo", "...ogRo.", "orogRo..", "orrRo...", ".oro....", "odo....."],
    # staves: oak, crystal, arcane, starfall, ashen
    'staff0': ["..obbo..", ".obbbbo.", ".obbbBo.", "..obBo..", "...oBo..", "...oBo..", "...oBo..", "...ooo.."],
    'staff1': STAFF,
    'staff2': ["..oppo..", ".opwpGo.", ".oppppo.", "..oGGo..", "...oGo..", "...oBo..", "...oBo..", "...ooo.."],
    'staff3': ["...ow...", "..owwo..", ".owgwwo.", "..owwo..", "...oug..", "...ouo..", "...ouo..", "...ooo.."],
    'staff4': ["..orro..", ".orgrRo.", ".orgRRo.", "..oRRo..", "...odo..", "...odo..", "...odo..", "...ooo.."],
    # bows: short, long, elven, windpiercer, emberflight
    'bow0': ["........", "..oo....", ".obboos.", ".ob..os.", ".ob.os..", "..oo....", "........", "........"],
    'bow1': BOW,
    'bow2': ["..og....", ".onno...", "on..os..", "on...os.", "on...os.", "on..os..", ".onno...", "..og...."],
    'bow3': ["..ow....", ".olwo..w", "ol..ow..", "ol...ow.", "ol...ow.", "ol..ow..", ".olwo..w", "..ow...."],
    'bow4': ["..og..g.", ".oRRo...", "oR..or..", "oR...or.", "oR...or.", "oR..or..", ".oRRo...", "..og..g."],
    # daggers: iron, steel dirk, shadow fang, viper's kiss, soot reaver
    'dagger0': recolor(DAGGER, dict(G='d')),
    'dagger1': recolor(DAGGER, dict(s='w', m='s')),
    'dagger2': recolor(DAGGER, dict(s='p', m='d', G='p')),
    'dagger3': ["......on", ".....onn", "....onno", "...onno.", "..oGno..", "..oGo...", ".oBo..n.", ".oo....."],
    'dagger4': recolor(DAGGER, dict(s='g', m='R', G='r', B='d')),
    # body armor: cloth, leather, chain, plate, cinderplate, wolfhide
    'armor0': recolor(ARMOR, dict(s='l', m='u', d='d')),
    'armor1': recolor(ARMOR, dict(s='b', m='b', d='B')),
    'armor2': [".oo..oo.", "osmoosmo", "osmdmsmo", ".odmdmo.", ".omdmdo.", ".odmdmo.", ".omdmdo.", "..oooo.."],
    'armor3': [".oo..oo.", "owsoowso", "owwwwwso", ".owGGso.", ".owwwso.", ".osmmmo.", ".osssmo.", "..oooo.."],
    'armor4': [".oo..oo.", "oRroorRo", "oRrrRrRo", ".orggRo.", ".oRggRo.", ".oRRRRo.", ".oRRRdo.", "..oooo.."],
    'armor_pelt': [".oo..oo.", "owwoowwo", "owbwwbwo", ".obbbbo.", ".obBbBo.", ".obbbbo.", ".oBBBBo.", "..oooo.."],
    # helms: leather cap, iron, knight's (plumed), hollow crown, ashen visage, bonecrest
    'helm0': recolor(HELM, dict(s='b', m='b', d='B')),
    'helm1': HELM,
    'helm2': ["..orro..", ".orsmro.", "osssmmmo", "osmmmddo", "osoooodo", "osmoomdo", ".od..do.", "..oo.oo."],
    'helm3': [".o.oo.o.", "ogoggogo", "ogggggGo", "ogwggwGo", ".oGGGGo.", ".oooooo.", "........", "........"],
    'helm4': ["..oooo..", ".oRRRRo.", "oRRRRRRo", "orgRRgro", "oRRooRRo", ".oRRRRo.", "..oRRo..", "...oo..."],
    'helm_bone': ["o.oooo.o", "owowwowo", "owwwwwwo", "owmwwmwo", "owoooowo", "owmoomwo", ".om..mo.", "..oo.oo."],
    # boots: leather, iron greaves, swift, windrunners, magma treads, amberstrides
    'boots0': BOOTS,
    'boots1': recolor(BOOTS, dict(b='m', B='d')),
    'boots2': ["........", "..oooo..", "..obbo.w", "..obbowo", ".oobbo..", "obbbbbo.", "oBBBBBBo", ".oooooo."],
    'boots3': ["......w.", "..oooowo", "..olluwo", "..olluo.", ".oollo..", "olllllo.", "ouuuuuuo", ".oooooo."],
    'boots4': ["........", "..oooo..", "..oRRo..", "..oRRo..", ".ooRRo..", "oRRRRRo.", "ogggggRo", ".oooooo."],
    'boots_amber': recolor(BOOTS, dict(b='g', B='G')),
    # charms that used to borrow another item's icon
    'phoenix': ["...o....", "..orro..", ".orggro.", ".orgro..", "..orrgo.", "...orro.", "..orro..", ".oo.oo.."],
    'emberheart': ["........", ".og.go..", "ogrgrgo.", "ogrrrro.", ".orrrro.", "..orro..", "...oo...", "........"],
})

ICON_ORDER = ['potion', 'hipotion', 'ether', 'elixir', 'key', 'skullkey',
              'sword0', 'sword1', 'sword2', 'sword3', 'sword4', 'staff0', 'staff1', 'staff2', 'staff3', 'staff4',
              'bow0', 'bow1', 'bow2', 'bow3', 'bow4', 'dagger0', 'dagger1', 'dagger2', 'dagger3', 'dagger4',
              'armor0', 'armor1', 'armor2', 'armor3', 'armor4', 'armor_pelt',
              'helm0', 'helm1', 'helm2', 'helm3', 'helm4', 'helm_bone',
              'boots0', 'boots1', 'boots2', 'boots3', 'boots4', 'boots_amber',
              'ring', 'pendant', 'clover', 'heartcharm', 'shield', 'eye', 'feather', 'crystal', 'phoenix', 'emberheart',
              'gel', 'wing', 'pelt', 'bone', 'ember', 'iron', 'amber', 'cinderheart']


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
for _n, _src in (('snew', 'new'), ('sready', 'ready')):       # side quests: the same shapes in blue
    MARKS[_n] = [r.replace('g', 'l') for r in MARKS[_src]]
MARK_ORDER = ['new', 'ready', 'busy', 'snew', 'sready']


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
        c.art(["..o..", ".owo.", "owwwo", ".owo.", "..o.."], 1, 1, KEY)
    else:                         # 'dot': gold pip
        c.art([".oo.", "ogGo", ".oo."], 2, 2, KEY)
    return c


if __name__ == '__main__':
    from common import sheet
    fr = [icon(n) for n in ICON_ORDER]
    sheet(fr, ICON_PAL, 10, scale=14).save('/tmp/prev/icons.png')
    sheet([marker(n) for n in MARK_ORDER] + [blip('player'), blip('dot')], ICON_PAL, 5, scale=14).save('/tmp/prev/marks.png')
    print(len(fr), 'icons')
