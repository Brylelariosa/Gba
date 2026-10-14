#include "inventory.h"
#include "items.h"
#include <string.h>

Slot g_inv[INV_SLOTS];
void (*g_inv_hook)(u8 id);

void inv_clear(void)
{
    memset(g_inv, 0, sizeof g_inv);
}

int inv_add(u8 id, int n)
{
    if (id == 0 || n <= 0) return 0;
    int added = 0;

    if (item_max_dur(id) > 0) { /* tools: one per slot */
        for (int i = 0; i < INV_SLOTS && n > 0; i++) {
            if (g_inv[i].id == 0) {
                g_inv[i].id = id;
                g_inv[i].count = 1;
                g_inv[i].dur = item_max_dur(id);
                n--;
                added++;
            }
        }
    } else {
        for (int i = 0; i < INV_SLOTS && n > 0; i++) {
            if (g_inv[i].id == id && g_inv[i].count < STACK_MAX) {
                int add = MIN(n, STACK_MAX - g_inv[i].count);
                g_inv[i].count = (u8)(g_inv[i].count + add);
                n -= add;
                added += add;
            }
        }
        for (int i = 0; i < INV_SLOTS && n > 0; i++) {
            if (g_inv[i].id == 0) {
                int add = MIN(n, STACK_MAX);
                g_inv[i].id = id;
                g_inv[i].count = (u8)add;
                g_inv[i].dur = 0;
                n -= add;
                added += add;
            }
        }
    }
    if (added > 0 && g_inv_hook) g_inv_hook(id);
    return n;
}

int inv_space(u8 id)
{
    int space = 0;
    if (item_max_dur(id) > 0) {
        for (int i = 0; i < INV_SLOTS; i++)
            if (g_inv[i].id == 0) space++;
        return space;
    }
    for (int i = 0; i < INV_SLOTS; i++) {
        if (g_inv[i].id == 0) space += STACK_MAX;
        else if (g_inv[i].id == id) space += STACK_MAX - g_inv[i].count;
    }
    return space;
}

int inv_count(u8 id)
{
    int c = 0;
    for (int i = 0; i < INV_SLOTS; i++)
        if (g_inv[i].id == id) c += g_inv[i].count;
    return c;
}

bool inv_take(u8 id, int n)
{
    if (inv_count(id) < n) return false;
    for (int i = 0; i < INV_SLOTS && n > 0; i++) {
        if (g_inv[i].id != id) continue;
        int t = MIN(n, g_inv[i].count);
        g_inv[i].count = (u8)(g_inv[i].count - t);
        n -= t;
        if (g_inv[i].count == 0) memset(&g_inv[i], 0, sizeof(Slot));
    }
    return true;
}

void inv_move(int from, int to)
{
    if (from == to || from < 0 || to < 0 || from >= INV_SLOTS || to >= INV_SLOTS) return;
    Slot *a = &g_inv[from], *b = &g_inv[to];
    if (a->id != 0 && a->id == b->id && item_max_dur(a->id) == 0) {
        int room = STACK_MAX - b->count;
        int mv = MIN(room, a->count);
        b->count = (u8)(b->count + mv);
        a->count = (u8)(a->count - mv);
        if (a->count == 0) memset(a, 0, sizeof(Slot));
        return;
    }
    Slot tmp = *a;
    *a = *b;
    *b = tmp;
}

void inv_wear(int slot)
{
    if (slot < 0 || slot >= INV_SLOTS) return;
    Slot *s = &g_inv[slot];
    if (item_max_dur(s->id) == 0) return;
    if (s->dur <= 1) memset(s, 0, sizeof(Slot));
    else s->dur--;
}
