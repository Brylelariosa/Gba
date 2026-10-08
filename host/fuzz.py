import random, sys
seed = int(sys.argv[1]); frames = int(sys.argv[2]); god = int(sys.argv[3])
r = random.Random(seed)
out = ['0 seed %d' % seed, '3 newgame %d' % r.randint(0, 3), '4 god %d' % god]
f = 10
keys_pool = [0x10, 0x20, 0x40, 0x80, 0x10|0x40, 0x10|0x80, 0x20|0x40, 0x20|0x80]
while f < frames:
    k = 0
    if r.random() < 0.7: k |= r.choice(keys_pool)
    if r.random() < 0.35: k |= 0x01
    if r.random() < 0.12: k |= 0x02
    if r.random() < 0.08: k |= 0x100
    if r.random() < 0.05: k |= 0x200
    if r.random() < 0.02: k |= 0x08
    out.append('%d keys %d' % (f, k))
    f += r.randint(1, 12)
    if r.random() < 0.01: out.append('%d warp %d %d %d' % (f, r.randint(0, 2), r.randint(3, 28), r.randint(3, 28)))
    if r.random() < 0.005: out.append('%d xp %d' % (f, r.randint(10, 400)))
    if r.random() < 0.003: out.append('%d gold %d' % (f, r.randint(0, 3000)))
    if r.random() < 0.004: out.append('%d give %d 1' % (f, r.randint(1, 25)))
    if r.random() < 0.002: out.append('%d spawn %d %d %d' % (f, r.randint(1, 4), r.randint(3, 28), r.randint(3, 28)))
print('\n'.join(out))
